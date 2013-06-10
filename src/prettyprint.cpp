#include "prettyprint.h"
#include "serialize.h"
#include "vm.h"

extern const char binary_prettyprint_data[];

Value eval(VM& vm, Value o, Map env);

FILE* streamToFile(StandardStream stream) {
  switch(stream) {
  case StandardStream::StdOut:
    return stdout;
  case StandardStream::StdErr:
    return stderr;
  }
}

PrettyPrinter::PrettyPrinter(VM& vm, const char* impl):
  data(eval(vm, deserialize(vm, impl ? impl : binary_prettyprint_data), vm.nil).getObj()),
  vm(vm) {}

void PrettyPrinter::print(Value value, int indent, StandardStream stream) {
  FILE* s = streamToFile(stream);
  Value quoted_input = make_list(vm, vm.syms.quote, value);
  Value str = eval(vm, make_list(vm, data, quoted_input, make_integer(vm, indent)), vm.nil);
  const char* data = string_value(str);
  fprintf(s, "%s\n", data);
}