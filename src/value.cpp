#include <stdio.h>
#include <string.h>

#include "vm.h"

String::String(const char* value): String(value, strlen(value)) {}

bool String::operator == (const String& other) const {
  return length == other.length &&
    memcmp(text, other.text, length) == 0;
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