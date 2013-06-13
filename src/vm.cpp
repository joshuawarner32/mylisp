#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdio.h>
#include <string.h>

#include "vm.h"
#include "builtin.h"
#include "serialize.h"

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
#define SYM(cpp, lisp) cpp(vm.Symbol(lisp)),
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

  true_->as_bool.value = true;
  false_->as_bool.value = false;

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


  core_imports = List(
    Cons(syms.add, objs.builtin_add),
    Cons(syms.sub, objs.builtin_sub),
    Cons(syms.mul, objs.builtin_mul),
    Cons(syms.div, objs.builtin_div),
    Cons(syms.modulo, objs.builtin_modulo),
    Cons(syms.Cons, objs.builtin_cons),
    Cons(syms.first, objs.builtin_first),
    Cons(syms.rest, objs.builtin_rest),
    Cons(syms.is_equal, objs.builtin_is_equal),
    Cons(syms.concat, objs.builtin_concat),
    Cons(syms.split, objs.builtin_split),
    Cons(syms.ctor, objs.builtin_constructor),
    Cons(syms.make_symbol, objs.builtin_make_symbol),
    Cons(syms.symbol_name, objs.builtin_symbol_name));
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

Value VM::Cons(Value first, Value rest) {
  Value o = new(*this) Object(Object::Type::Cons);
  o->as_cons.first = first.getObj();
  o->as_cons.rest = rest.getObj();
  return o;
}

Value VM::Symbol(const char* name) {
  return make_symbol_with_length(*this, name, strlen(name));
}

Value VM::Integer(int value) {
  Value o = new(*this) Object(Object::Type::Integer);
  o->as_integer.value = value;
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
  if(prettyPrinterImpl.isNil()) {
    prettyPrinterImpl = eval(*this, deserialize(*this, binary_prettyprint_data), nil);
  }
  FILE* s = streamToFile(stream);
  Value quoted_input = List(syms.quote, value);
  Value str = eval(*this, List(prettyPrinterImpl, quoted_input, Integer(indent)), nil);
  const char* data = string_value(str);
  fprintf(s, "%s\n", data);
}

Value VM::transform(Value input) {
  if(transformerImpl.isNil()) {
    transformerImpl = eval(*this, deserialize(*this, binary_transform_data), nil);
  }
  Value quoted_input = List(syms.quote, input);
  Value transformed = eval(*this, List(transformerImpl, quoted_input), nil);
  return transformed;
}

void VM::errorOccurred(const char* file, int line, const char* message) {
  fprintf(stderr, "error occurred: %s:%d: %s\n", file, line, message);
  currentEvalFrame->dump(StandardStream::StdErr);
  exit(1);
}
