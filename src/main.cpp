#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

void _assert_failed(const char* file, int line, const char* message, ...);

#define EXPECT(e) do { if(!(e)) {_assert_failed(__FILE__, __LINE__, "%s", #e);} } while(0)
#define EXPECT_MSG(e, msg, ...) \
  do { if(!(e)) {_assert_failed(__FILE__, __LINE__, "%s\n  " msg, #e, ##__VA_ARGS__);} } while(0)

#define EXPECT_INT_EQ(expected, actual) \
  EXPECT_MSG((expected) == (actual), "expected: %d, actual: %d", expected, actual);

#define ENABLE_ASSERTS
#define ENABLE_DUHS

#ifdef ENABLE_ASSERTS
#define ASSERT(...) EXPECT(__VA_ARGS__)
#define ASSERT_MSG(...) EXPECT_MSG(__VA_ARGS__)
#else
#define ASSERT(...)
#define ASSERT_MSG(...)
#endif

#ifdef ENABLE_DUHS
#define DUH(...) EXPECT(__VA_ARGS__)
#define DUH_MSG(...) EXPECT_MSG(__VA_ARGS__)
#else
#define DUH(...)
#define DUH_MSG(...)
#endif

#define VM_EXPECT(vm, expected) \
  do { \
    if(!expected) { \
      vm.errorOccurred(__FILE__, __LINE__, #expected); \
    } \
  } while(0) \

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

class VM;

class Object;

class Value {
private:
  Object* obj;

public:
  Value(): obj(0) {}
  Value(Object* obj): obj(obj) {if(!obj) exit(1);}

  inline explicit operator Object* () { return obj; }
  inline Object* operator -> () { return obj; }
  inline bool isNil();
  inline operator bool () { return !isNil(); }
  inline bool operator == (const Value& other) { return obj == other.obj; }
  inline bool operator != (const Value& other) { return obj != other.obj; }
};

typedef Value (*BuiltinFunc)(VM& vm, Value args);

class Object {
public:
  enum class Type {
    Nil,
    Cons,
    String,
    Integer,
    Symbol,
    Builtin,
    Bool,
    Lambda
  };

  Type type;

  union {
    struct {
      Value first;
      Value rest;
    } as_cons;
    struct {
      const char* value;
    } as_string;
    struct {
      int value;
    } as_integer;
    struct {
      const char* name;
    } as_symbol;
    struct {
      BuiltinFunc func;
    } as_builtin;
    struct {
      bool value;
    } as_bool;
    struct {
      Value params;
      Value body;
      Value env;
    } as_lambda;
  };

  Object(Type type): type(type) {}

  inline void* operator new (size_t size, VM& vm);
};

bool Value::isNil() { return obj->type == Object::Type::Nil; }

typedef struct _heap_block_t {
  struct _heap_block_t* next;
  size_t capacity;
  size_t used;
  uint8_t* data;
} heap_block_t;

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

size_t max_sizet(size_t a, size_t b) {
  return a > b ? a : b;
}

Value make_nil(VM& vm);
Value make_bool(VM& vm, bool value);
Value make_symbol(VM& vm, const char* name);
Value make_builtin(VM& vm, BuiltinFunc func);
Value make_cons(VM& vm, Value first, Value rest);

Value builtin_add(VM& vm, Value args);
Value builtin_is_cons(VM& vm, Value args);
Value builtin_cons(VM& vm, Value args);
Value builtin_first(VM& vm, Value args);
Value builtin_rest(VM& vm, Value args);
Value builtin_is_symbol(VM& vm, Value args);
Value builtin_is_equal(VM& vm, Value args);
Value builtin_is_nil(VM& vm, Value args);

Value make_list(VM& vm);

template<class T, class... TS>
Value make_list(VM& vm, T t, TS... ts);

void obj_print(Value value, int indent = 0, FILE* stream = stdout);

class VM;

bool is_cons(Value o);
bool is_symbol(Value o);

class EvalFrame {
public:
  VM& vm;
  Value evaluating;
  Value env;
  EvalFrame* previous;

  EvalFrame(VM& vm, Value evaluating, Value env);

  ~EvalFrame();

  void dump(FILE* stream);
};

class VM {
public:
  heap_block_t* heap;
  size_t heap_block_size;

  struct {
    Value nil;

    Value bool_true;
    Value bool_false;

    Value sym_if;
    Value sym_letlambdas;
    Value sym_import;
    Value sym_core;
    Value sym_quote;

    Value sym_plus;
    Value sym_is_cons;
    Value sym_cons;
    Value sym_first;
    Value sym_rest;
    Value sym_is_symbol;
    Value sym_is_equal;
    Value sym_is_nil;

    Value builtin_add;
    Value builtin_is_cons;
    Value builtin_cons;
    Value builtin_first;
    Value builtin_rest;
    Value builtin_is_symbol;
    Value builtin_is_equal;
    Value builtin_is_nil;
  } objs;

  Value syms;

  Value core_imports;

  EvalFrame* currentEvalFrame = 0;

  VM(size_t heap_block_size = 4096) {
    VM& vm = *this;
    this->heap_block_size = heap_block_size;
    heap = make_heap_block(heap_block_size, 0);

    syms = make_nil(vm);

    objs.nil = make_nil(vm);

    objs.bool_true = make_bool(vm, true);
    objs.bool_false = make_bool(vm, false);

    objs.sym_if = make_symbol(vm, "if");
    objs.sym_letlambdas = make_symbol(vm, "letlambdas");
    objs.sym_import = make_symbol(vm, "import");
    objs.sym_core = make_symbol(vm, "core");
    objs.sym_quote = make_symbol(vm, "quote");

    objs.sym_plus = make_symbol(vm, "+");
    objs.sym_is_cons = make_symbol(vm, "cons?");
    objs.sym_cons = make_symbol(vm, "cons");
    objs.sym_first = make_symbol(vm, "first");
    objs.sym_rest = make_symbol(vm, "rest");
    objs.sym_is_symbol = make_symbol(vm, "sym?");
    objs.sym_is_equal = make_symbol(vm, "eq?");
    objs.sym_is_nil = make_symbol(vm, "nil?");

    objs.builtin_add = make_builtin(vm, builtin_add);
    objs.builtin_is_cons = make_builtin(vm, builtin_is_cons);
    objs.builtin_cons = make_builtin(vm, builtin_cons);
    objs.builtin_first = make_builtin(vm, builtin_first);
    objs.builtin_rest = make_builtin(vm, builtin_rest);
    objs.builtin_is_symbol = make_builtin(vm, builtin_is_symbol);
    objs.builtin_is_equal = make_builtin(vm, builtin_is_equal);
    objs.builtin_is_nil = make_builtin(vm, builtin_is_nil);

    core_imports = make_list(vm,
      make_cons(vm, objs.sym_plus, objs.builtin_add),
      make_cons(vm, objs.sym_is_cons, objs.builtin_is_cons),
      make_cons(vm, objs.sym_cons, objs.builtin_cons),
      make_cons(vm, objs.sym_first, objs.builtin_first),
      make_cons(vm, objs.sym_rest, objs.builtin_rest),
      make_cons(vm, objs.sym_is_symbol, objs.builtin_is_symbol),
      make_cons(vm, objs.sym_is_equal, objs.builtin_is_equal),
      make_cons(vm, objs.sym_is_nil, objs.builtin_is_nil));
  }

  ~VM() {
    free_heap_block(heap);
  }

  void* alloc(size_t size) {
    if(size > heap->capacity - heap->used) {
      heap = make_heap_block(max_sizet(heap_block_size, size * 2 + sizeof(heap_block_t)), heap);
    }
    void* ret = heap->data + heap->used;
    heap->used += size;
    return ret;
  }

  void errorOccurred(const char* file, int line, const char* message) {
    fprintf(stderr, "error occurred: %s:%d: %s\n", file, line, message);
    currentEvalFrame->dump(stderr);
    exit(1);
  }
};

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
  Value end = previous ? previous->env : vm.objs.nil;
  while(env && env != end) {
    ASSERT(is_cons(env));
    Value pair = env->as_cons.first;
    env = env->as_cons.rest;
    ASSERT(is_cons(pair));
    Value key = pair->as_cons.first;

    if(obj_mentions_symbol(evaluating, key)) {
      Value value = pair->as_cons.rest;
      ASSERT(is_symbol(key));
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

Value make_nil(VM& vm) {
  return new(vm) Object(Object::Type::Nil);
}

bool is_cons(Value o) {
  return o->type == Object::Type::Cons;
}

Value make_cons(VM& vm, Value a, Value b) {
  Value o = new(vm) Object(Object::Type::Cons);
  o->as_cons.first = a;
  o->as_cons.rest = b;
  return o;
}

Value make_list(VM& vm) {
  return vm.objs.nil;
}

template<class T, class... TS>
Value make_list(VM& vm, T t, TS... ts) {
  return make_cons(vm, t, make_list(vm, ts...));
}

Value cons_first(VM& vm, Value o) {
  VM_EXPECT(vm, is_cons(o));
  return o->as_cons.first;
}

Value cons_rest(Value o) {
  EXPECT(is_cons(o));
  return o->as_cons.rest;
}

size_t list_length(Value list) {
  size_t ret = 0;
  while(list) {
    ret++;
    list = cons_rest(list);
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

bool is_string(Value o) {
  return o->type == Object::Type::String;
}

Value make_string(VM& vm, const char* value) {
  Value o = new(vm) Object(Object::Type::String);
  o->as_string.value = value;
  return o;
}

const char* string_value(Value o) {
  EXPECT(is_string(o));
  return o->as_string.value;
}

bool is_integer(Value o) {
  return o->type == Object::Type::Integer;
}

Value make_integer(VM& vm, int value) {
  Value o = new(vm) Object(Object::Type::Integer);
  o->as_integer.value = value;
  return o;
}

int integer_value(Value o) {
  EXPECT(is_integer(o));
  return o->as_integer.value;
}

bool is_symbol(Value o) {
  return o->type == Object::Type::Symbol;
}

const char* symbol_name(Value o) {
  EXPECT(is_symbol(o));
  return o->as_symbol.name;
}

typedef Value Map;

Value map_lookup(VM& vm, Map map, Value key) {
  DEBUG_LOG(Map orig = map);
  while(map) {
    Value item = cons_first(vm, map);
    if(cons_first(vm, item) == key) {
      return cons_rest(item);
    } else {
      map = cons_rest(map);
    }
  }
  DEBUG_LOG(fprintf(stderr, "lookup failed for: "); debug_print(key));
  DEBUG_LOG(fprintf(stderr, "  in: "); debug_print(orig));
  EXPECT(0);
  return 0;
}

Value make_symbol_with_length(VM& vm, const char* begin, size_t length) {
  Value o = vm.syms;
  while(o) {
    Value first = cons_first(vm, o);
    o = cons_rest(o);
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
  vm.syms = make_cons(vm, o, vm.syms);
  return o;
}

Value make_symbol(VM& vm, const char* name) {
  return make_symbol_with_length(vm, name, strlen(name));
}

bool is_builtin(Value o) {
  return o->type == Object::Type::Builtin;
}

BuiltinFunc builtin_func(Value o) {
  EXPECT(is_builtin(o));
  return o->as_builtin.func;
}

Value make_builtin(VM& vm, BuiltinFunc func) {
  Value o = new(vm) Object(Object::Type::Builtin);
  o->as_builtin.func = func;
  return o;
}

bool is_bool(Value o) {
  return o->type == Object::Type::Bool;
}

bool bool_value(Value o) {
  EXPECT(is_bool(o));
  return o->as_bool.value;
}

Value make_bool(VM& vm, bool value) {
  Value o = new(vm) Object(Object::Type::Bool);
  o->as_bool.value = value;
  return o;
}

bool is_lambda(Value o) {
  return o->type == Object::Type::Lambda;
}

Value lambda_params(Value o) {
  EXPECT(is_lambda(o));
  return o->as_lambda.params;
}

Value lambda_body(Value o) {
  EXPECT(is_lambda(o));
  return o->as_lambda.body;
}

Value lambda_env(Value o) {
  EXPECT(is_lambda(o));
  return o->as_lambda.env;
}

Value make_lambda(VM& vm, Value params, Value body, Value env) {
  Value o = new(vm) Object(Object::Type::Lambda);
  o->as_lambda.params = params;
  o->as_lambda.body = body;
  o->as_lambda.env = env;
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
      if(obj_equal(cons_rest(a), cons_rest(b))) {
        return true;
      }
    }
    return false;
  case Object::Type::String:
    return strcmp(string_value(a), string_value(b)) == 0;
  case Object::Type::Integer:
    return integer_value(a) == integer_value(b);
  case Object::Type::Symbol:
    return a == b;
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
  switch(obj->type) {
  case Object::Type::Cons:
    return
      obj_mentions_symbol(obj->as_cons.first, symbol) ||
      obj_mentions_symbol(obj->as_cons.rest, symbol);
  case Object::Type::Symbol:
    return obj == symbol;
  case Object::Type::Nil:
  case Object::Type::String:
  case Object::Type::Integer:
  case Object::Type::Builtin:
  case Object::Type::Bool:
    return false;
  default:
    EXPECT(0);
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
    print_list(cons_rest(value), indent, true, stream);
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
    return print_list(cons_rest(value), indent + 1, true, stream);
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
    fprintf(stream, "<builtin@%p>", builtin_func(value));
    return false;
  case Object::Type::Bool:
    fprintf(stream, "%s", bool_value(value) ? "#t" : "#f");
    return false;
  case Object::Type::Lambda:
    fprintf(stream, "<lambda@%p>", (Object*)value);
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
  while(args) {
    Value a = cons_first(vm, args);
    args = cons_rest(args);
    res += integer_value(a);
  }
  return make_integer(vm, res);
}

Value builtin_is_cons(VM& vm, Value args) {
  Value arg = cons_first(vm, args);
  EXPECT(!cons_rest(args));
  return is_cons(arg) ? vm.objs.bool_true : vm.objs.bool_false;
}

Value builtin_cons(VM& vm, Value args) {
  Value first = cons_first(vm, args);
  args = cons_rest(args);
  Value rest = cons_first(vm, args);
  EXPECT(!cons_rest(args));
  return make_cons(vm, first, rest);
}

Value builtin_first(VM& vm, Value args) {
  Value arg = cons_first(vm, args);
  EXPECT(!cons_rest(args));
  return cons_first(vm, arg);
}

Value builtin_rest(VM& vm, Value args) {
  Value arg = cons_first(vm, args);
  EXPECT(!cons_rest(args));
  return cons_rest(arg);
}

Value builtin_is_symbol(VM& vm, Value args) {
  Value arg = cons_first(vm, args);
  EXPECT(!cons_rest(args));
  return is_symbol(arg) ? vm.objs.bool_true : vm.objs.bool_false;
}

Value builtin_is_equal(VM& vm, Value args) {
  Value a = cons_first(vm, args);
  args = cons_rest(args);
  Value b = cons_first(vm, args);
  EXPECT(!cons_rest(args));
  return obj_equal(a, b) ? vm.objs.bool_true : vm.objs.bool_false;
}

Value builtin_is_nil(VM& vm, Value args) {
  Value arg = cons_first(vm, args);
  EXPECT(!cons_rest(args));
  return arg.isNil() ? vm.objs.bool_true : vm.objs.bool_false;
}

bool is_self_evaluating(Value o) {
  return is_integer(o) || o.isNil() || is_builtin(o) || is_bool(o) || is_lambda(o);
}

Value extend_env(VM& vm, Value params, Value args, Value env) {
  if(params) {
    Value key = cons_first(vm, params);
    EXPECT(is_symbol(key));
    Value value = cons_first(vm, args);
    return extend_env(vm,
      cons_rest(params),
      cons_rest(args),
      make_cons(vm, make_cons(vm, key, value), env));
  } else {
    EXPECT(!args);
    return env;
  }
}

Value make_lambdas_env(VM& vm, Value lambdas, Value env) {
  size_t len = list_length(lambdas);
  Value orig_env = env;

  env = list_prepend_n_objs(vm, len, vm.objs.nil, env);

  Value envptr = env;
  while(lambdas) {
    Value lambda = cons_first(vm, lambdas);

    Value name_and_params = cons_first(vm, lambda);
    lambda = cons_rest(lambda);
    EXPECT(!cons_rest(lambda));
    Value body = cons_first(vm, lambda);

    Value name = cons_first(vm, name_and_params);
    DEBUG_LOG(fprintf(stderr, "name: "); debug_print(name));
    EXPECT(is_symbol(name));
    Value params = cons_rest(name_and_params);
    DEBUG_LOG(fprintf(stderr, "params: "); debug_print(params));
    DEBUG_LOG(fprintf(stderr, "body: "); debug_print(body));

    lambda = make_lambda(vm, params, body, env);

    envptr->as_cons.first = make_cons(vm, name, lambda);
    envptr = cons_rest(envptr);

    lambdas = cons_rest(lambdas);
  }

  ASSERT(orig_env == envptr);

  return env;
}

Value eval(VM& vm, Value o, Map env);
Value eval_list(VM& vm, Value o, Map env) {
  if(is_cons(o)) {
    return make_cons(vm, eval(vm, cons_first(vm, o), env), eval_list(vm, cons_rest(o), env));
  } else {
    return eval(vm, o, env);
  }
}

Value eval(VM& vm, Value o, Map env) {
  EvalFrame frame(vm, o, env);
  // Value procedure;
  // Value arguments;
  // Value result;
  static int depth = 0;
  DEBUG_LOG(fprintf(stderr, "%*.seval: ", depth*2, ""); debug_print(o));
  while(true) {
    if(is_self_evaluating(o)) {
      return o;
    } else if(is_symbol(o)) {
      return map_lookup(vm, env, o);
    } else if(is_cons(o)) {

      Value f = cons_first(vm, o);
      o = cons_rest(o);

      DEBUG_LOG(fprintf(stderr, "f: %p %p: ", (Object*)f, (Object*)vm.objs.sym_letlambdas); debug_print(f));

      if(f == vm.objs.sym_if) {
        Value c = cons_first(vm, o);
        depth++;
        c = eval(vm, c, env);
        depth--;
        if(bool_value(c)) {
          o = cons_first(vm, cons_rest(o));
        } else {
          o = cons_first(vm, cons_rest(cons_rest(o)));
        }
      } else if(f == vm.objs.sym_letlambdas) {
        DEBUG_LOG(fprintf(stderr, "found letlambdas: "); debug_print(o));
        Value lambdas = cons_first(vm, o);
        o = cons_rest(o);
        EXPECT(!cons_rest(o));
        o = cons_first(vm, o);
        env = make_lambdas_env(vm, lambdas, env);
      } else if(f == vm.objs.sym_import) {
        Value lib = cons_first(vm, o);
        o = cons_rest(o);
        Value sym = cons_first(vm, o);
        EXPECT(!cons_rest(o));
        EXPECT(lib == vm.objs.sym_core);
        return map_lookup(vm, vm.core_imports, sym);
      } else if(f == vm.objs.sym_quote) {
        Value a = cons_first(vm, o);
        EXPECT(!cons_rest(o));
        return a;
      } else {
        depth++;
        DEBUG_LOG(Value orig = f);
        f = eval(vm, f, env);
        depth--;
        if(is_builtin(f)) {
          Value params = eval_list(vm, o, env);
          DEBUG_LOG(fprintf(stderr, "calling builtin: "); print_value(orig, 0, false, stderr); fprintf(stderr, " : "); debug_print(params));
          Value res = builtin_func(f)(vm, params);
          DEBUG_LOG(fprintf(stderr, "builtin result: "); debug_print(res));
          return res;
        } else if(is_lambda(f)) {
          Value params = eval_list(vm, o, env);
          env = extend_env(vm, lambda_params(f), params, lambda_env(f));
          o = lambda_body(f);
        } else {
          EXPECT(0);
          return 0;
        }
      }
    } else {
      debug_print(o);
      EXPECT(0);
      return 0;
    }
    DEBUG_LOG(fprintf(stderr, "continuing: "); debug_print(o));
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
  return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '?' || ch == '+' || ch == '-';
}

static Value parse_value(VM& vm, const char** text);

static Value parse_list(VM& vm, const char** text) {
  *text = skip_ws(*text);
  if(**text == ')') {
    (*text)++;
    return make_nil(vm);
  } else {
    Value first = parse_value(vm, text);
    Value rest = parse_list(vm, text);
    return make_cons(vm, first, rest);
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
      return vm.objs.bool_true;
    case 'f':
      return vm.objs.bool_false;
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
  Value nil = vm.objs.nil;
  Value result = nil;
  Value* ptr = &result;
  text = skip_ws(text);
  while(*text != '\0') {
    DEBUG_LOG(fprintf(stderr, "parsing at [[[%s]]]\n", text));
    Value o = parse_value(vm, &text);
    // result = make_cons(vm, o, result);
    *ptr = make_cons(vm, o, nil);
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
    Value b = vm.objs.nil;
    EXPECT(obj_equal(a, b));
  }

  {
    Value a = make_list(vm, one);
    Value b = make_cons(vm, one, vm.objs.nil);
    EXPECT(obj_equal(a, b));
  }

  {
    Value a = make_list(vm, one, two);
    Value b = make_cons(vm, one, make_cons(vm, two, vm.objs.nil));
    EXPECT(obj_equal(a, b));
  }

  {
    Value a = make_list(vm, one, two, three);
    Value b = make_cons(vm, one, make_cons(vm, two, make_cons(vm, three, vm.objs.nil)));
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
    EXPECT(is_symbol(res));
  }

  {
    Value res = parse(vm, "(a b)");
    Value a = make_symbol(vm, "a");
    Value b = make_symbol(vm, "b");
    EXPECT(cons_first(vm, res) == a);
    EXPECT(cons_first(vm, cons_rest(res)) == b);
    EXPECT(!cons_rest(cons_rest(res)));
  }
}

void testEval() {
  VM vm;

  {
    EXPECT_INT_EQ(integer_value(eval(vm, make_integer(vm, 1), vm.objs.nil)), 1);
    EXPECT_INT_EQ(integer_value(eval(vm, make_integer(vm, 42), vm.objs.nil)), 42);
  }

  {
    Value add = vm.objs.builtin_add;
    Value one = make_integer(vm, 1);
    Value two = make_integer(vm, 2);

    Value list = make_list(vm, add, one, two);

    EXPECT_INT_EQ(integer_value(eval(vm, list, vm.objs.nil)), 3);
  }

  {
    Value a = make_symbol(vm, "a");
    Value pair = make_cons(vm, a, make_integer(vm, 42));
    Value env = make_cons(vm, pair, make_nil(vm));
    EXPECT_INT_EQ(integer_value(eval(vm, a, env)), 42);
  }

  {
    Value plus = vm.objs.sym_plus;

    Value add = vm.objs.builtin_add;
    Value one = make_integer(vm, 1);
    Value two = make_integer(vm, 2);

    Value list = make_list(vm, plus, one, two);

    Value pair = make_cons(vm, plus, add);
    Value env = make_cons(vm, pair, make_nil(vm));
    EXPECT_INT_EQ(integer_value(eval(vm, list, env)), 3);
  }

  {
    Value _if = vm.objs.sym_if;
    Value _true = vm.objs.bool_true;
    Value one = make_integer(vm, 1);
    Value two = make_integer(vm, 2);

    Value list = make_list(vm, _if, _true, one, two);
    EXPECT_INT_EQ(integer_value(eval(vm, list, vm.objs.nil)), 1);
  }

  {
    Value _if = vm.objs.sym_if;
    Value _false = vm.objs.bool_false;
    Value one = make_integer(vm, 1);
    Value two = make_integer(vm, 2);

    Value list = make_list(vm, _if, _false, one, two);
    EXPECT_INT_EQ(integer_value(eval(vm, list, vm.objs.nil)), 2);
  }

}

void testParseAndEval() {
  VM vm;

  {
    Value plus = vm.objs.sym_plus;
    Value add = vm.objs.builtin_add;

    Value pair = make_cons(vm, plus, add);
    Value env = make_cons(vm, pair, make_nil(vm));

    Value input = parse(vm, "(+ 1 2)");
    Value res = eval(vm, input, env);

    EXPECT_INT_EQ(integer_value(res), 3);
  }

  {
    Value scons = vm.objs.sym_cons;
    Value cons = vm.objs.builtin_cons;

    Value env = make_list(vm, make_cons(vm, scons, cons));

    Value input = parse(vm, "((letlambdas ( ((myfunc x y) (cons x y)) ) myfunc) 1 2)");
    Value res = eval(vm, input, env);

    EXPECT(obj_equal(res, make_cons(vm, make_integer(vm, 1), make_integer(vm, 2))));
  }

  {
    Value input = parse(vm, "((import core +) 1 2)");
    Value res = eval(vm, input, make_nil(vm));

    EXPECT_INT_EQ(integer_value(res), 3);
  }

}

void testAll() {
  testMakeList();
  testParse();
  testEval();
  testParseAndEval();
}

Value parse_file(VM& vm, const char* file, bool multiexpr) {

  FILE* f = fopen(file, "rb");
  fseek(f, 0, SEEK_END);
  size_t len = ftell(f);
  fseek(f, 0, SEEK_SET);
  char* buf = (char*) malloc(len + 1);
  fread(buf, 1, len, f);
  fclose(f);
  buf[len] = 0;

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
  return eval(vm, obj, vm.objs.nil);
}

int main(int argc, char** argv) {
  if(argc == 2) {
    if(strcmp(argv[1], "--test") == 0) {
      testAll();
      return 0;
    } else {
      VM vm;

      Value result = run_file(vm, argv[1], false);

      debug_print(result);

    }
  } else if(argc == 3) {
    VM vm;

    Value transformer = run_file(vm, argv[1], false);
    Value input = parse_file(vm, argv[2], true);

    Value quoted_input = make_list(vm, vm.objs.sym_quote, input);
    Value transformed = eval(vm, make_list(vm, transformer, quoted_input), vm.objs.nil);

    Value result = eval(vm, transformed, vm.objs.nil);
    debug_print(result);
    obj_print(transformed);

  }
  return 0;
}

