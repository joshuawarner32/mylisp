#include "prettyprint.h"
#include "serialize.h"
#include "vm.h"

extern const char binary_prettyprint_data[];

Value eval(VM& vm, Value o, Map env);

PrettyPrinter::PrettyPrinter(VM& vm, const char* impl):
  data(eval(vm, deserialize(vm, impl ? impl : binary_prettyprint_data), vm.nil).getObj()),
  vm(vm) {}

void PrettyPrinter::print(Value value, int indent) {
  Value quoted_input = make_list(vm, vm.syms.quote, value);
  Value str = eval(vm, make_list(vm, data, quoted_input), vm.nil);
  const char* data = string_value(str);
  printf("%s", data);
}