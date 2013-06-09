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

void _assert_failed(const char* file, int line, const char* message, ...) {
  printf("assertion failure, %s:%d:\n  ", file, line);

  va_list ap;
  va_start(ap, message);
  vprintf(message, ap);
  va_end(ap);

  printf("\n");

  exit(1);
}

typedef int bool;

const bool false = 0;
const bool true = 1;

#define nullptr ((void*)0)

enum {
  OBJ_NIL,
  OBJ_CONS,
  OBJ_STRING,
  OBJ_INTEGER,
  OBJ_SYMBOL,
  OBJ_BUILTIN,
  OBJ_BOOL,
  OBJ_LAMBDA
};

struct _vm_t;

typedef struct _obj_t* (*builtin_func_t)(struct _vm_t* vm, struct _obj_t* args);

typedef struct _obj_t {
  uintptr_t flags;
  union {
    struct {
      struct _obj_t* first;
      struct _obj_t* rest;
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
      builtin_func_t func;
    } as_builtin;
    struct {
      bool value;
    } as_bool;
    struct {
      struct _obj_t* params;
      struct _obj_t* body;
      struct _obj_t* env;
    } as_lambda;
  };
} obj_t;

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

typedef struct _vm_t {
  heap_block_t* heap;
  size_t heap_block_size;

  struct {
    obj_t* nil;

    obj_t* bool_true;
    obj_t* bool_false;

    obj_t* sym_if;
    obj_t* sym_letlambdas;
    obj_t* sym_import;
    obj_t* sym_core;
    obj_t* sym_quote;

    obj_t* sym_plus;
    obj_t* sym_is_cons;
    obj_t* sym_cons;
    obj_t* sym_first;
    obj_t* sym_rest;
    obj_t* sym_is_symbol;
    obj_t* sym_is_equal;
    obj_t* sym_is_nil;


    obj_t* builtin_add;
    obj_t* builtin_is_cons;
    obj_t* builtin_cons;
    obj_t* builtin_first;
    obj_t* builtin_rest;
    obj_t* builtin_is_symbol;
    obj_t* builtin_is_equal;
    obj_t* builtin_is_nil;
  } objs;

  obj_t* syms;

  obj_t* core_imports;
} vm_t;

obj_t* make_nil(vm_t* vm);
obj_t* make_bool(vm_t* vm, bool value);
obj_t* make_symbol(vm_t* vm, const char* name);
obj_t* make_builtin(vm_t* vm, builtin_func_t func);
obj_t* make_list(vm_t* vm, ...);
obj_t* make_cons(vm_t* vm, obj_t* first, obj_t* rest);

obj_t* builtin_add(vm_t* vm, obj_t* args);
obj_t* builtin_is_cons(vm_t* vm, obj_t* args);
obj_t* builtin_cons(vm_t* vm, obj_t* args);
obj_t* builtin_first(vm_t* vm, obj_t* args);
obj_t* builtin_rest(vm_t* vm, obj_t* args);
obj_t* builtin_is_symbol(vm_t* vm, obj_t* args);
obj_t* builtin_is_equal(vm_t* vm, obj_t* args);
obj_t* builtin_is_nil(vm_t* vm, obj_t* args);

