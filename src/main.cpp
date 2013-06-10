#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

#include "serialize.h"
#include "vm.h"
#include "transform.h"

void _assert_failed(const char* file, int line, const char* message, ...) {
  fprintf(stderr, "assertion failure, %s:%d:\n  ", file, line);

  va_list ap;
  va_start(ap, message);
  vprintf(message, ap);
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

Value make_symbol(VM& vm, const char* name);
Value make_builtin(VM& vm, const char* name, BuiltinFunc func);
Value make_cons(VM& vm, Value first, Value rest);

Value make_list(VM& vm);

template<class T, class... TS>
Value make_list(VM& vm, T t, TS... ts);

void obj_print(Value value, int indent = 0, FILE* stream = stdout);


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

void EvalFrame::dump(FILE* stream) {
  static const char prefix[] = "evaluating ";
  fprintf(stream, prefix);
  obj_print(evaluating, strlen(prefix) / 2, stream);
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
      int len = fprintf(stream, "    where %s = ", name);
      obj_print(value, len / 2, stderr);
    }
  }
  if(previous) {
    previous->dump(stream);
  }
}

void debug_print(Value value);

Value make_cons(VM& vm, Value a, Value b) {
  Value o = new(vm) Object(Object::Type::Cons);
  o->as_cons.first = a.getObj();
  o->as_cons.rest = b.getObj();
  return o;
}

