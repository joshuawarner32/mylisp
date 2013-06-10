
#include "stream.h"

class Value;
class Object;
class VM;

class PrettyPrinter {
public:
  Object* data;
  VM& vm;

  PrettyPrinter(VM& vm, const char* impl = 0);

  void print(Value value, int indent = 0, StandardStream stream = StandardStream::StdOut);
};