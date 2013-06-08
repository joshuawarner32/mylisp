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
    obj_t* sym_plus;
    obj_t* sym_lambda;

    obj_t* builtin_add;
  } objs;

  obj_t* syms;
} vm_t;

obj_t* make_nil(vm_t* vm);
obj_t* make_bool(vm_t* vm, bool value);
obj_t* make_symbol(vm_t* vm, const char* name, size_t length);
obj_t* make_builtin(vm_t* vm, builtin_func_t func);

static obj_t* builtin_add(vm_t* vm, obj_t* args);

vm_t* make_vm(size_t heap_block_size) {
  vm_t* vm = (vm_t*) malloc(sizeof(vm_t));
  vm->heap_block_size = heap_block_size;
  vm->heap = make_heap_block(heap_block_size, 0);

  vm->syms = make_nil(vm);

  vm->objs.nil = make_nil(vm);

  vm->objs.bool_true = make_bool(vm, true);
  vm->objs.bool_false = make_bool(vm, false);

  vm->objs.sym_if = make_symbol(vm, "if", 2);
  vm->objs.sym_plus = make_symbol(vm, "+", 1);
  vm->objs.sym_lambda = make_symbol(vm, "lambda", 6);

  vm->objs.builtin_add = make_builtin(vm, builtin_add);
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

obj_t* make_symbol(vm_t* vm, const char* begin, size_t length) {
  obj_t* o = vm->syms;
  while(!is_nil(o)) {
    obj_t* first = cons_first(o);
    o = cons_rest(o);
    if(strncmp(begin, symbol_name(first), length) == 0) {
      return first;
    }
  }
  o = make_obj(vm, OBJ_SYMBOL);
  o->as_symbol.name = strndup(begin, length);
  vm->syms = make_cons(vm, o, vm->syms);
  return o;
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

bool is_self_evaluating(obj_t* o) {
  return is_integer(o) || is_nil(o) || is_builtin(o) || is_bool(o);
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
  while(true) {
    printf("eval ");
    obj_print(o);
    printf("env: ");
    obj_print(env);

    if(is_self_evaluating(o)) {
      return o;
    } else if(is_symbol(o)) {
      return map_lookup(env, o);
    } else if(is_cons(o)) {
      obj_t* f = cons_first(o);
      o = cons_rest(o);

      if(f == vm->objs.sym_if) {
        obj_t* c = cons_first(o);
        c = eval(vm, c, env);
        if(bool_value(c)) {
          o = cons_first(cons_rest(o));
        } else {
          o = cons_first(cons_rest(cons_rest(o)));
        }
      } else if(f == vm->objs.sym_lambda) {
        obj_t* params = cons_first(o);
        obj_t* body = cons_first(cons_rest(o));
        return make_lambda(vm, params, body, env);
      } else {
        f = eval(vm, f, env);
        if(is_builtin(f)) {
          obj_t* params = eval_list(vm, o, env);
          return builtin_func(f)(vm, params);
        } else if(is_lambda(f)) {
          obj_t* params = eval_list(vm, o, env);
          env = extend_env(vm, lambda_params(f), params, env);
          o = lambda_body(f);
        } else {
          EXPECT(0);
          return 0;
        }
      }
    } else {
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
  return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '?' || ch == '+';
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
    return make_symbol(vm, start, *text - start);
  } else {
    EXPECT(0);
    return 0;
  }
}

obj_t* parse(vm_t* vm, const char* text) {
  return parse_value(vm, &text);
}

void testMakeList() {
  vm_t* vm = make_vm(4096);

  obj_t* one = make_integer(vm, 1);
  obj_t* two = make_integer(vm, 2);
  obj_t* three = make_integer(vm, 3);

  {
    obj_t* a = make_list(vm, 0);
    obj_t* b = vm->objs.nil;
    EXPECT(obj_equal(a, b));
  }

  {
    obj_t* a = make_list(vm, one, 0);
    obj_t* b = make_cons(vm, one, vm->objs.nil);
    EXPECT(obj_equal(a, b));
  }

  {
    obj_t* a = make_list(vm, one, two, 0);
    obj_t* b = make_cons(vm, one, make_cons(vm, two, vm->objs.nil));
    EXPECT(obj_equal(a, b));
  }

  {
    obj_t* a = make_list(vm, one, two, three, 0);
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
    obj_t* a = make_symbol(vm, "a", 1);
    obj_t* b = make_symbol(vm, "b", 1);
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

    obj_t* list = make_list(vm, add, one, two, 0);

    EXPECT_INT_EQ(integer_value(eval(vm, list, vm->objs.nil)), 3);
  }

  {
    obj_t* a = make_symbol(vm, "a", 1);
    obj_t* pair = make_cons(vm, a, make_integer(vm, 42));
    obj_t* env = make_cons(vm, pair, make_nil(vm));
    EXPECT_INT_EQ(integer_value(eval(vm, a, env)), 42);
  }

  {
    obj_t* plus = vm->objs.sym_plus;

    obj_t* add = vm->objs.builtin_add;
    obj_t* one = make_integer(vm, 1);
    obj_t* two = make_integer(vm, 2);

    obj_t* list = make_list(vm, plus, one, two, 0);

    obj_t* pair = make_cons(vm, plus, add);
    obj_t* env = make_cons(vm, pair, make_nil(vm));
    EXPECT_INT_EQ(integer_value(eval(vm, list, env)), 3);
  }

  {
    obj_t* _if = vm->objs.sym_if;
    obj_t* _true = vm->objs.bool_true;
    obj_t* one = make_integer(vm, 1);
    obj_t* two = make_integer(vm, 2);

    obj_t* list = make_list(vm, _if, _true, one, two, 0);
    EXPECT_INT_EQ(integer_value(eval(vm, list, vm->objs.nil)), 1);
  }

  {
    obj_t* _if = vm->objs.sym_if;
    obj_t* _false = vm->objs.bool_false;
    obj_t* one = make_integer(vm, 1);
    obj_t* two = make_integer(vm, 2);

    obj_t* list = make_list(vm, _if, _false, one, two, 0);
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
    obj_t* plus = vm->objs.sym_plus;
    obj_t* add = vm->objs.builtin_add;

    obj_t* pair = make_cons(vm, plus, add);
    obj_t* env = make_cons(vm, pair, make_nil(vm));

    obj_t* input = parse(vm, "((lambda (x y) (+ x y)) 1 2)");
    obj_t* res = eval(vm, input, env);

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

int main(int argc, char** argv) {
  testAll();
  return 0;
}

