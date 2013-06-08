#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

void _assert_failed(const char* file, int line, const char* message, ...);

#define EXPECT(e) do { if(!(e)) {_assert_failed(__FILE__, __LINE__, "%s", #e);} } while(0)
#define EXPECT_MSG(e, msg, ...) do { if(!(e)) {_assert_failed(__FILE__, __LINE__, "%s\n  " msg, #e, ##__VA_ARGS__);} } while(0)

#define EXPECT_INT_EQ(expected, actual) EXPECT_MSG((expected) == (actual), "expected: %d, actual: %d", expected, actual);

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

    obj_t* builtin_add;
  } objs;

  obj_t* syms;
} vm_t;

obj_t* make_nil(vm_t* vm);
obj_t* make_builtin(vm_t* vm, builtin_func_t func);

static obj_t* builtin_add(vm_t* vm, obj_t* args);

vm_t* make_vm(size_t heap_block_size) {
  vm_t* vm = (vm_t*) malloc(sizeof(vm_t));
  vm->heap_block_size = heap_block_size;
  vm->heap = make_heap_block(heap_block_size, 0);

  vm->syms = make_nil(vm);

  vm->objs.nil = make_nil(vm);

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
    puts("lookup");
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
  return is_integer(o) || is_nil(o);
}

obj_t* eval(vm_t* vm, obj_t* o, map_t* env) {
  // obj_t* procedure;
  // obj_t* arguments;
  // obj_t* result;

  if(is_self_evaluating(o)) {
    return o;
  } else if(is_symbol(o)) {
    return map_lookup(env, o);
  } else if(is_cons(o)) {
    obj_t* f = cons_first(o);
    if(is_builtin(f)) {
      return builtin_func(f)(vm, cons_rest(o));
    } else {
      EXPECT(0);
    }
  } else {
    EXPECT(0);
  }

  return 0;
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

void testParse() {
  vm_t* vm = make_vm(4096);
  EXPECT_INT_EQ(integer_value(parse(vm, "1")), 1);
  EXPECT_INT_EQ(integer_value(parse(vm, "42")), 42);

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
  EXPECT_INT_EQ(integer_value(eval(vm, make_integer(vm, 1), 0)), 1);
  EXPECT_INT_EQ(integer_value(eval(vm, make_integer(vm, 42), 0)), 42);

  {
    obj_t* add = vm->objs.builtin_add;
    obj_t* one = make_integer(vm, 1);
    obj_t* two = make_integer(vm, 2);

    obj_t* list = make_cons(vm, add, make_cons(vm, one, make_cons(vm, two, make_nil(vm))));
    EXPECT_INT_EQ(integer_value(eval(vm, list, 0)), 3);
  }

  {
    obj_t* a = make_symbol(vm, "a", 1);
    obj_t* pair = make_cons(vm, a, make_integer(vm, 42));
    obj_t* env = make_cons(vm, pair, make_nil(vm));
    EXPECT_INT_EQ(integer_value(eval(vm, a, env)), 42);
  }

  {
    obj_t* plus = make_symbol(vm, "+", 1);

    obj_t* add = vm->objs.builtin_add;
    obj_t* one = make_integer(vm, 1);
    obj_t* two = make_integer(vm, 2);

    obj_t* list = make_cons(vm, add, make_cons(vm, one, make_cons(vm, two, make_nil(vm))));
    obj_t* pair = make_cons(vm, plus, add);
    obj_t* env = make_cons(vm, pair, make_nil(vm));
    EXPECT_INT_EQ(integer_value(eval(vm, list, env)), 3);
  }
}

void testAll() {
  testParse();
  testEval();
}

int main(int argc, char** argv) {
  testAll();
  return 0;
}