vm_t* make_vm(size_t heap_block_size) {
  vm_t* vm = (vm_t*) malloc(sizeof(vm_t));
  vm->heap_block_size = heap_block_size;
  vm->heap = make_heap_block(heap_block_size, 0);

  vm->syms = make_nil(vm);

  vm->objs.nil = make_nil(vm);

  vm->objs.bool_true = make_bool(vm, true);
  vm->objs.bool_false = make_bool(vm, false);

  vm->objs.sym_if = make_symbol(vm, "if");
  vm->objs.sym_letlambdas = make_symbol(vm, "letlambdas");
  vm->objs.sym_import = make_symbol(vm, "import");
  vm->objs.sym_core = make_symbol(vm, "core");
  vm->objs.sym_quote = make_symbol(vm, "quote");

  vm->objs.sym_plus = make_symbol(vm, "+");
  vm->objs.sym_is_cons = make_symbol(vm, "cons?");
  vm->objs.sym_cons = make_symbol(vm, "cons");
  vm->objs.sym_first = make_symbol(vm, "first");
  vm->objs.sym_rest = make_symbol(vm, "rest");
  vm->objs.sym_is_symbol = make_symbol(vm, "sym?");
  vm->objs.sym_is_equal = make_symbol(vm, "eq?");
  vm->objs.sym_is_nil = make_symbol(vm, "nil?");

  vm->objs.builtin_add = make_builtin(vm, builtin_add);
  vm->objs.builtin_is_cons = make_builtin(vm, builtin_is_cons);
  vm->objs.builtin_cons = make_builtin(vm, builtin_cons);
  vm->objs.builtin_first = make_builtin(vm, builtin_first);
  vm->objs.builtin_rest = make_builtin(vm, builtin_rest);
  vm->objs.builtin_is_symbol = make_builtin(vm, builtin_is_symbol);
  vm->objs.builtin_is_equal = make_builtin(vm, builtin_is_equal);
  vm->objs.builtin_is_nil = make_builtin(vm, builtin_is_nil);

  vm->core_imports = make_list(vm,
    make_cons(vm, vm->objs.sym_plus, vm->objs.builtin_add),
    make_cons(vm, vm->objs.sym_is_cons, vm->objs.builtin_is_cons),
    make_cons(vm, vm->objs.sym_cons, vm->objs.builtin_cons),
    make_cons(vm, vm->objs.sym_first, vm->objs.builtin_first),
    make_cons(vm, vm->objs.sym_rest, vm->objs.builtin_rest),
    make_cons(vm, vm->objs.sym_is_symbol, vm->objs.builtin_is_symbol),
    make_cons(vm, vm->objs.sym_is_equal, vm->objs.builtin_is_equal),
    make_cons(vm, vm->objs.sym_is_nil, vm->objs.builtin_is_nil),
    nullptr);
  return vm;
}

void free_vm(vm_t* vm) {
  free_heap_block(vm->heap);
  free(vm);
}

size_t max_sizet(size_t a, size_t b) {
  return a > b ? a : b;
}

void* vm_alloc(vm_t* vm, size_t size) {
  if(size > vm->heap->capacity - vm->heap->used) {
    vm->heap = make_heap_block(max_sizet(vm->heap_block_size, size * 2 + sizeof(heap_block_t)), vm->heap);
  }
  void* ret = vm->heap->data + vm->heap->used;
  vm->heap->used += size;
  return ret;
}

obj_t* make_obj(vm_t* vm, int type) {
  obj_t* ret = (obj_t*) vm_alloc(vm, sizeof(obj_t));
  memset(ret, 0xfe, sizeof(obj_t));
  ret->flags = type;
  return ret;
}

void obj_print(obj_t* value);

bool is_nil(obj_t* o) {
  return o->flags == OBJ_NIL;
}

obj_t* make_nil(vm_t* vm) {
  obj_t* o = (obj_t*) vm_alloc(vm, sizeof(obj_t));
  o->flags = OBJ_NIL;
  return o;
}

bool is_cons(obj_t* o) {
  return o->flags == OBJ_CONS;
}

obj_t* make_cons(vm_t* vm, obj_t* a, obj_t* b) {
  obj_t* o = make_obj(vm, OBJ_CONS);
  o->as_cons.first = a;
  o->as_cons.rest = b;
  return o;
}

obj_t* make_list(vm_t* vm, ...) {
  va_list ap;
  va_start(ap, vm);

  obj_t* nil = vm->objs.nil;
  obj_t* result = nil;
  obj_t** ptr = &result;
  while(true) {
    obj_t* value = va_arg(ap, obj_t*);
    if(!value) {
      break;
    }
    *ptr = make_cons(vm, value, nil);
    ptr = &(*ptr)->as_cons.rest;
  }

  va_end(ap);

  return result;
}

obj_t* cons_first(obj_t* o) {
  EXPECT(is_cons(o));
  return o->as_cons.first;
}

obj_t* cons_rest(obj_t* o) {
  EXPECT(is_cons(o));
  return o->as_cons.rest;
}

size_t list_length(obj_t* list) {
  size_t ret = 0;
  while(!is_nil(list)) {
    ret++;
    list = cons_rest(list);
  }
  return ret;
}

obj_t* list_prepend_n_objs(vm_t* vm, size_t len, obj_t* obj, obj_t* list) {
  while(len > 0) {
    list = make_cons(vm, obj, list);
    len--;
  }
  return list;
}

