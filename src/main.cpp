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
      const char* name = key->as_symbol.name;
      int len = fprintf(f, "    where %s = ", name);
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
    list = vm.Cons(obj, list);
    len--;
  }
  return list;
}

Value make_string(VM& vm, const char* value) {
  Value o = new(vm) Object(Object::Type::String);
  o->as_string.value = value;
  return o;
}

const char* string_value(Value o) {
  EXPECT(o.isString());
  return o->as_string.value;
}

int integer_value(VM& vm, Value o) {
  VM_EXPECT(vm, o.isInteger());
  return o->as_integer.value;
}

const char* symbol_name(Value o) {
  EXPECT(o.isSymbol());
  return o->as_symbol.name;
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

Value make_symbol_with_length(VM& vm, const char* begin, size_t length) {
  Value o = vm.symList;
  while(!o.isNil()) {
    Value first = cons_first(vm, o);
    o = cons_rest(vm, o);
    const char* sym = symbol_name(first);
    size_t len = strlen(sym);
    if(len == length) {
      if(memcmp(begin, sym, length) == 0) {
        return first;
      }
    }
  }
  o = new(vm) Object(Object::Type::Symbol);
  o->as_symbol.name = strndup(begin, length);
  vm.symList = vm.Cons(o, vm.symList);
  return o;
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

bool bool_value(Value o) {
  EXPECT(o.isBool());
  return o->as_bool.value;
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
    return b.isNil();
  case Object::Type::Cons:
    if(obj_equal(a->as_cons.first, a->as_cons.first)) {
      if(obj_equal(a->as_cons.rest, b->as_cons.rest)) {
        return true;
      }
    }
    return false;
  case Object::Type::String:
    return strcmp(string_value(a), string_value(b)) == 0;
  case Object::Type::Integer:
    return a->as_integer.value == b->as_integer.value;
  case Object::Type::Symbol:
  case Object::Type::Builtin:
    return a == b;
  case Object::Type::Bool:
    return bool_value(a) == bool_value(b);
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
      vm.Cons(vm.Cons(key, value), env));
  } else {
    EXPECT(args.isNil());
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

    envptr->as_cons.first = vm.Cons(name, lambda).getObj();
    envptr = cons_rest(vm, envptr);

    lambdas = cons_rest(vm, lambdas);
  }

  ASSERT(orig_env == envptr);

  return env;
}

