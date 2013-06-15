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

Value lambda_params(Value o) {
  EXPECT(o.isLambda());
  return o->as_lambda.params;
}

Value lambda_body(Value o) {
  EXPECT(o.isLambda());
  return o->as_lambda.body;
}

Value lambda_env(Value o) {
  EXPECT(o.isLambda());
  return o->as_lambda.env;
}

Value make_lambda(VM& vm, Value params, Value body, Value env) {
  Value o = new(vm) Object(Object::Type::Lambda);
  o->as_lambda.params = params.getObj();
  o->as_lambda.body = body.getObj();
  o->as_lambda.env = env.getObj();
  return o;
}

bool obj_equal(Value a, Value b) {
  if(a->type != b->type) {
    return false;
  }
  switch(a->type) {
  case Object::Type::Nil:
    ASSERT(b.isNil() == (a == b));
    return b.isNil();
  case Object::Type::Cons:
    if(obj_equal(a->as_cons.first, a->as_cons.first)) {
      if(obj_equal(a->as_cons.rest, b->as_cons.rest)) {
        return true;
      }
    }
    return false;
  case Object::Type::String:
    return a.asStringUnsafe() == b.asStringUnsafe();
  case Object::Type::Integer:
    return a.asIntegerUnsafe() == b.asIntegerUnsafe();
  case Object::Type::Symbol:
  case Object::Type::Builtin:
    return a == b;
  case Object::Type::Bool:
    ASSERT( (a == b) == (a.asBoolUnsafe() == b.asBoolUnsafe()) );
    return a == b;
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