bool is_string(obj_t* o) {
  return o->flags == OBJ_STRING;
}

obj_t* make_string(vm_t* vm, const char* value) {
  obj_t* o = (obj_t*) vm_alloc(vm, sizeof(obj_t));
  o->flags = OBJ_STRING;
  o->as_string.value = value;
  return o;
}

const char* string_value(obj_t* o) {
  EXPECT(is_string(o));
  return o->as_string.value;
}

bool is_integer(obj_t* o) {
  return o->flags == OBJ_INTEGER;
}

obj_t* make_integer(vm_t* vm, int value) {
  obj_t* o = make_obj(vm, OBJ_INTEGER);
  o->as_integer.value = value;
  return o;
}

int integer_value(obj_t* o) {
  EXPECT(is_integer(o));
  return o->as_integer.value;
}

bool is_symbol(obj_t* o) {
  return o->flags == OBJ_SYMBOL;
}

const char* symbol_name(obj_t* o) {
  EXPECT(is_symbol(o));
  return o->as_symbol.name;
}
typedef obj_t map_t;

obj_t* map_lookup(map_t* map, obj_t* key) {
  while(!(is_nil(map))) {
    obj_t* item = cons_first(map);
    if(cons_first(item) == key) {
      return cons_rest(item);
    } else {
      map = cons_rest(map);
    }
  }
  EXPECT(0);
  return 0;
}

obj_t* make_symbol_with_length(vm_t* vm, const char* begin, size_t length) {
  obj_t* o = vm->syms;
  while(!is_nil(o)) {
    obj_t* first = cons_first(o);
    o = cons_rest(o);
    const char* sym = symbol_name(first);
    size_t len = strlen(sym);
    if(len == length) {
      if(memcmp(begin, sym, length) == 0) {
        return first;
      }
    }
  }
  o = make_obj(vm, OBJ_SYMBOL);
  o->as_symbol.name = strndup(begin, length);
  vm->syms = make_cons(vm, o, vm->syms);
  return o;
}

obj_t* make_symbol(vm_t* vm, const char* name) {
  return make_symbol_with_length(vm, name, strlen(name));
}

bool is_builtin(obj_t* o) {
  return o->flags == OBJ_BUILTIN;
}

builtin_func_t builtin_func(obj_t* o) {
  EXPECT(is_builtin(o));
  return o->as_builtin.func;
}

obj_t* make_builtin(vm_t* vm, builtin_func_t func) {
  obj_t* o = make_obj(vm, OBJ_BUILTIN);
  o->as_builtin.func = func;
  return o;
}

bool is_bool(obj_t* o) {
  return o->flags == OBJ_BOOL;
}

bool bool_value(obj_t* o) {
  EXPECT(is_bool(o));
  return o->as_bool.value;
}

obj_t* make_bool(vm_t* vm, bool value) {
  obj_t* o = make_obj(vm, OBJ_BOOL);
  o->as_bool.value = value;
  return o;
}

bool is_lambda(obj_t* o) {
  return o->flags == OBJ_LAMBDA;
}

obj_t* lambda_params(obj_t* o) {
  EXPECT(is_lambda(o));
  return o->as_lambda.params;
}

obj_t* lambda_body(obj_t* o) {
  EXPECT(is_lambda(o));
  return o->as_lambda.body;
}

obj_t* lambda_env(obj_t* o) {
  EXPECT(is_lambda(o));
  return o->as_lambda.env;
}

obj_t* make_lambda(vm_t* vm, obj_t* params, obj_t* body, obj_t* env) {
  obj_t* o = make_obj(vm, OBJ_LAMBDA);
  o->as_lambda.params = params;
  o->as_lambda.body = body;
  o->as_lambda.env = env;
  return o;
}

bool obj_equal(obj_t* a, obj_t* b) {
  if(a->flags != b->flags) {
    return false;
  }
  switch(a->flags) {
  case OBJ_NIL:
    return is_nil(b);
  case OBJ_CONS:
    if(obj_equal(cons_first(a), cons_first(b))) {
      if(obj_equal(cons_rest(a), cons_rest(b))) {
        return true;
      }
    }
    return false;
  case OBJ_STRING:
    return strcmp(string_value(a), string_value(b)) == 0;
  case OBJ_INTEGER:
    return integer_value(a) == integer_value(b);
  case OBJ_SYMBOL:
    return a == b;
  case OBJ_BUILTIN:
    return a == b;
  case OBJ_BOOL:
    return bool_value(a) == bool_value(b);
  default:
    EXPECT(0);
    return false;
  }
}

