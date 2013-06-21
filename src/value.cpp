#include <stdio.h>
#include <string.h>

#include "vm.h"

String::String(const char* value): String(value, strlen(value)) {}

bool String::operator == (const String& other) const {
  return length == other.length &&
    memcmp(text, other.text, length) == 0;
}

void* Object::operator new (size_t size, VM& vm) {
  return vm.alloc(size);
}

Cons& Value::asCons(VM& vm) const {
  VM_EXPECT(vm, isCons());
  return asConsUnsafe();
}

String& Value::asSymbol(VM& vm) const {
  VM_EXPECT(vm, isSymbol());
  return asSymbolUnsafe();
}

String& Value::asString(VM& vm) const {
  VM_EXPECT(vm, isString());
  return asStringUnsafe();
}

bool& Value::asBool(VM& vm) const {
  VM_EXPECT(vm, isBool());
  return asBoolUnsafe();
}

int& Value::asInteger(VM& vm) const {
  VM_EXPECT(vm, isInteger());
  return asIntegerUnsafe();
}

Lambda& Value::asLambda(VM& vm) const {
  VM_EXPECT(vm, isLambda());
  return asLambdaUnsafe();
}


Value cons_first(VM& vm, Value o) {
  VM_EXPECT(vm, o.isCons());
  return o->as_cons.first;
}

Value cons_rest(VM& vm, Value o) {
  VM_EXPECT(vm, o.isCons());
  return o->as_cons.rest;
}

size_t list_length(Value list) {
  size_t ret = 0;
  while(!list.isNil()) {
    ret++;
    ASSERT(list.isCons());
    list = list->as_cons.rest;
  }
  return ret;
}

BuiltinFunc builtin_func(Value o) {
  EXPECT(o.isBuiltin());
  return o->as_builtin.func;
}

const char* builtin_name(Value o) {
  EXPECT(o.isBuiltin());
  return o->as_builtin.name;
}

Value make_builtin(VM& vm, const char* name, BuiltinFunc func) {
  Value o = new(vm) Object(Object::Type::Builtin);
  o->as_builtin.name = name;
  o->as_builtin.func = func;
  return o;
}

Value make_lambda(VM& vm, Value params, Value body, Value env) {
  Value o = new(vm) Object(Object::Type::Lambda);
  o->as_lambda.params = params;
  o->as_lambda.body = body;
  o->as_lambda.env = env;
  return o;
}

bool Value::operator == (const Value& other) const {
  if(obj == other.obj) {
    return true;
  }
  if(obj->type != other.obj->type) {
    return false;
  }
  switch(obj->type) {
  case Object::Type::Nil:
    ASSERT(other.isNil() == (obj == other.obj));
    return other.isNil();
  case Object::Type::Cons: {
    Cons ca = asConsUnsafe();
    Cons cb = other.asConsUnsafe();
    return ca.first == cb.first && ca.rest == cb.rest;
  } break;
  case Object::Type::String:
    return asStringUnsafe() == other.asStringUnsafe();
  case Object::Type::Integer:
    return asIntegerUnsafe() == other.asIntegerUnsafe();
  case Object::Type::Bool:
    ASSERT( (obj == other.obj) == (asBoolUnsafe() == other.asBoolUnsafe()) );
    // fallthrough
  case Object::Type::Symbol:
  case Object::Type::Builtin:
  case Object::Type::Lambda:
    return obj == other.obj;
  default:
    EXPECT(0);
    return false;
  }
}

Value map_lookup(VM& vm, Map map, Value key) {
  Value p = map;
  while(!p.isNil()) {
    Value item = cons_first(vm, p);
    if(cons_first(vm, item) == key) {
      return cons_rest(vm, item);
    } else {
      p = cons_rest(vm, p);
    }
  }
  VM_ERROR(vm, "lookup failed");
  return 0;
}