Value make_list(VM& vm) {
  return vm.nil;
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
    list = make_cons(vm, obj, list);
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

Value make_integer(VM& vm, int value) {
  Value o = new(vm) Object(Object::Type::Integer);
  o->as_integer.value = value;
  return o;
}

int integer_value(Value o) {
  EXPECT(o.isInteger());
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
  vm.symList = make_cons(vm, o, vm.symList);
  return o;
}

Value make_symbol(VM& vm, const char* name) {
  return make_symbol_with_length(vm, name, strlen(name));
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
    return integer_value(a) == integer_value(b);
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

bool print_value(Value value, int indent, bool list_on_newline, FILE* stream);
bool print_list(Value value, int indent, bool list_on_newline, FILE* stream) {
  switch(value->type) {
  case Object::Type::Nil:
    fprintf(stream, ")");
    return false;
  case Object::Type::Cons:
    fprintf(stream, " ");
    print_value(value->as_cons.first, indent, true, stream);
    print_list(value->as_cons.rest, indent, true, stream);
    return true;
  default:
    fprintf(stream, " . ");
    print_value(value, indent, true, stream);
    fprintf(stream, ")");
    return true;
  }
}

bool print_value(Value value, int indent, bool list_on_newline, FILE* stream) {
  switch(value->type) {
  case Object::Type::Nil:
    fprintf(stream, "()");
    return false;
  case Object::Type::Cons:
    if(list_on_newline) {
      fprintf(stream, "\n%*s", indent * 2, "");
    }
    fprintf(stream, "(");
    print_value(value->as_cons.first, indent + 1, false, stream);
    return print_list(value->as_cons.rest, indent + 1, true, stream);
  case Object::Type::String:
    fprintf(stream, "\"%s\"", string_value(value));
    return false;
  case Object::Type::Integer:
    fprintf(stream, "%d", integer_value(value));
    return false;
  case Object::Type::Symbol:
    fprintf(stream, "%s", symbol_name(value));
    return false;
  case Object::Type::Builtin:
    fprintf(stream, "<builtin:%s>", builtin_name(value));
    return false;
  case Object::Type::Bool:
    fprintf(stream, "%s", bool_value(value) ? "#t" : "#f");
    return false;
  case Object::Type::Lambda:
    fprintf(stream, "<lambda@%p>", value.getObj());
    return false;
  default:
    EXPECT(0);
    return false;
  }
}

void obj_print(Value value, int indent, FILE* stream) {
  print_value(value, indent, false, stream);
  fprintf(stream, "\n");
}

void debug_print(Value value) {
  obj_print(value, 0, stderr);
}

Value builtin_add(VM& vm, Value args) {
  int res = 0;
  while(!args.isNil()) {
    Value a = cons_first(vm, args);
    args = cons_rest(vm, args);
    res += integer_value(a);
  }
  return make_integer(vm, res);
}

Value builtin_sub(VM& vm, Value args) {
  int res;
  if(!args.isNil()) {
    Value a = cons_first(vm, args);
    args = cons_rest(vm, args);
    if(args.isNil()) {
      return make_integer(vm, -integer_value(a));
    }
    res = integer_value(a);
  }
  while(!args.isNil()) {
    Value a = cons_first(vm, args);
    args = cons_rest(vm, args);
    res -= integer_value(a);
  }
  return make_integer(vm, res);
}

Value builtin_mul(VM& vm, Value args) {
  int res = 1;
  while(!args.isNil()) {
    Value a = cons_first(vm, args);
    args = cons_rest(vm, args);
    res *= integer_value(a);
  }
  return make_integer(vm, res);
}

Value builtin_div(VM& vm, Value args) {
  EXPECT(!args.isNil());
  int res = integer_value(cons_first(vm, args));
  args = cons_rest(vm, args);
  while(!args.isNil()) {
    Value a = cons_first(vm, args);
    args = cons_rest(vm, args);
    res /= integer_value(a);
  }
  return make_integer(vm, res);
}

Value builtin_modulo(VM& vm, Value args) {
  Value a = cons_first(vm, args);
  args = cons_rest(vm, args);
  Value b = cons_first(vm, args);
  VM_EXPECT(vm, cons_rest(vm, args).isNil());
  return make_integer(vm,
    integer_value(a) % integer_value(b));
}

Value builtin_is_cons(VM& vm, Value args) {
  Value arg = cons_first(vm, args);
  VM_EXPECT(vm, cons_rest(vm, args).isNil());
  return arg.isCons() ? vm.true_ : vm.false_;
}

Value builtin_cons(VM& vm, Value args) {
  Value first = cons_first(vm, args);
  args = cons_rest(vm, args);
  Value rest = cons_first(vm, args);
  VM_EXPECT(vm, cons_rest(vm, args).isNil());
  return make_cons(vm, first, rest);
}

Value builtin_first(VM& vm, Value args) {
  Value arg = cons_first(vm, args);
  VM_EXPECT(vm, cons_rest(vm, args).isNil());
  return cons_first(vm, arg);
}

Value builtin_rest(VM& vm, Value args) {
  Value arg = cons_first(vm, args);
  VM_EXPECT(vm, cons_rest(vm, args).isNil());
  return cons_rest(vm, arg);
}

Value builtin_is_symbol(VM& vm, Value args) {
  Value arg = cons_first(vm, args);
  VM_EXPECT(vm, cons_rest(vm, args).isNil());
  return arg.isSymbol() ? vm.true_ : vm.false_;
}

Value builtin_is_equal(VM& vm, Value args) {
  Value a = cons_first(vm, args);
  args = cons_rest(vm, args);
  Value b = cons_first(vm, args);
  VM_EXPECT(vm, cons_rest(vm, args).isNil());
  return obj_equal(a, b) ? vm.true_ : vm.false_;
}

Value builtin_is_nil(VM& vm, Value args) {
  Value arg = cons_first(vm, args);
  VM_EXPECT(vm, cons_rest(vm, args).isNil());
  return arg.isNil() ? vm.true_ : vm.false_;
}

Value builtin_is_int(VM& vm, Value args) {
  Value arg = cons_first(vm, args);
  VM_EXPECT(vm, cons_rest(vm, args).isNil());
  return arg.isInteger() ? vm.true_ : vm.false_;
}

Value builtin_concat(VM& vm, Value args) {
  StringBuffer buf;
  while(!args.isNil()) {
    Value a = cons_first(vm, args);
    args = cons_rest(vm, args);
    buf.append(string_value(a));
  }
  return make_string(vm, buf.str());
}

Value builtin_symbol_name(VM& vm, Value args) {
  Value arg = cons_first(vm, args);
  VM_EXPECT(vm, cons_rest(vm, args).isNil());
  return make_string(vm, symbol_name(arg));
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
      make_cons(vm, make_cons(vm, key, value), env));
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
    EXPECT(cons_rest(vm, lambda).isNil());
    Value body = cons_first(vm, lambda);

    Value name = cons_first(vm, name_and_params);
    EXPECT(name.isSymbol());
    Value params = cons_rest(vm, name_and_params);

    lambda = make_lambda(vm, params, body, env);

    envptr->as_cons.first = make_cons(vm, name, lambda).getObj();
    envptr = cons_rest(vm, envptr);

    lambdas = cons_rest(vm, lambdas);
  }

  ASSERT(orig_env == envptr);

  return env;
}

Value eval(VM& vm, Value o, Map env);
Value eval_list(VM& vm, Value o, Map env) {
  if(o.isCons()) {
    return make_cons(vm, eval(vm, cons_first(vm, o), env), eval_list(vm, cons_rest(vm, o), env));
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
          EvalFrame builtinFrame(vm, make_cons(vm, f, params), env);
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
  return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '?' || ch == '+' || ch == '-' || ch == '/' || ch == '*';
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
    return make_cons(vm, first, rest);
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
    return make_integer(vm, value);
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
    // result = make_cons(vm, o, result);
    *ptr = make_cons(vm, o, nil).getObj();
    ptr = &(*ptr)->as_cons.rest;
    text = skip_ws(text);
  }
  return result;
}

void testMakeList() {
  VM vm;

  Value one = make_integer(vm, 1);
  Value two = make_integer(vm, 2);
  Value three = make_integer(vm, 3);

  {
    Value a = make_list(vm);
    Value b = vm.nil;
    EXPECT(obj_equal(a, b));
  }

  {
    Value a = make_list(vm, one);
    Value b = make_cons(vm, one, vm.nil);
    EXPECT(obj_equal(a, b));
  }

  {
    Value a = make_list(vm, one, two);
    Value b = make_cons(vm, one, make_cons(vm, two, vm.nil));
    EXPECT(obj_equal(a, b));
  }

  {
    Value a = make_list(vm, one, two, three);
    Value b = make_cons(vm, one, make_cons(vm, two, make_cons(vm, three, vm.nil)));
    EXPECT(obj_equal(a, b));
  }

}

