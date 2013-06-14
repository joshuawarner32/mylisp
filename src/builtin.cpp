#include "builtin.h"
#include "vm.h"
#include "serialize.h"
#include "string.h"

Value builtin_add(VM& vm, Value args) {
  int res = 0;
  while(!args.isNil()) {
    Value a = cons_first(vm, args);
    args = cons_rest(vm, args);
    res += integer_value(vm, a);
  }
  return vm.makeInteger(res);
}

Value builtin_sub(VM& vm, Value args) {
  int res;
  if(!args.isNil()) {
    Value a = cons_first(vm, args);
    args = cons_rest(vm, args);
    if(args.isNil()) {
      return vm.makeInteger(-integer_value(vm, a));
    }
    res = integer_value(vm, a);
  }
  while(!args.isNil()) {
    Value a = cons_first(vm, args);
    args = cons_rest(vm, args);
    res -= integer_value(vm, a);
  }
  return vm.makeInteger(res);
}

Value builtin_mul(VM& vm, Value args) {
  int res = 1;
  while(!args.isNil()) {
    Value a = cons_first(vm, args);
    args = cons_rest(vm, args);
    res *= integer_value(vm, a);
  }
  return vm.makeInteger(res);
}

Value builtin_div(VM& vm, Value args) {
  EXPECT(!args.isNil());
  int res = integer_value(vm, cons_first(vm, args));
  args = cons_rest(vm, args);
  while(!args.isNil()) {
    Value a = cons_first(vm, args);
    args = cons_rest(vm, args);
    res /= integer_value(vm, a);
  }
  return vm.makeInteger(res);
}

template<class Func>
static Value expandArgs2(VM& vm, Value args, Func func) {
  Value a = cons_first(vm, args);
  args = cons_rest(vm, args);
  Value b = cons_first(vm, args);
  VM_EXPECT(vm, cons_rest(vm, args).isNil());
  return func(a, b);
}

Value singleValue(VM& vm, Value args) {
  Value ret = cons_first(vm, args);
  VM_EXPECT(vm, cons_rest(vm, args).isNil());
  return ret;
}

Value builtin_modulo(VM& vm, Value args) {
  return expandArgs2(vm, args, [&vm] (Value a, Value b) {
    return vm.makeInteger(
      integer_value(vm, a) % integer_value(vm, b));
  });
}

Value builtin_cons(VM& vm, Value args) {
  return expandArgs2(vm, args, [&vm] (Value first, Value rest) {
    return vm.makeCons(first, rest);
  });
}

Value builtin_first(VM& vm, Value args) {
  return cons_first(vm, singleValue(vm, args));
}

Value builtin_rest(VM& vm, Value args) {
  return cons_rest(vm, singleValue(vm, args));
}

Value builtin_is_equal(VM& vm, Value args) {
  return expandArgs2(vm, args, [&vm] (Value a, Value b) {
    return vm.makeBool(obj_equal(a, b));
  });
}

Value builtin_constructor(VM& vm, Value args) {
  switch(singleValue(vm, args)->type) {
  case Object::Type::Nil:       return vm.syms.Nil;
  case Object::Type::Cons:      return vm.syms.Cons;
  case Object::Type::String:    return vm.syms.String;
  case Object::Type::Integer:   return vm.syms.Integer;
  case Object::Type::Symbol:    return vm.syms.Symbol;
  case Object::Type::Builtin:   return vm.syms.Builtin;
  case Object::Type::Bool:      return vm.syms.Bool;
  case Object::Type::Lambda:    return vm.syms.Lambda;
  default:
    EXPECT(0);
    return 0;
  }
}

Value builtin_concat(VM& vm, Value args) {
  StringBuffer buf;
  while(!args.isNil()) {
    const String& s = cons_first(vm, args).asString(vm);
    args = cons_rest(vm, args);
    buf.append(s);
  }
  return vm.makeString(buf.str());
}

static Value _split(VM& vm, const String& str, Value indexes) {
  if(indexes.isNil()) {
    return vm.makeCons(vm.makeString(str), vm.nil);
  } else {
    int l = integer_value(vm, cons_first(vm, indexes));
    if(l > (int)str.length) {
      VM_ERROR(vm, "string index out of bounds");
    }
    return vm.makeCons(
      vm.makeString(str.substr(0, l)),
      _split(vm, str.substr(l, str.length - l), cons_rest(vm, indexes)));
  }
}

Value builtin_split(VM& vm, Value args) {
  String str = cons_first(vm, args).asString(vm);
  return _split(vm,
    str,
    cons_rest(vm, args));
}

Value builtin_symbol_name(VM& vm, Value args) {
  return vm.makeString(singleValue(vm, args).asSymbol(vm));
}

Value builtin_make_symbol(VM& vm, Value args) {
  String str = singleValue(vm, args).asString(vm);
  return vm.makeSymbol(str);
}
