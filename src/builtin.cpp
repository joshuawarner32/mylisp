#include "builtin.h"
#include "vm.h"
#include "serialize.h"
#include "string.h"

Value builtin_add(VM& vm, Value args) {
  int res = 0;
  while(!args.isNil()) {
    Value a = cons_first(vm, args);
    args = cons_rest(vm, args);
    res += integer_value(a);
  }
  return vm.Integer(res);
}

Value builtin_sub(VM& vm, Value args) {
  int res;
  if(!args.isNil()) {
    Value a = cons_first(vm, args);
    args = cons_rest(vm, args);
    if(args.isNil()) {
      return vm.Integer(-integer_value(a));
    }
    res = integer_value(a);
  }
  while(!args.isNil()) {
    Value a = cons_first(vm, args);
    args = cons_rest(vm, args);
    res -= integer_value(a);
  }
  return vm.Integer(res);
}

Value builtin_mul(VM& vm, Value args) {
  int res = 1;
  while(!args.isNil()) {
    Value a = cons_first(vm, args);
    args = cons_rest(vm, args);
    res *= integer_value(a);
  }
  return vm.Integer(res);
}

Value builtin_div(VM& vm, Value args) {
  EXPECT(!args.isNil());
  int res = integer_value(cons_first(vm, args));
  args = cons_rest(vm, args);
  while(!args.isNil()) {
    Value a = cons_first(vm, args);
    args = cons_rest(vm, args);
    res /= integer_value(a);
  }
  return vm.Integer(res);
}

template<class Func>
static Value expandArgs2(VM& vm, Value args, Func func) {
  Value a = cons_first(vm, args);
  args = cons_rest(vm, args);
  Value b = cons_first(vm, args);
  VM_EXPECT(vm, cons_rest(vm, args).isNil());
  return func(a, b);
}

Value builtin_modulo(VM& vm, Value args) {
  return expandArgs2(vm, args, [&vm] (Value a, Value b) {
    return vm.Integer(
      integer_value(a) % integer_value(b));
  });
}

Value builtin_is_cons(VM& vm, Value args) {
  Value arg = cons_first(vm, args);
  VM_EXPECT(vm, cons_rest(vm, args).isNil());
  return arg.isCons() ? vm.true_ : vm.false_;
}

Value builtin_cons(VM& vm, Value args) {
  return expandArgs2(vm, args, [&vm] (Value first, Value rest) {
    return vm.Cons(first, rest);
  });
}

Value builtin_first(VM& vm, Value args) {
  Value arg = cons_first(vm, args);
  VM_EXPECT(vm, cons_rest(vm, args).isNil());
  return cons_first(vm, arg);
}

Value builtin_rest(VM& vm, Value args) {
  Value arg = cons_first(vm, args);
  VM_EXPECT(vm, cons_rest(vm, args).isNil());
  return cons_rest(vm, arg);
}

Value builtin_is_symbol(VM& vm, Value args) {
  Value arg = cons_first(vm, args);
  VM_EXPECT(vm, cons_rest(vm, args).isNil());
  return arg.isSymbol() ? vm.true_ : vm.false_;
}

Value builtin_is_equal(VM& vm, Value args) {
  return expandArgs2(vm, args, [&vm] (Value a, Value b) {
    return vm.Bool(obj_equal(a, b));
  });
}

Value builtin_is_nil(VM& vm, Value args) {
  Value arg = cons_first(vm, args);
  VM_EXPECT(vm, cons_rest(vm, args).isNil());
  return arg.isNil() ? vm.true_ : vm.false_;
}

Value builtin_is_int(VM& vm, Value args) {
  Value arg = cons_first(vm, args);
  VM_EXPECT(vm, cons_rest(vm, args).isNil());
  return arg.isInteger() ? vm.true_ : vm.false_;
}

Value builtin_is_str(VM& vm, Value args) {
  Value arg = cons_first(vm, args);
  VM_EXPECT(vm, cons_rest(vm, args).isNil());
  return arg.isString() ? vm.true_ : vm.false_;
}

Value builtin_concat(VM& vm, Value args) {
  StringBuffer buf;
  while(!args.isNil()) {
    Value a = cons_first(vm, args);
    args = cons_rest(vm, args);
    buf.append(string_value(a));
  }
  return make_string(vm, buf.str());
}

static Value _split(VM& vm, const char* str, Value indexes) {
  if(indexes.isNil()) {
    return vm.Cons(make_string(vm, str), vm.nil);
  } else {
    int l = integer_value(cons_first(vm, indexes));
    if(l > (int)strlen(str)) {
      VM_ERROR(vm, "string index out of bounds");
    }
    return vm.Cons(
      make_string(vm, strndup(str, l)),
      _split(vm, str + l, cons_rest(vm, indexes)));
  }
}

Value builtin_split(VM& vm, Value args) {
  return _split(vm,
    string_value(cons_first(vm, args)),
    cons_rest(vm, args));
}

Value builtin_symbol_name(VM& vm, Value args) {
  Value arg = cons_first(vm, args);
  VM_EXPECT(vm, cons_rest(vm, args).isNil());
  return make_string(vm, symbol_name(arg));
}