Value eval(VM& vm, Value o, Map env);
Value eval_list(VM& vm, Value o, Map env) {
  if(o.isCons()) {
    return vm.Cons(eval(vm, cons_first(vm, o), env), eval_list(vm, cons_rest(vm, o), env));
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
        if(bool_value(c)) {
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
          EvalFrame builtinFrame(vm, vm.Cons(f, params), env);
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

const char* skip_ws(const char* text) {
  while(true) {
    while(*text == ' ' || *text == '\n' || *text == '\t') {
      text++;
    }
    if(*text == ';') {
      text++;
      while(*text != '\n' && *text != '\0') {
        text++;
      }
    } else {
      return text;
    }
  }
}

bool is_digit(int ch) {
  return ch >= '0' && ch <= '9';
}

bool is_sym_char(int ch) {
  return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
    ch == '?' || ch == '+' || ch == '-' || ch == '/' || ch == '*';
}

static Value parse_value(VM& vm, const char** text);

static Value parse_list(VM& vm, const char** text) {
  *text = skip_ws(*text);
  if(**text == ')') {
    (*text)++;
    return vm.nil;
  } else {
    Value first = parse_value(vm, text);
    Value rest = parse_list(vm, text);
    return vm.Cons(first, rest);
  }
}

static Value parse_string(VM& vm, const char** text) {
  StringBuffer buf;
  (*text)++;
  bool escape = false;
  while(true) {
    char ch = **text;
    (*text)++;
    if(escape) {
      escape = false;
      switch(ch) {
      case '\0':
        fprintf(stderr, "unterminated string\n");
        EXPECT(0);
        return 0;
      case '\\':
      case '"':
        buf.append(ch);
        break;
      case 'n':
        buf.append('\n');
        break;
      default:
        fprintf(stderr, "bad escape: \\%c\n", ch);
        EXPECT(0);
        return 0;
      }
    } else {
      switch(ch) {
      case '\0':
        fprintf(stderr, "unterminated string\n");
        EXPECT(0);
        return 0;
      case '\\':
        escape = true;
        break;
      case '"':
        return make_string(vm, buf.str());
      default:
        buf.append(ch);
        break;
      }
    }
  }
}

static Value parse_value(VM& vm, const char** text) {
  *text = skip_ws(*text);
  if(**text == '(') {
    (*text)++;
    return parse_list(vm, text);
  } else if(is_digit(**text)) {
    int value = 0;
    while(is_digit(**text)) {
      value *= 10;
      value += **text - '0';
      (*text)++;
    }
    return vm.Integer(value);
  } else if(**text == '#') {
    (*text)++;
    switch(*((*text)++)) {
    case 't':
      return vm.true_;
    case 'f':
      return vm.false_;
    default:
      EXPECT(0);
      return 0;
    }
  } else if(is_sym_char(**text)) {
    const char* start = *text;
    while(is_sym_char(**text)) {
      (*text)++;
    }
    return make_symbol_with_length(vm, start, *text - start);
  } else if(**text == '"') {
    return parse_string(vm, text);
  } else {
    fprintf(stderr, "problem near [[[%s]]]\n", *text);
    EXPECT(0);
    return 0;
  }
}

Value parse(VM& vm, const char* text) {
  Value result = parse_value(vm, &text);
  text = skip_ws(text);
  if(*text != '\0') {
    fprintf(stderr, "expected eof at [[[%s]]]\n", text);
    EXPECT(0);
  }
  return result;
}

Value parse_multi(VM& vm, const char* text) {
  Value nil = vm.nil;
  Value result = nil;
  Object** ptr = &result.getObj();
  text = skip_ws(text);
  while(*text != '\0') {
    Value o = parse_value(vm, &text);
    // result = vm.Cons(o, result);
    *ptr = vm.Cons(o, nil).getObj();
    ptr = &(*ptr)->as_cons.rest;
    text = skip_ws(text);
  }
  return result;
}

void testMakeList() {
  VM vm;

  Value one = vm.Integer(1);
  Value two = vm.Integer(2);
  Value three = vm.Integer(3);

  {
    Value a = vm.List();
    Value b = vm.nil;
    EXPECT(obj_equal(a, b));
  }

  {
    Value a = vm.List(one);
    Value b = vm.Cons(one, vm.nil);
    EXPECT(obj_equal(a, b));
  }

  {
    Value a = vm.List(one, two);
    Value b = vm.Cons(one, vm.Cons(two, vm.nil));
    EXPECT(obj_equal(a, b));
  }

  {
    Value a = vm.List(one, two, three);
    Value b = vm.Cons(one, vm.Cons(two, vm.Cons(three, vm.nil)));
    EXPECT(obj_equal(a, b));
  }

}

void testParse() {
  VM vm;

  {
    EXPECT_INT_EQ(integer_value(vm, parse(vm, "1")), 1);
    EXPECT_INT_EQ(integer_value(vm, parse(vm, "42")), 42);
  }

  {
    EXPECT(bool_value(parse(vm, "#t")) == true);
    EXPECT(bool_value(parse(vm, "#f")) == false);
  }

  {
    Value res = parse(vm, "a");
    EXPECT(res.isSymbol());
  }

  {
    Value res = parse(vm, "\"a\"");
    EXPECT(obj_equal(res, make_string(vm, "a")));
  }

  {
    Value res = parse(vm, "(a b)");
    Value a = vm.Symbol("a");
    Value b = vm.Symbol("b");
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
  Value one = vm.Integer(1);
  Value two = vm.Integer(2);

  {
    EXPECT_INT_EQ(integer_value(vm, eval(vm, vm.Integer(1), vm.nil)), 1);
    EXPECT_INT_EQ(integer_value(vm, eval(vm, vm.Integer(42), vm.nil)), 42);
  }

  {
    Value list = vm.List(add, one, two);

    EXPECT_INT_EQ(integer_value(vm, eval(vm, list, vm.nil)), 3);
  }

  {
    Value a = vm.Symbol("a");
    Value pair = vm.Cons(a, vm.Integer(42));
    Value env = vm.Cons(pair, vm.nil);
    EXPECT_INT_EQ(integer_value(vm, eval(vm, a, env)), 42);
  }

  {
    Value list = vm.List(plus, one, two);

    Value pair = vm.Cons(plus, add);
    Value env = vm.Cons(pair, vm.nil);
    EXPECT_INT_EQ(integer_value(vm, eval(vm, list, env)), 3);
  }

  {
    Value list = vm.List(_if, _true, one, two);
    EXPECT_INT_EQ(integer_value(vm, eval(vm, list, vm.nil)), 1);
  }

  {
    Value list = vm.List(_if, _false, one, two);
    EXPECT_INT_EQ(integer_value(vm, eval(vm, list, vm.nil)), 2);
  }

}

void testParseAndEval() {
  VM vm;

  {
    Value plus = vm.syms.add;
    Value add = vm.objs.builtin_add;

    Value pair = vm.Cons(plus, add);
    Value env = vm.Cons(pair, vm.nil);

    Value input = parse(vm, "(+ 1 2)");
    Value res = eval(vm, input, env);

    EXPECT_INT_EQ(integer_value(vm, res), 3);
  }

  {
    Value scons = vm.syms.Cons;
    Value cons = vm.objs.builtin_cons;

    Value env = vm.List(vm.Cons(scons, cons));

    Value input = parse(vm, "((letlambdas ( ((myfunc x y) (cons x y)) ) myfunc) 1 2)");
    Value res = eval(vm, input, env);

    EXPECT(obj_equal(res, vm.Cons(vm.Integer(1), vm.Integer(2))));
  }

  {
    Value input = parse(vm, "((import core +) 1 2)");
    Value res = eval(vm, input, vm.nil);

    EXPECT_INT_EQ(integer_value(vm, res), 3);
  }

}

void testSerialize() {
  VM vm;

  {
    Value original = vm.nil;
    Data serialized = serialize(original);
    Value deserialized = deserialize(vm, serialized.data);
    EXPECT(obj_equal(deserialized, original));
  }

  {
    Value original = vm.Integer(0);
    Data serialized = serialize(original);
    Value deserialized = deserialize(vm, serialized.data);
    EXPECT(obj_equal(deserialized, original));
  }

  {
    Value original = vm.Integer(1);
    Data serialized = serialize(original);
    Value deserialized = deserialize(vm, serialized.data);
    EXPECT(obj_equal(deserialized, original));
  }

  {
    Value a = vm.Symbol("some-random-symbol");
    Value original = vm.List(a,
      vm.Integer(1),
      vm.Integer(42),
      vm.Integer(129),
      vm.Integer(4),
      vm.Integer(5));
    Data serialized = serialize(original);
    Value deserialized = deserialize(vm, serialized.data);
    EXPECT(obj_equal(deserialized, original));
  }
}

void testAll() {
  testMakeList();
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

void saveBytes(const char* file, Data data) {
  FILE* f = fopen(file, "wb");
  fwrite(data.data, 1, data.length, f);
  fclose(f);
}

Value parse_file(VM& vm, const char* file, bool multiexpr) {
  char* buf = loadBytes(file);

  Value obj;
  if(multiexpr) {
    obj = parse_multi(vm, buf);
  } else {
    obj = parse(vm, buf);
  }

  free(buf);

  return obj;
}

Value run_file(VM& vm, const char* file, bool multiexpr) {
  Value obj = parse_file(vm, file, multiexpr);
  return eval(vm, obj, vm.nil);
}

Value run_transform_file(VM& vm, const char* file) {
  return vm.transform(parse_file(vm, file, true));
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
          Data data = serialize(transformed);
          saveBytes(serialize_to, data);
        } else {
          vm.print(transformed);
        }
        return 0;
      }
    } else if(serialize_to) {
      Value value = parse_file(vm, file, false);
      Data data = serialize(value);
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

