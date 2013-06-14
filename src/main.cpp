#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

#include "serialize.h"
#include "vm.h"

void _assert_failed(const char* file, int line, const char* message, ...) {
  fprintf(stderr, "assertion failure, %s:%d:\n  ", file, line);

  va_list ap;
  va_start(ap, message);
  vfprintf(stderr, message, ap);
  va_end(ap);

  fprintf(stderr, "\n");

  exit(1);
}

// #define ENABLE_DEBUG

#ifndef ENABLE_DEBUG
#define DEBUG_LOG(val)
#else
#define DEBUG_LOG(val) val
#endif

Value make_builtin(VM& vm, const char* name, BuiltinFunc func);

void* Object::operator new (size_t size, VM& vm) {
  return vm.alloc(size);
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

bool obj_mentions_symbol(Value obj, Value symbol);
FILE* streamToFile(StandardStream stream);

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

Value cons_first(VM& vm, Value o) {
  VM_EXPECT(vm, o.isCons());
  return o->as_cons.first;
}

Value cons_rest(VM& vm, Value o) {
  VM_EXPECT(vm, o.isCons());
  return o->as_cons.rest;
}

size_t list_length(Value list) {
  size_t ret = 0;
  while(!list.isNil()) {
    ret++;
    ASSERT(list.isCons());
    list = list->as_cons.rest;
  }
  return ret;
}

Value list_prepend_n_objs(VM& vm, size_t len, Value obj, Value list) {
  while(len > 0) {
    list = vm.makeCons(obj, list);
    len--;
  }
  return list;
}

int integer_value(VM& vm, Value o) {
  VM_EXPECT(vm, o.isInteger());
  return o->as_integer.value;
}

typedef Value Map;

Value map_lookup(VM& vm, Map map, Value key) {
  Value p = map;
  while(!p.isNil()) {
    Value item = cons_first(vm, p);
    if(cons_first(vm, item) == key) {
      return cons_rest(vm, item);
    } else {
      p = cons_rest(vm, p);
    }
  }
  VM_ERROR(vm, "lookup failed");
  return 0;
}

BuiltinFunc builtin_func(Value o) {
  EXPECT(o.isBuiltin());
  return o->as_builtin.func;
}

const char* builtin_name(Value o) {
  EXPECT(o.isBuiltin());
  return o->as_builtin.name;
}

Value make_builtin(VM& vm, const char* name, BuiltinFunc func) {
  Value o = new(vm) Object(Object::Type::Builtin);
  o->as_builtin.name = name;
  o->as_builtin.func = func;
  return o;
}

Value lambda_params(Value o) {
  EXPECT(o.isLambda());
  return o->as_lambda.params;
}

Value lambda_body(Value o) {
  EXPECT(o.isLambda());
  return o->as_lambda.body;
}

Value lambda_env(Value o) {
  EXPECT(o.isLambda());
  return o->as_lambda.env;
}

Value make_lambda(VM& vm, Value params, Value body, Value env) {
  Value o = new(vm) Object(Object::Type::Lambda);
  o->as_lambda.params = params.getObj();
  o->as_lambda.body = body.getObj();
  o->as_lambda.env = env.getObj();
  return o;
}

bool obj_equal(Value a, Value b) {
  if(a->type != b->type) {
    return false;
  }
  switch(a->type) {
  case Object::Type::Nil:
    ASSERT(b.isNil() == (a == b));
    return b.isNil();
  case Object::Type::Cons:
    if(obj_equal(a->as_cons.first, a->as_cons.first)) {
      if(obj_equal(a->as_cons.rest, b->as_cons.rest)) {
        return true;
      }
    }
    return false;
  case Object::Type::String:
    return a.asStringUnsafe() == b.asStringUnsafe();
  case Object::Type::Integer:
    return a->as_integer.value == b->as_integer.value;
  case Object::Type::Symbol:
  case Object::Type::Builtin:
    return a == b;
  case Object::Type::Bool:
    ASSERT( (a == b) == (a.asBoolUnsafe() == b.asBoolUnsafe()) );
    return a == b;
  default:
    EXPECT(0);
    return false;
  }
}

bool obj_mentions_symbol(Value obj, Value symbol) {
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

bool is_self_evaluating(Value o) {
  return o.isInteger() || o.isNil() || o.isBuiltin() || o.isBool() || o.isLambda() || o.isString();
}

Value extend_env(VM& vm, Value params, Value args, Value env) {
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

Value make_lambdas_env(VM& vm, Value lambdas, Value env) {
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
Value eval_list(VM& vm, Value o, Map env) {
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

void testMakeList() {
  VM vm;

  Value one = vm.makeInteger(1);
  Value two = vm.makeInteger(2);
  Value three = vm.makeInteger(3);

  {
    Value a = vm.makeList();
    Value b = vm.nil;
    EXPECT(obj_equal(a, b));
  }

  {
    Value a = vm.makeList(one);
    Value b = vm.makeCons(one, vm.nil);
    EXPECT(obj_equal(a, b));
  }

  {
    Value a = vm.makeList(one, two);
    Value b = vm.makeCons(one, vm.makeCons(two, vm.nil));
    EXPECT(obj_equal(a, b));
  }

  {
    Value a = vm.makeList(one, two, three);
    Value b = vm.makeCons(one, vm.makeCons(two, vm.makeCons(three, vm.nil)));
    EXPECT(obj_equal(a, b));
  }

}

void testSymbols() {
  VM vm;

  Value a = vm.makeSymbol("a");
  Value b = vm.makeSymbol("b");
  Value a2 = vm.makeSymbol("a");

  Value one = vm.makeInteger(1);
  Value two = vm.makeInteger(2);

  EXPECT(a == a2);
  EXPECT(a != b);
  EXPECT(a2 != b);

  Value map = vm.makeList(
    vm.makeCons(a, one),
    vm.makeCons(b, two));

  EXPECT_INT_EQ(integer_value(vm, map_lookup(vm, map, a)), 1);
  EXPECT_INT_EQ(integer_value(vm, map_lookup(vm, map, b)), 2);
}

void testParse() {
  VM vm;

  {
    EXPECT_INT_EQ(integer_value(vm, vm.parse("1")), 1);
    EXPECT_INT_EQ(integer_value(vm, vm.parse("42")), 42);
  }

  {
    EXPECT(vm.parse("#t").asBool(vm) == true);
    EXPECT(vm.parse("#f").asBool(vm) == false);
  }

  {
    Value res = vm.parse("a");
    EXPECT(res.isSymbol());
  }

  {
    Value res = vm.parse("\"a\"");
    EXPECT(obj_equal(res, vm.makeString("a")));
  }

  {
    Value res = vm.parse("(a b)");
    Value a = vm.makeSymbol("a");
    Value b = vm.makeSymbol("b");
    EXPECT(cons_first(vm, res) == a);
    EXPECT(cons_first(vm, cons_rest(vm, res)) == b);
    EXPECT(cons_rest(vm, cons_rest(vm, res)).isNil());
  }
}

void testEval() {
  VM vm;

  Value plus = vm.syms.add;
  Value add = vm.objs.builtin_add;
  Value _if = vm.syms.if_;
  Value _true = vm.true_;
  Value _false = vm.false_;
  Value one = vm.makeInteger(1);
  Value two = vm.makeInteger(2);

  {
    EXPECT_INT_EQ(integer_value(vm, eval(vm, vm.makeInteger(1), vm.nil)), 1);
    EXPECT_INT_EQ(integer_value(vm, eval(vm, vm.makeInteger(42), vm.nil)), 42);
  }

  {
    Value list = vm.makeList(add, one, two);

    EXPECT_INT_EQ(integer_value(vm, eval(vm, list, vm.nil)), 3);
  }

  {
    Value a = vm.makeSymbol("a");
    Value pair = vm.makeCons(a, vm.makeInteger(42));
    Value env = vm.makeCons(pair, vm.nil);
    EXPECT_INT_EQ(integer_value(vm, eval(vm, a, env)), 42);
  }

  {
    Value list = vm.makeList(plus, one, two);

    Value pair = vm.makeCons(plus, add);
    Value env = vm.makeCons(pair, vm.nil);
    EXPECT_INT_EQ(integer_value(vm, eval(vm, list, env)), 3);
  }

  {
    Value list = vm.makeList(_if, _true, one, two);
    EXPECT_INT_EQ(integer_value(vm, eval(vm, list, vm.nil)), 1);
  }

  {
    Value list = vm.makeList(_if, _false, one, two);
    EXPECT_INT_EQ(integer_value(vm, eval(vm, list, vm.nil)), 2);
  }

}

void testParseAndEval() {
  VM vm;

  {
    Value plus = vm.syms.add;
    Value add = vm.objs.builtin_add;

    Value pair = vm.makeCons(plus, add);
    Value env = vm.makeCons(pair, vm.nil);

    Value input = vm.parse("(+ 1 2)");
    Value res = eval(vm, input, env);

    EXPECT_INT_EQ(integer_value(vm, res), 3);
  }

  {
    Value scons = vm.syms.Cons;
    Value cons = vm.objs.builtin_cons;

    Value env = vm.makeList(vm.makeCons(scons, cons));

    Value input = vm.parse("((letlambdas ( ((myfunc x y) (cons x y)) ) myfunc) 1 2)");
    Value res = eval(vm, input, env);

    EXPECT(obj_equal(res, vm.makeCons(vm.makeInteger(1), vm.makeInteger(2))));
  }

  {
    Value input = vm.parse("((import core +) 1 2)");
    Value res = eval(vm, input, vm.nil);

    EXPECT_INT_EQ(integer_value(vm, res), 3);
  }

}

void testSerialize() {
  VM vm;

  {
    Value original = vm.nil;
    String serialized = serialize(original);
    Value deserialized = deserialize(vm, serialized.text);
    EXPECT(obj_equal(deserialized, original));
  }

  {
    Value original = vm.makeInteger(0);
    String serialized = serialize(original);
    Value deserialized = deserialize(vm, serialized.text);
    EXPECT(obj_equal(deserialized, original));
  }

  {
    Value original = vm.makeInteger(1);
    String serialized = serialize(original);
    Value deserialized = deserialize(vm, serialized.text);
    EXPECT(obj_equal(deserialized, original));
  }

  {
    Value a = vm.makeSymbol("some-random-symbol");
    Value original = vm.makeList(a,
      vm.makeInteger(1),
      vm.makeInteger(42),
      vm.makeInteger(129),
      vm.makeInteger(4),
      vm.makeInteger(5));
    String serialized = serialize(original);
    Value deserialized = deserialize(vm, serialized.text);
    EXPECT(obj_equal(deserialized, original));
  }
}

void testAll() {
  testMakeList();
  testSymbols();
  testParse();
  testEval();
  testParseAndEval();
  testSerialize();
}

char* loadBytes(const char* file) {
  FILE* f = fopen(file, "rb");
  fseek(f, 0, SEEK_END);
  size_t len = ftell(f);
  fseek(f, 0, SEEK_SET);
  char* buf = (char*) malloc(len + 1);
  fread(buf, 1, len, f);
  fclose(f);
  buf[len] = 0;
  return buf;
}

void saveBytes(const char* file, String data) {
  FILE* f = fopen(file, "wb");
  fwrite(data.text, 1, data.length, f);
  fclose(f);
}

Value parse_file(VM& vm, const char* file, bool multiexpr) {
  char* buf = loadBytes(file);

  Value obj = vm.parse(buf, multiexpr);

  free(buf);

  return obj;
}

Value run_file(VM& vm, const char* file, bool multiexpr) {
  Value obj = parse_file(vm, file, multiexpr);
  return eval(vm, obj, vm.nil);
}

Value run_transform_file(VM& vm, const char* file) {
  Value obj = vm.transform(parse_file(vm, file, true));
  // vm.print(obj, printf("transformed:"));
  return obj;
}

int main(int argc, char** argv) {
  bool test = false;
  const char* transform_file = 0;
  const char* file = 0;
  const char* serialize_to = 0;
  const char* deserialize_from = 0;


  enum {
    START,
    TRANSFORM_FILE,
    SERIALIZE,
    DESERIALIZE,
  } state = START;

  for(int i = 1; i < argc; i++) {
    const char* arg = argv[i];
    switch(state) {
    case START:
      if(strcmp(arg, "--transform-file") == 0) {
        state = TRANSFORM_FILE;
      } else if(strcmp(arg, "--test") == 0) {
        test = true;
      } else if(strcmp(arg, "--serialize") == 0) {
        state = SERIALIZE;
      } else if(strcmp(arg, "--deserialize") == 0) {
        state = DESERIALIZE;
      } else {
        file = arg;
        state = START;
      }
      break;
    case TRANSFORM_FILE:
      transform_file = arg;
      state = START;
      break;
    case SERIALIZE:
      serialize_to = arg;
      state = START;
      break;
    case DESERIALIZE:
      deserialize_from = arg;
      state = START;
      break;
    }
  }
  
  VM vm;

  if(state != START) {
    fprintf(stderr, "couldn't parse arguments %d\n", state);
  } else {
    if(test) {
      if(file || transform_file) {
        fprintf(stderr, "can't provide both --test and (--transform-file or file to run)\n");
      } else {
        testAll();
        return 0;
      }
    } else if(transform_file) {
      if(file) {
        fprintf(stderr, "can't provide both --transform-file and file to run\n");
      } else {
        Value transformed = run_transform_file(vm, transform_file);
        if(serialize_to) {
          String data = serialize(transformed);
          saveBytes(serialize_to, data);
        } else {
          vm.print(transformed);
        }
        return 0;
      }
    } else if(serialize_to) {
      Value value = parse_file(vm, file, false);
      String data = serialize(value);
      saveBytes(serialize_to, data);
      return 0;
    } else if(deserialize_from) {
      const char* data = loadBytes(deserialize_from);
      Value value = deserialize(vm, data);
      vm.print(value);
      return 0;
    } else if(file) {
      Value transformed = run_transform_file(vm, file);
      Value result = eval(vm, transformed, vm.nil);

      vm.print(result);
      return 0;
    } else {
      fprintf(stderr, "must provide either --transform-file or file to run\n");
    }
  }
  return 1;
}

