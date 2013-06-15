#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "vm.h"
#include "builtin.h"
#include "serialize.h"

void _assert_failed(const char* file, int line, const char* message, ...) {
  fprintf(stderr, "assertion failure, %s:%d:\n  ", file, line);

  va_list ap;
  va_start(ap, message);
  vfprintf(stderr, message, ap);
  va_end(ap);

  fprintf(stderr, "\n");

  exit(1);
}

struct heap_block_t {
  heap_block_t* next;
  size_t capacity;
  size_t used;
  uint8_t* data;
};

size_t max_sizet(size_t a, size_t b) {
  return a > b ? a : b;
}

heap_block_t* make_heap_block(size_t size, heap_block_t* next) {
  uint8_t* data = (uint8_t*) malloc(size);
  heap_block_t* h = (heap_block_t*)(void*)data;
  h->next = next;
  h->capacity = size - sizeof(heap_block_t);
  h->used = 0;
  h->data = data + sizeof(heap_block_t);
  return h;
}

void free_heap_block(heap_block_t* h) {
  if(h->next) {
    free_heap_block(h->next);
  }
  free(h);
}

Syms::Syms(VM& vm):
#define SYM(cpp, lisp) cpp(vm.makeSymbol(lisp)),
#include "symbols.inc.h"
#undef SYM
  dummy_value(0) {}

VM::VM(size_t heap_block_size):
  heap_block_size(heap_block_size),
  heap(make_heap_block(heap_block_size, 0)),
  transformer(0),
  prettyPrinter(0),
  nil(new(*this) Object(Object::Type::Nil)),
  true_(new (*this) Object(Object::Type::Bool)),
  false_(new (*this) Object(Object::Type::Bool)),
  symList(nil),
  syms(*this)
{
  VM& vm = *this;

  true_->as_bool = true;
  false_->as_bool = false;

  objs.builtin_add = make_builtin(vm, "add", builtin_add);
  objs.builtin_sub = make_builtin(vm, "sub", builtin_sub);
  objs.builtin_mul = make_builtin(vm, "mul", builtin_mul);
  objs.builtin_div = make_builtin(vm, "div", builtin_div);
  objs.builtin_modulo = make_builtin(vm, "modulo", builtin_modulo);
  objs.builtin_cons = make_builtin(vm, "cons", builtin_cons);
  objs.builtin_first = make_builtin(vm, "first", builtin_first);
  objs.builtin_rest = make_builtin(vm, "rest", builtin_rest);
  objs.builtin_is_equal = make_builtin(vm, "is_equal", builtin_is_equal);
  objs.builtin_concat = make_builtin(vm, "concat", builtin_concat);
  objs.builtin_split = make_builtin(vm, "split", builtin_split);
  objs.builtin_constructor = make_builtin(vm, "constructor", builtin_constructor);
  objs.builtin_symbol_name = make_builtin(vm, "symbol-name", builtin_symbol_name);
  objs.builtin_make_symbol = make_builtin(vm, "make-symbol", builtin_make_symbol);


  core_imports = makeList(
    makeCons(syms.add, objs.builtin_add),
    makeCons(syms.sub, objs.builtin_sub),
    makeCons(syms.mul, objs.builtin_mul),
    makeCons(syms.div, objs.builtin_div),
    makeCons(syms.modulo, objs.builtin_modulo),
    makeCons(syms.Cons, objs.builtin_cons),
    makeCons(syms.first, objs.builtin_first),
    makeCons(syms.rest, objs.builtin_rest),
    makeCons(syms.is_equal, objs.builtin_is_equal),
    makeCons(syms.concat, objs.builtin_concat),
    makeCons(syms.split, objs.builtin_split),
    makeCons(syms.ctor, objs.builtin_constructor),
    makeCons(syms.make_symbol, objs.builtin_make_symbol),
    makeCons(syms.symbol_name, objs.builtin_symbol_name));
  prettyPrinterImpl = nil;
  transformerImpl = nil;
  parserImpl = nil;
}

VM::~VM() {
  free_heap_block(heap);
}

void* VM::alloc(size_t size) {
  if(size > heap->capacity - heap->used) {
    heap = make_heap_block(max_sizet(heap_block_size, size * 2 + sizeof(heap_block_t)), heap);
  }
  void* ret = heap->data + heap->used;
  heap->used += size;
  return ret;
}

Value VM::makeCons(Value first, Value rest) {
  Value o = new(*this) Object(Object::Type::Cons);
  o->as_cons.first = first.getObj();
  o->as_cons.rest = rest.getObj();
  return o;
}