void testParse() {
  VM vm;

  {
    EXPECT_INT_EQ(integer_value(parse(vm, "1")), 1);
    EXPECT_INT_EQ(integer_value(parse(vm, "42")), 42);
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
    Value a = make_symbol(vm, "a");
    Value b = make_symbol(vm, "b");
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
  Value one = make_integer(vm, 1);
  Value two = make_integer(vm, 2);

  {
    EXPECT_INT_EQ(integer_value(eval(vm, make_integer(vm, 1), vm.nil)), 1);
    EXPECT_INT_EQ(integer_value(eval(vm, make_integer(vm, 42), vm.nil)), 42);
  }

  {
    Value list = make_list(vm, add, one, two);

    EXPECT_INT_EQ(integer_value(eval(vm, list, vm.nil)), 3);
  }

  {
    Value a = make_symbol(vm, "a");
    Value pair = make_cons(vm, a, make_integer(vm, 42));
    Value env = make_cons(vm, pair, vm.nil);
    EXPECT_INT_EQ(integer_value(eval(vm, a, env)), 42);
  }

  {
    Value list = make_list(vm, plus, one, two);

    Value pair = make_cons(vm, plus, add);
    Value env = make_cons(vm, pair, vm.nil);
    EXPECT_INT_EQ(integer_value(eval(vm, list, env)), 3);
  }

  {
    Value list = make_list(vm, _if, _true, one, two);
    EXPECT_INT_EQ(integer_value(eval(vm, list, vm.nil)), 1);
  }

  {
    Value list = make_list(vm, _if, _false, one, two);
    EXPECT_INT_EQ(integer_value(eval(vm, list, vm.nil)), 2);
  }

}

void testParseAndEval() {
  VM vm;

  {
    Value plus = vm.syms.add;
    Value add = vm.objs.builtin_add;

    Value pair = make_cons(vm, plus, add);
    Value env = make_cons(vm, pair, vm.nil);

    Value input = parse(vm, "(+ 1 2)");
    Value res = eval(vm, input, env);

    EXPECT_INT_EQ(integer_value(res), 3);
  }

  {
    Value scons = vm.syms.cons;
    Value cons = vm.objs.builtin_cons;

    Value env = make_list(vm, make_cons(vm, scons, cons));

    Value input = parse(vm, "((letlambdas ( ((myfunc x y) (cons x y)) ) myfunc) 1 2)");
    Value res = eval(vm, input, env);

    EXPECT(obj_equal(res, make_cons(vm, make_integer(vm, 1), make_integer(vm, 2))));
  }

  {
    Value input = parse(vm, "((import core +) 1 2)");
    Value res = eval(vm, input, vm.nil);

    EXPECT_INT_EQ(integer_value(res), 3);
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
    Value original = make_integer(vm, 0);
    Data serialized = serialize(original);
    Value deserialized = deserialize(vm, serialized.data);
    EXPECT(obj_equal(deserialized, original));
  }

  {
    Value original = make_integer(vm, 1);
    Data serialized = serialize(original);
    Value deserialized = deserialize(vm, serialized.data);
    EXPECT(obj_equal(deserialized, original));
  }

  {
    Value a = make_symbol(vm, "some-random-symbol");
    Value original = make_list(vm, a,
      make_integer(vm, 1),
      make_integer(vm, 42),
      make_integer(vm, 129),
      make_integer(vm, 4),
      make_integer(vm, 5));
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

  Transformer t(vm);

  // obj_print(transformer, 0, stderr);
  // obj_print(t.data, 0, stderr);
  // EXPECT(obj_equal(transformer, t.data));

  return t.transform(parse_file(vm, file, true));
  // Value input = parse_file(vm, file, true);

  // Value quoted_input = make_list(vm, vm.syms.quote, input);
  // Value transformed = eval(vm, make_list(vm, transformer, quoted_input), vm.nil);
  // return transformed;
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
        VM vm;
        Value transformed = run_transform_file(vm, transform_file);
        if(serialize_to) {
          Data data = serialize(transformed);
          saveBytes(serialize_to, data);
        } else {
          obj_print(transformed);
        }
        return 0;
      }
    } else if(serialize_to) {
      VM vm;
      Value value = parse_file(vm, file, false);
      Data data = serialize(value);
      saveBytes(serialize_to, data);
      return 0;
    } else if(deserialize_from) {
      const char* data = loadBytes(deserialize_from);
      VM vm;
      Value value = deserialize(vm, data);
      obj_print(value);
      return 0;
    } else if(file) {
      VM vm;
      Value transformed = run_transform_file(vm, file);
      Value result = eval(vm, transformed, vm.nil);
      obj_print(result);
      return 0;
    } else {
      fprintf(stderr, "must provide either --transform-file or file to run\n");
    }
  }
  return 1;
}

