#include <stdio.h>

#include "value.h"

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

#define VM_ERROR(vm, message) \
  vm.errorOccurred(__FILE__, __LINE__, message)

#define VM_EXPECT(vm, expected) \
  do { \
    if(!expected) { \
      VM_ERROR(vm, #expected); \
    } \
  } while(0)

class Syms {
public:
#define SYM_LIST \
  SYM(if_, "if") \
  SYM(letlambdas, "letlambdas") \
  SYM(import, "import") \
  SYM(core, "core") \
  SYM(quote, "quote") \
  SYM(add, "+") \
  SYM(sub, "-") \
  SYM(mul, "*") \
  SYM(div, "/") \
  SYM(modulo, "modulo") \
  SYM(is_cons, "cons?") \
  SYM(cons, "cons") \
  SYM(first, "first") \
  SYM(rest, "rest") \
  SYM(is_symbol, "sym?") \
  SYM(is_equal, "eq?") \
  SYM(is_nil, "nil?") \
  SYM(is_int, "int?") \
  SYM(concat, "concat") \
  SYM(symbol_name, "sym-name")

#define SYM(cpp, lisp) Value cpp;
  SYM_LIST
#undef SYM
  int dummy_value;

  Syms(VM& vm):
#define SYM(cpp, lisp) cpp(make_symbol(vm, lisp)),
  SYM_LIST
#undef SYM
  dummy_value(0) {}

#undef SYM_LIST
};

struct heap_block_t;
class EvalFrame;

class VM {
public:
  size_t heap_block_size;
  heap_block_t* heap;

  Value nil;
  Value true_;
  Value false_;

  Value symList;
  struct {
    Value builtin_add;
    Value builtin_sub;
    Value builtin_mul;
    Value builtin_div;
    Value builtin_modulo;
    Value builtin_is_cons;
    Value builtin_cons;
    Value builtin_first;
    Value builtin_rest;
    Value builtin_is_symbol;
    Value builtin_is_equal;
    Value builtin_is_nil;
    Value builtin_is_int;
    Value builtin_concat;
    Value builtin_symbol_name;
  } objs;

  Syms syms;

  Value core_imports;

  EvalFrame* currentEvalFrame = 0;

  VM(size_t heap_block_size = 4096);
  ~VM();

  void* alloc(size_t size);

  Value Cons(Value first, Value rest);

  void errorOccurred(const char* file, int line, const char* message);
};

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