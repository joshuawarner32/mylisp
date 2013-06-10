#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "vm.h"
#include "builtin.h"

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

VM::VM(size_t heap_block_size):
  heap_block_size(heap_block_size),
  heap(make_heap_block(heap_block_size, 0)),
  nil(new(*this) Object(Object::Type::Nil)),
  true_(new (*this) Object(Object::Type::Bool)),
  false_(new (*this) Object(Object::Type::Bool)),
  symList(nil),
  syms(*this)
{
  VM& vm = *this;

  true_->as_bool.value = true;
  false_->as_bool.value = false;

  objs.builtin_add = make_builtin(vm, "add", builtin_add);
  objs.builtin_sub = make_builtin(vm, "sub", builtin_sub);
  objs.builtin_mul = make_builtin(vm, "mul", builtin_mul);
  objs.builtin_div = make_builtin(vm, "div", builtin_div);
  objs.builtin_modulo = make_builtin(vm, "modulo", builtin_modulo);
  objs.builtin_is_cons = make_builtin(vm, "is_cons", builtin_is_cons);
  objs.builtin_cons = make_builtin(vm, "cons", builtin_cons);
  objs.builtin_first = make_builtin(vm, "first", builtin_first);
  objs.builtin_rest = make_builtin(vm, "rest", builtin_rest);
  objs.builtin_is_symbol = make_builtin(vm, "is_symbol", builtin_is_symbol);
  objs.builtin_is_equal = make_builtin(vm, "is_equal", builtin_is_equal);
  objs.builtin_is_nil = make_builtin(vm, "is_nil", builtin_is_nil);
  objs.builtin_is_int = make_builtin(vm, "is_int", builtin_is_int);
  objs.builtin_concat = make_builtin(vm, "concat", builtin_concat);
  objs.builtin_symbol_name = make_builtin(vm, "concat", builtin_symbol_name);

  core_imports = make_list(vm,
    make_cons(vm, syms.add, objs.builtin_add),
    make_cons(vm, syms.sub, objs.builtin_sub),
    make_cons(vm, syms.mul, objs.builtin_mul),
    make_cons(vm, syms.div, objs.builtin_div),
    make_cons(vm, syms.modulo, objs.builtin_modulo),
    make_cons(vm, syms.is_cons, objs.builtin_is_cons),
    make_cons(vm, syms.cons, objs.builtin_cons),
    make_cons(vm, syms.first, objs.builtin_first),
    make_cons(vm, syms.rest, objs.builtin_rest),
    make_cons(vm, syms.is_symbol, objs.builtin_is_symbol),
    make_cons(vm, syms.is_equal, objs.builtin_is_equal),
    make_cons(vm, syms.is_nil, objs.builtin_is_nil),
    make_cons(vm, syms.is_int, objs.builtin_is_int),
    make_cons(vm, syms.concat, objs.builtin_concat),
    make_cons(vm, syms.symbol_name, objs.builtin_symbol_name));
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

void VM::errorOccurred(const char* file, int line, const char* message) {
  fprintf(stderr, "error occurred: %s:%d: %s\n", file, line, message);
  currentEvalFrame->dump(stderr);
  exit(1);
}