void print_value(obj_t* value);
void print_list(obj_t* value) {
  switch(value->flags) {
  case OBJ_NIL:
    printf(")");
    return;
  case OBJ_CONS:
    printf(" ");
    print_value(cons_first(value));
    print_list(cons_rest(value));
    return;
  default:
    printf(" . ");
    print_value(value);
    printf(")");
    return;
  }
}

void print_value(obj_t* value) {
  switch(value->flags) {
  case OBJ_NIL:
    printf("()");
    return;
  case OBJ_CONS:
    printf("(");
    print_value(cons_first(value));
    print_list(cons_rest(value));
    return;
  case OBJ_STRING:
    printf("\"%s\"", string_value(value));
    return;
  case OBJ_INTEGER:
    printf("%d", integer_value(value));
    return;
  case OBJ_SYMBOL:
    printf("%s", symbol_name(value));
    return;
  case OBJ_BUILTIN:
    printf("<builtin@%p>", builtin_func(value));
    return;
  case OBJ_BOOL:
    printf("%s", bool_value(value) ? "#t" : "#f");
    return;
  case OBJ_LAMBDA:
    printf("<lambda@%p>", value);
    return;
  default:
    EXPECT(0);
    return;
  }
}

void obj_print(obj_t* value) {
  print_value(value);
  printf("\n");
}

obj_t* builtin_add(vm_t* vm, obj_t* args) {
  int res = 0;
  while(!is_nil(args)) {
    obj_t* a = cons_first(args);
    args = cons_rest(args);
    res += integer_value(a);
  }
  return make_integer(vm, res);
}

obj_t* builtin_is_cons(vm_t* vm, obj_t* args) {
  obj_t* arg = cons_first(args);
  EXPECT(is_nil(cons_rest(args)));
  return is_cons(arg) ? vm->objs.bool_true : vm->objs.bool_false;
}

obj_t* builtin_cons(vm_t* vm, obj_t* args) {
  obj_t* first = cons_first(args);
  args = cons_rest(args);
  obj_t* rest = cons_first(args);
  EXPECT(is_nil(cons_rest(args)));
  return make_cons(vm, first, rest);
}

obj_t* builtin_first(vm_t* vm, obj_t* args) {
  obj_t* arg = cons_first(args);
  EXPECT(is_nil(cons_rest(args)));
  return cons_first(arg);
}

obj_t* builtin_rest(vm_t* vm, obj_t* args) {
  obj_t* arg = cons_first(args);
  EXPECT(is_nil(cons_rest(args)));
  return cons_rest(arg);
}

obj_t* builtin_is_symbol(vm_t* vm, obj_t* args) {
  obj_t* arg = cons_first(args);
  EXPECT(is_nil(cons_rest(args)));
  return is_symbol(arg) ? vm->objs.bool_true : vm->objs.bool_false;
}

obj_t* builtin_is_equal(vm_t* vm, obj_t* args) {
  obj_t* a = cons_first(args);
  args = cons_rest(args);
  obj_t* b = cons_first(args);
  EXPECT(is_nil(cons_rest(args)));
  return obj_equal(a, b) ? vm->objs.bool_true : vm->objs.bool_false;
}

obj_t* builtin_is_nil(vm_t* vm, obj_t* args) {
  obj_t* arg = cons_first(args);
  EXPECT(is_nil(cons_rest(args)));
  return is_nil(arg) ? vm->objs.bool_true : vm->objs.bool_false;
}

bool is_self_evaluating(obj_t* o) {
  return is_integer(o) || is_nil(o) || is_builtin(o) || is_bool(o) || is_lambda(o);
}

obj_t* extend_env(vm_t* vm, obj_t* params, obj_t* args, obj_t* env) {
  if(is_nil(params)) {
    EXPECT(is_nil(args));
    return env;
  } else {
    obj_t* key = cons_first(params);
    EXPECT(is_symbol(key));
    obj_t* value = cons_first(args);
    return extend_env(vm,
      cons_rest(params),
      cons_rest(args),
      make_cons(vm, make_cons(vm, key, value), env));
  }
}