Value VM::makeSymbol(const String& name) {
  Value l = symList;
  while(!l.isNil()) {
    Value first = l->as_cons.first;
    l = l->as_cons.rest;
    const String& symName = first.asSymbolUnsafe();
    if(symName == name) {
      return first;
    }
  }
  Value o = new(*this) Object(Object::Type::Symbol);
  o.asSymbolUnsafe() = name;
  symList = makeCons(o, symList);
  return o;
}

Value VM::makeString(const String& value) {
  Value o = new(*this) Object(Object::Type::String);
  o.asStringUnsafe() = value;
  return o;
}

Value VM::makeInteger(int value) {
  Value o = new(*this) Object(Object::Type::Integer);
  o.asIntegerUnsafe() = value;
  return o;
}

extern const char binary_prettyprint_data[];
extern const char binary_transform_data[];
extern const char binary_parse_data[];

Value eval(VM& vm, Value o, Map env);

FILE* streamToFile(StandardStream stream) {
  switch(stream) {
  case StandardStream::StdOut:
    return stdout;
  case StandardStream::StdErr:
    return stderr;
  }
}

void VM::print(Value value, int indent, StandardStream stream) {
  suppressInternalRecursion = true;
  if(prettyPrinterImpl.isNil()) {
    prettyPrinterImpl = eval(*this, deserialize(*this, binary_prettyprint_data), nil);
  }
  FILE* s = streamToFile(stream);
  Value quoted_input = makeList(syms.quote, value);
  Value str = eval(*this, makeList(prettyPrinterImpl, quoted_input, makeInteger(indent)), nil);
  const String& data = str.asString(*this);
  fwrite(data.text, 1, data.length, s);
  fprintf(s, "\n");
  suppressInternalRecursion = false;
}

Value VM::transform(Value input) {
  suppressInternalRecursion = true;
  if(transformerImpl.isNil()) {
    transformerImpl = eval(*this, deserialize(*this, binary_transform_data), nil);
  }
  Value quoted_input = makeList(syms.quote, input);
  Value transformed = eval(*this, makeList(transformerImpl, quoted_input), nil);
  suppressInternalRecursion = false;
  return transformed;
}

Value VM::parse(const char* text, bool multiexpr) {
  suppressInternalRecursion = true;
  if(parserImpl.isNil()) {
    parserImpl = eval(*this, deserialize(*this, binary_parse_data), nil);
  }
  Value input = makeString(text);
  Value result = eval(*this, makeList(parserImpl, input, makeBool(multiexpr)), nil);
  suppressInternalRecursion = false;
  return result;
}

void VM::errorOccurred(const char* file, int line, const char* message) {
  fprintf(stderr, "error occurred: %s:%d: %s\n", file, line, message);
  currentEvalFrame->dump(StandardStream::StdErr);
  exit(1);
}

EvalFrame::EvalFrame(VM& vm, Value evaluating, Value env):
  vm(vm),
  evaluating(evaluating),
  env(env),
  previous(vm.currentEvalFrame)
{
  // if(!vm.suppressInternalRecursion) {
  //   printf("debug: ");
  //   vm.print(evaluating);
  // }
  vm.currentEvalFrame = this;
}

EvalFrame::~EvalFrame() {
  vm.currentEvalFrame = previous;
}

static bool obj_mentions_symbol(Value obj, Value symbol) {
  if(obj->type == Object::Type::Cons) {
    return
      obj_mentions_symbol(obj->as_cons.first, symbol) ||
      obj_mentions_symbol(obj->as_cons.rest, symbol);
  } else if(obj->type == Object::Type::Symbol) {
    return obj == symbol;
  } else {
    return false;
  }
}

void EvalFrame::dump(StandardStream stream) {
  FILE* f = streamToFile(stream);
  static const char prefix[] = "evaluating ";
  fprintf(f, prefix);
  vm.print(evaluating, strlen(prefix), stream);
  Value end = previous ? previous->env : vm.nil;
  while(!env.isNil() && env != end) {
    ASSERT(env.isCons());
    Value pair = env->as_cons.first;
    env = env->as_cons.rest;
    ASSERT(pair.isCons());
    Value key = pair->as_cons.first;

    if(obj_mentions_symbol(evaluating, key)) {
      Value value = pair->as_cons.rest;
      ASSERT(key.isSymbol());
      const char* text = key->as_symbol.text;
      size_t length = key->as_symbol.length;
      int len = fprintf(f, "    where %*s = ", (int)length, text);
      vm.print(value, len, stream);
    }
  }
  if(previous) {
    previous->dump(stream);
  }
}

