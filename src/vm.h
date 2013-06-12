#include <stdio.h>

#include "value.h"
#include "stream.h"

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
    if(!(expected)) { \
      VM_ERROR(vm, #expected); \
    } \
  } while(0)

class Syms {
public:
#define SYM(cpp, lisp) Value cpp;
  #include "symbols.inc.h"
#undef SYM
  int dummy_value;

  Syms(VM& vm);
};

struct heap_block_t;
class EvalFrame;

class Transformer;
class PrettyPrinter;

class VM {
private:
  size_t heap_block_size;
  heap_block_t* heap;

  Transformer* transformer;
  PrettyPrinter* prettyPrinter;

public:
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
    Value builtin_is_str;
    Value builtin_concat;
    Value builtin_split;
    Value builtin_symbol_name;
    Value builtin_constructor;
  } objs;

  Syms syms;

  Value core_imports;

  EvalFrame* currentEvalFrame = 0;

  VM(size_t heap_block_size = 4096);
  ~VM();

  void* alloc(size_t size);

  Value Cons(Value first, Value rest);

  inline Value List() { return nil; }

  template<class T, class... TS>
  Value List(T t, TS... ts) {
    return Cons(t, List(ts...));
  }

  Value Symbol(const char* name);
  Value Integer(int value);
  inline Value Bool(bool value) { return value ? true_ : false_; }

  void print(Value value, int indent = 0, StandardStream stream = StandardStream::StdOut);

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

  void dump(StandardStream stream);
};