obj_t* make_lambdas_env(vm_t* vm, obj_t* lambdas, obj_t* env) {
  size_t len = list_length(lambdas);
  obj_t* orig_env = env;

  env = list_prepend_n_objs(vm, len, vm->objs.nil, env);

  obj_t* envptr = env;
  while(!is_nil(lambdas)) {
    obj_t* lambda = cons_first(lambdas);

    obj_t* name_and_params = cons_first(lambda);
    lambda = cons_rest(lambda);
    EXPECT(is_nil(cons_rest(lambda)));
    obj_t* body = cons_first(lambda);

    obj_t* name = cons_first(name_and_params);
    printf("name: "); obj_print(name);
    EXPECT(is_symbol(name));
    obj_t* params = cons_rest(name_and_params);
    printf("params: "); obj_print(params);
    printf("body: "); obj_print(body);

    lambda = make_lambda(vm, params, body, env);

    envptr->as_cons.first = make_cons(vm, name, lambda);
    envptr = cons_rest(envptr);

    lambdas = cons_rest(lambdas);
  }

  ASSERT(orig_env == envptr);

  return env;
}

obj_t* eval(vm_t* vm, obj_t* o, map_t* env);
obj_t* eval_list(vm_t* vm, obj_t* o, map_t* env) {
  if(is_cons(o)) {
    return make_cons(vm, eval(vm, cons_first(o), env), eval_list(vm, cons_rest(o), env));
  } else {
    return eval(vm, o, env);
  }
}

obj_t* eval(vm_t* vm, obj_t* o, map_t* env) {
  // obj_t* procedure;
  // obj_t* arguments;
  // obj_t* result;
  static int depth = 0;
  printf("%*.seval: ", depth*2, ""); obj_print(o);
  while(true) {
    if(is_self_evaluating(o)) {
      return o;
    } else if(is_symbol(o)) {
      return map_lookup(env, o);
    } else if(is_cons(o)) {

      obj_t* f = cons_first(o);
      o = cons_rest(o);

      printf("f: %p %p: ", f, vm->objs.sym_letlambdas);obj_print(f);

      if(f == vm->objs.sym_if) {
        obj_t* c = cons_first(o);
        depth++;
        c = eval(vm, c, env);
        depth--;
        if(bool_value(c)) {
          o = cons_first(cons_rest(o));
        } else {
          o = cons_first(cons_rest(cons_rest(o)));
        }
      } else if(f == vm->objs.sym_letlambdas) {
        printf("found letlambdas: "); obj_print(o);
        obj_t* lambdas = cons_first(o);
        o = cons_rest(o);
        EXPECT(is_nil(cons_rest(o)));
        o = cons_first(o);
        env = make_lambdas_env(vm, lambdas, env);
      } else if(f == vm->objs.sym_import) {
        obj_t* lib = cons_first(o);
        o = cons_rest(o);
        obj_t* sym = cons_first(o);
        EXPECT(is_nil(cons_rest(o)));
        EXPECT(lib == vm->objs.sym_core);
        return map_lookup(vm->core_imports, sym);
      } else if(f == vm->objs.sym_quote) {
        obj_t* a = cons_first(o);
        EXPECT(is_nil(cons_rest(o)));
        return a;
      } else {
        depth++;
        f = eval(vm, f, env);
        depth--;
        if(is_builtin(f)) {
          obj_t* params = eval_list(vm, o, env);
          return builtin_func(f)(vm, params);
        } else if(is_lambda(f)) {
          obj_t* params = eval_list(vm, o, env);
          env = extend_env(vm, lambda_params(f), params, lambda_env(f));
          o = lambda_body(f);
          printf("continuing: "); obj_print(o);
        } else {
          EXPECT(0);
          return 0;
        }
      }
    } else {
      obj_print(o);
      EXPECT(0);
      return 0;
    }
  }

}

const char* skip_ws(const char* text) {
  while(*text == ' ' || *text == '\n' || *text == '\t') {
    text++;
  }
  return text;
}

bool is_digit(int ch) {
  return ch >= '0' && ch <= '9';
}

