#include "transform.h"
#include "serialize.h"
#include "vm.h"

extern const char binary_boot_data[];

Value eval(VM& vm, Value o, Map env);

Transformer::Transformer(VM& vm, const char* impl):
  data(eval(vm, deserialize(vm, impl ? impl : binary_boot_data), vm.nil).getObj()),
  vm(vm) {}

Value Transformer::transform(Value input) {
  Value quoted_input = make_list(vm, vm.syms.quote, input);
  Value transformed = eval(vm, make_list(vm, data, quoted_input), vm.nil);
  return transformed;
}