static Value list_prepend_n_objs(VM& vm, size_t len, Value obj, Value list) {
  while(len > 0) {
    list = vm.makeCons(obj, list);
    len--;
  }
  return list;
}

static bool is_self_evaluating(Value o) {
  return o.isInteger() || o.isNil() || o.isBuiltin() || o.isBool() || o.isLambda() || o.isString();
}

static Value extend_env(VM& vm, Value params, Value args, Value env) {
  if(!params.isNil()) {
    Value key = cons_first(vm, params);
    EXPECT(key.isSymbol());
    Value value = cons_first(vm, args);
    return extend_env(vm,
      cons_rest(vm, params),
      cons_rest(vm, args),
      vm.makeCons(vm.makeCons(key, value), env));
  } else {
    VM_EXPECT(vm, args.isNil());
    return env;
  }
}

static Value make_lambdas_env(VM& vm, Value lambdas, Value env) {
  size_t len = list_length(lambdas);
  Value orig_env = env;

  env = list_prepend_n_objs(vm, len, vm.nil, env);

  Value envptr = env;
  while(!lambdas.isNil()) {
    Value lambda = cons_first(vm, lambdas);

    Value name_and_params = cons_first(vm, lambda);
    lambda = cons_rest(vm, lambda);
    VM_EXPECT(vm, cons_rest(vm, lambda).isNil());
    Value body = cons_first(vm, lambda);

    Value name = cons_first(vm, name_and_params);
    EXPECT(name.isSymbol());
    Value params = cons_rest(vm, name_and_params);

    lambda = make_lambda(vm, params, body, env);

    envptr->as_cons.first = vm.makeCons(name, lambda).getObj();
    envptr = cons_rest(vm, envptr);

    lambdas = cons_rest(vm, lambdas);
  }

  ASSERT(orig_env == envptr);

  return env;
}

Value eval(VM& vm, Value o, Map env);
static Value eval_list(VM& vm, Value o, Map env) {
  if(o.isCons()) {
    return vm.makeCons(eval(vm, cons_first(vm, o), env), eval_list(vm, cons_rest(vm, o), env));
  } else {
    return eval(vm, o, env);
  }
}

Value eval(VM& vm, Value o, Map env) {
  while(true) {
    EvalFrame frame(vm, o, env);
    if(is_self_evaluating(o)) {
      return o;
    } else if(o.isSymbol()) {
      return map_lookup(vm, env, o);
    } else if(o.isCons()) {

      Value f = cons_first(vm, o);
      o = cons_rest(vm, o);

      if(f == vm.syms.if_) {
        Value c = cons_first(vm, o);
        c = eval(vm, c, env);
        if(c.asBool(vm)) {
          o = cons_first(vm, cons_rest(vm, o));
        } else {
          o = cons_first(vm, cons_rest(vm, cons_rest(vm, o)));
        }
      } else if(f == vm.syms.letlambdas) {
        Value lambdas = cons_first(vm, o);
        o = cons_rest(vm, o);
        EXPECT(cons_rest(vm, o).isNil());
        o = cons_first(vm, o);
        env = make_lambdas_env(vm, lambdas, env);
      } else if(f == vm.syms.import) {
        Value lib = cons_first(vm, o);
        o = cons_rest(vm, o);
        Value sym = cons_first(vm, o);
        EXPECT(cons_rest(vm, o).isNil());
        EXPECT(lib == vm.syms.core);
        return map_lookup(vm, vm.core_imports, sym);
      } else if(f == vm.syms.quote) {
        Value a = cons_first(vm, o);
        EXPECT(cons_rest(vm, o).isNil());
        return a;
      } else {
        f = eval(vm, f, env);
        if(f.isBuiltin()) {
          Value params = eval_list(vm, o, env);
          EvalFrame builtinFrame(vm, vm.makeCons(f, params), env);
          Value res = builtin_func(f)(vm, params);
          return res;
        } else if(f.isLambda()) {
          Value params = eval_list(vm, o, env);
          env = extend_env(vm, lambda_params(f), params, lambda_env(f));
          o = lambda_body(f);
        } else {
          VM_ERROR(vm, "calling non-function value");
          return 0;
        }
      }
    } else {
      VM_ERROR(vm, "unknown value type");
      return 0;
    }
  }

}