bool is_sym_char(int ch) {
  return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '?' || ch == '+' || ch == '-';
}

static obj_t* parse_value(vm_t* vm, const char** text);

static obj_t* parse_list(vm_t* vm, const char** text) {
  *text = skip_ws(*text);
  if(**text == ')') {
    (*text)++;
    return make_nil(vm);
  } else {
    obj_t* first = parse_value(vm, text);
    obj_t* rest = parse_list(vm, text);
    return make_cons(vm, first, rest);
  }
}

static obj_t* parse_value(vm_t* vm, const char** text) {
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
      return vm->objs.bool_true;
    case 'f':
      return vm->objs.bool_false;
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
    puts(*text);
    EXPECT(0);
    return 0;
  }
}

obj_t* parse(vm_t* vm, const char* text) {
  obj_t* result = parse_value(vm, &text);
  if(*text != '\0') {
    EXPECT(0);
  }
  return result;
}

obj_t* parse_multi(vm_t* vm, const char* text) {
  obj_t* result = vm->objs.nil;
  text = skip_ws(text);
  while(*text != '\0') {
    obj_t* o = parse_value(vm, &text);
    result = make_cons(vm, o, result);
  }
  return result;
}

void testMakeList() {
  vm_t* vm = make_vm(4096);

  obj_t* one = make_integer(vm, 1);
  obj_t* two = make_integer(vm, 2);
  obj_t* three = make_integer(vm, 3);

  {
    obj_t* a = make_list(vm, nullptr);
    obj_t* b = vm->objs.nil;
    EXPECT(obj_equal(a, b));
  }

  {
    obj_t* a = make_list(vm, one, nullptr);
    obj_t* b = make_cons(vm, one, vm->objs.nil);
    EXPECT(obj_equal(a, b));
  }

  {
    obj_t* a = make_list(vm, one, two, nullptr);
    obj_t* b = make_cons(vm, one, make_cons(vm, two, vm->objs.nil));
    EXPECT(obj_equal(a, b));
  }

  {
    obj_t* a = make_list(vm, one, two, three, nullptr);
    obj_t* b = make_cons(vm, one, make_cons(vm, two, make_cons(vm, three, vm->objs.nil)));
    EXPECT(obj_equal(a, b));
  }

  free_vm(vm);
}

void testParse() {
  vm_t* vm = make_vm(4096);

  {
    EXPECT_INT_EQ(integer_value(parse(vm, "1")), 1);
    EXPECT_INT_EQ(integer_value(parse(vm, "42")), 42);
  }

  {
    EXPECT(bool_value(parse(vm, "#t")) == true);
    EXPECT(bool_value(parse(vm, "#f")) == false);
  }

  {
    obj_t* res = parse(vm, "a");
    EXPECT(is_symbol(res));
  }

  {
    obj_t* res = parse(vm, "(a b)");
    obj_t* a = make_symbol(vm, "a");
    obj_t* b = make_symbol(vm, "b");
    EXPECT(cons_first(res) == a);
    EXPECT(cons_first(cons_rest(res)) == b);
    EXPECT(is_nil(cons_rest(cons_rest(res))));
  }
  free_vm(vm);
}

