#include "transform.h"
#include "serialize.h"
#include "vm.h"

extern const char binary_boot_data[];

Value eval(VM& vm, Value o, Map env);

Transformer::Transformer(VM& vm, const char* impl):
  data(eval(vm, deserialize(vm, impl ? impl : binary_boot_data), vm.nil).getObj()),
  vm(vm) {}

Value Transformer::transform(Value input) {
  Value quoted_input = vm.List(vm.syms.quote, input);
  Value transformed = eval(vm, vm.List(data, quoted_input), vm.nil);
  return transformed;
}