#include "builtin.h"
#include "vm.h"
#include "serialize.h"
#include "string.h"

Value builtin_add(VM& vm, Value args) {
  int res = 0;
  while(!args.isNil()) {
    Cons c = args.asCons(vm);
    res += c.first.asInteger(vm);
    args = c.rest;
  }
  return vm.makeInteger(res);
}

Value builtin_sub(VM& vm, Value args) {
  int res;
  if(!args.isNil()) {
    Cons c = args.asCons(vm);
    int i = c.first.asInteger(vm);
    args = c.rest;
    if(args.isNil()) {
      return vm.makeInteger(-i);
    }
    res = i;
  }
  while(!args.isNil()) {
    Cons c = args.asCons(vm);
    res -= c.first.asInteger(vm);
    args = c.rest;
  }
  return vm.makeInteger(res);
}

Value builtin_mul(VM& vm, Value args) {
  int res = 1;
  while(!args.isNil()) {
    Cons c = args.asCons(vm);
    res *= c.first.asInteger(vm);
    args = c.rest;
  }
  return vm.makeInteger(res);
}

Value builtin_div(VM& vm, Value args) {
  Cons c = args.asCons(vm);
  int res = c.first.asInteger(vm);
  args = c.rest;
  while(!args.isNil()) {
    c = args.asCons(vm);
    res /= c.first.asInteger(vm);
    args = c.rest;
  }
  return vm.makeInteger(res);
}

template<class Func>
static Value expandArgs2(VM& vm, Value args, Func func) {
  Cons c = args.asCons(vm);
  Value a = c.first;
  c = c.rest.asCons(vm);
  Value b = c.first;
  VM_EXPECT(vm, c.rest.isNil());
  return func(a, b);
}

Value singleValue(VM& vm, Value args) {
  Cons c = args.asCons(vm);
  VM_EXPECT(vm, c.rest.isNil());
  return c.first;
}

Value builtin_modulo(VM& vm, Value args) {
  return expandArgs2(vm, args, [&vm] (Value a, Value b) {
    return vm.makeInteger(
      a.asInteger(vm) % b.asInteger(vm));
  });
}

Value builtin_cons(VM& vm, Value args) {
  return expandArgs2(vm, args, [&vm] (Value first, Value rest) {
    return vm.makeCons(first, rest);
  });
}

Value builtin_first(VM& vm, Value args) {
  return singleValue(vm, args).asCons(vm).first;
}

Value builtin_rest(VM& vm, Value args) {
  return singleValue(vm, args).asCons(vm).rest;
}

Value builtin_is_equal(VM& vm, Value args) {
  return expandArgs2(vm, args, [&vm] (Value a, Value b) {
    return vm.makeBool(a == b);
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
    Cons c = args.asCons(vm);
    buf.append(c.first.asString(vm));
    args = c.rest;
  }
  return vm.makeString(buf.str());
}

static Value _split(VM& vm, const String& str, Value indexes) {
  if(indexes.isNil()) {
    return vm.makeCons(vm.makeString(str), vm.nil);
  } else {
    Cons c = indexes.asCons(vm);
    int l = c.first.asInteger(vm);
    if(l > (int)str.length) {
      VM_ERROR(vm, "string index out of bounds");
    }
    return vm.makeCons(
      vm.makeString(str.substr(0, l)),
      _split(vm, str.substr(l, str.length - l), c.rest));
  }
}

Value builtin_split(VM& vm, Value args) {
  Cons c = args.asCons(vm);
  String str = c.first.asString(vm);
  return _split(vm,
    str,
    c.rest);
}

Value builtin_symbol_name(VM& vm, Value args) {
  return vm.makeString(singleValue(vm, args).asSymbol(vm));
}

Value builtin_make_symbol(VM& vm, Value args) {
  String str = singleValue(vm, args).asString(vm);
  return vm.makeSymbol(str);
}

Value builtin_load_module(VM& vm, Value args) {
  Value moduleName = singleValue(vm, args);
  VM_EXPECT(vm, moduleName.isSymbol());
  return map_lookup_or_else(vm, vm.loaded_modules, moduleName, [&vm](Value name) {
    return vm.loadModule(name);
  });
}

Value builtin_load_from_core(VM& vm, Value args) {
  Value symName = singleValue(vm, args);
  VM_EXPECT(vm, symName.isSymbol());
  return map_lookup(vm, vm.core_imports, symName);
}