void testEval() {
  vm_t* vm = make_vm(4096);

  {
    EXPECT_INT_EQ(integer_value(eval(vm, make_integer(vm, 1), vm->objs.nil)), 1);
    EXPECT_INT_EQ(integer_value(eval(vm, make_integer(vm, 42), vm->objs.nil)), 42);
  }

  {
    obj_t* add = vm->objs.builtin_add;
    obj_t* one = make_integer(vm, 1);
    obj_t* two = make_integer(vm, 2);

    obj_t* list = make_list(vm, add, one, two, nullptr);

    EXPECT_INT_EQ(integer_value(eval(vm, list, vm->objs.nil)), 3);
  }

  {
    obj_t* a = make_symbol(vm, "a");
    obj_t* pair = make_cons(vm, a, make_integer(vm, 42));
    obj_t* env = make_cons(vm, pair, make_nil(vm));
    EXPECT_INT_EQ(integer_value(eval(vm, a, env)), 42);
  }

  {
    obj_t* plus = vm->objs.sym_plus;

    obj_t* add = vm->objs.builtin_add;
    obj_t* one = make_integer(vm, 1);
    obj_t* two = make_integer(vm, 2);

    obj_t* list = make_list(vm, plus, one, two, nullptr);

    obj_t* pair = make_cons(vm, plus, add);
    obj_t* env = make_cons(vm, pair, make_nil(vm));
    EXPECT_INT_EQ(integer_value(eval(vm, list, env)), 3);
  }

  {
    obj_t* _if = vm->objs.sym_if;
    obj_t* _true = vm->objs.bool_true;
    obj_t* one = make_integer(vm, 1);
    obj_t* two = make_integer(vm, 2);

    obj_t* list = make_list(vm, _if, _true, one, two, nullptr);
    EXPECT_INT_EQ(integer_value(eval(vm, list, vm->objs.nil)), 1);
  }

  {
    obj_t* _if = vm->objs.sym_if;
    obj_t* _false = vm->objs.bool_false;
    obj_t* one = make_integer(vm, 1);
    obj_t* two = make_integer(vm, 2);

    obj_t* list = make_list(vm, _if, _false, one, two, nullptr);
    EXPECT_INT_EQ(integer_value(eval(vm, list, vm->objs.nil)), 2);
  }

  free_vm(vm);
}

void testParseAndEval() {
  vm_t* vm = make_vm(4096);

  {
    obj_t* plus = vm->objs.sym_plus;
    obj_t* add = vm->objs.builtin_add;

    obj_t* pair = make_cons(vm, plus, add);
    obj_t* env = make_cons(vm, pair, make_nil(vm));

    obj_t* input = parse(vm, "(+ 1 2)");
    obj_t* res = eval(vm, input, env);

    EXPECT_INT_EQ(integer_value(res), 3);
  }

  {
    obj_t* scons = vm->objs.sym_cons;
    obj_t* cons = vm->objs.builtin_cons;

    obj_t* env = make_list(vm, make_cons(vm, scons, cons), nullptr);

    obj_t* input = parse(vm, "((letlambdas ( ((myfunc x y) (cons x y)) ) myfunc) 1 2)");
    obj_t* res = eval(vm, input, env);

    EXPECT(obj_equal(res, make_cons(vm, make_integer(vm, 1), make_integer(vm, 2))));
  }

  {
    obj_t* input = parse(vm, "((import core +) 1 2)");
    obj_t* res = eval(vm, input, make_nil(vm));

    EXPECT_INT_EQ(integer_value(res), 3);
  }

  free_vm(vm);
}

void testAll() {
  testMakeList();
  testParse();
  testEval();
  testParseAndEval();
}

obj_t* parse_file(vm_t* vm, const char* file, bool multiexpr) {

  FILE* f = fopen(file, "rb");
  fseek(f, 0, SEEK_END);
  size_t len = ftell(f);
  fseek(f, 0, SEEK_SET);
  char* buf = (char*) malloc(len + 1);
  fread(buf, 1, len, f);
  fclose(f);
  buf[len] = 0;

  obj_t* obj;
  if(multiexpr) {
    obj = parse_multi(vm, buf);
  } else {
    obj = parse(vm, buf);
  }

  free(buf);

  return obj;
}

obj_t* run_file(vm_t* vm, const char* file, bool multiexpr) {
  obj_t* obj = parse_file(vm, file, multiexpr);
  return eval(vm, obj, vm->objs.nil);
}

int main(int argc, char** argv) {
  if(argc == 2) {
    if(strcmp(argv[1], "--test") == 0) {
      testAll();
      return 0;
    } else {
      vm_t* vm = make_vm(4096);

      obj_t* result = run_file(vm, argv[1], false);

      obj_print(result);

      free_vm(vm);
    }
  } else if(argc == 3) {
    vm_t* vm = make_vm(4096);

    obj_t* transformer = run_file(vm, argv[1], false);
    obj_t* input = parse_file(vm, argv[2], true);

    obj_t* quoted_input = make_list(vm, vm->objs.sym_quote, input, nullptr);
    obj_t* transformed = eval(vm, make_list(vm, transformer, quoted_input, nullptr), vm->objs.nil);

    obj_t* result = eval(vm, transformed, vm->objs.nil);
    obj_print(result);

    free_vm(vm);
  }
  return 0;
}

