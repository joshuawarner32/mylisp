#include <stdlib.h>
#include <string.h>

#include "serialize.h"
#include "vm.h"

StringBuffer::StringBuffer():
  bufCapacity(10),
  buf((char*)malloc(bufCapacity)),
  used(0)
{
  buf[0] = '\0';
}

StringBuffer::~StringBuffer() {
  free(buf);
}

void StringBuffer::ensure(size_t len) {
  if(len + used + 1 > bufCapacity) {
    buf = (char*) realloc(buf, bufCapacity = (len + used + 1) * 2);
  }
}

void StringBuffer::append(const char* text) {
  size_t len = strlen(text);
  ensure(len);
  memcpy(buf + used, text, len + 1);
  used += len;
}

void StringBuffer::append(char ch) {
  ensure(1);
  buf[used++] = ch;
  buf[used] = 0;
}

char* StringBuffer::str() {
  char* d = (char*) malloc(used + 1);
  memcpy(d, buf, used + 1);
  return d;
}

void writeInt(StringBuffer& buf, int value) {
  while(true) {
    int bits = value & 0x7f;
    value >>= 7;
    if(value) {
      bits |= 0x80;
    }
    buf.append(bits);
    if(!value) {
      return;
    }
  }
}

int readInt(const char*& data) {
  int ret = 0;
  int shift = 0;
  while(true) {
    int bits = *(data++);
    ret |= ((bits & 0x7f) << shift);
    if(bits & 0x80) {
      shift += 7;
    } else {
      return ret;
    }
  }
}

int builtin_id(Value value) {
  return 0;
}

Value builtin_with_id(VM& vm, int id) {
  return 0;
}

void serializeTo(StringBuffer& buf, Value value) {
  switch(value->type) {
  case Object::Type::Nil:
    buf.append(SerializedData::NIL);
    return;
  case Object::Type::Cons:
    buf.append(SerializedData::CONS);
    serializeTo(buf, value->as_cons.first);
    serializeTo(buf, value->as_cons.rest);
    return;
  case Object::Type::String: {
    buf.append(SerializedData::STRING);
    const char* data = string_value(value);
    writeInt(buf, strlen(data));
    buf.append(data);
  } return;
  case Object::Type::Integer: {
    buf.append(SerializedData::INTEGER);
    int v = integer_value(value);
    writeInt(buf, v);
  } return;
  case Object::Type::Symbol: {
    buf.append(SerializedData::SYMBOL);
    const char* data = symbol_name(value);
    writeInt(buf, strlen(data));
    buf.append(data);
  } return;
  case Object::Type::Builtin:
    buf.append(SerializedData::BUILTIN);
    writeInt(buf, builtin_id(value));
    return;
  case Object::Type::Bool:
    buf.append(bool_value(value) ? SerializedData::BOOL_TRUE : SerializedData::BOOL_FALSE);
    return;
  case Object::Type::Lambda:
    buf.append(SerializedData::LAMBDA);
    serializeTo(buf, value->as_lambda.env);
    serializeTo(buf, value->as_lambda.params);
    serializeTo(buf, value->as_lambda.body);
    return;
  default:
    EXPECT(0);
    return;
  }
}

Data serialize(Value value) {
  StringBuffer buf;
  serializeTo(buf, value);
  return Data(buf.str(), buf.used);
}

Value deserializeFrom(VM& vm, const char*& data) {
  static int depth = 0;
  char ch = *(data++);
  switch(ch) {
  case SerializedData::NIL:
    return vm.nil;
  case SerializedData::CONS: {
    depth++;
    Value first = deserializeFrom(vm, data);
    Value rest = deserializeFrom(vm, data);
    depth--;
    return vm.Cons(first, rest);
  } break;
  case SerializedData::STRING: {
    int len = readInt(data);
    char* buf = (char*) malloc(len + 1);
    memcpy(buf, data, len);
    buf[len] = 0;
    data += len;
    return make_string(vm, buf);
  } break;
  case SerializedData::INTEGER: {
    int res = readInt(data);
    return make_integer(vm, res);
  } break;
  case SerializedData::SYMBOL: {
    int len = readInt(data);
    char* buf = (char*) malloc(len + 1);
    memcpy(buf, data, len);
    buf[len] = 0;
    data += len;
    return vm.Symbol(buf);
  } break;
  case SerializedData::BUILTIN: {
    int id = readInt(data);
    return builtin_with_id(vm, id);
  } break;
  case SerializedData::BOOL_TRUE:
    return vm.true_;
  case SerializedData::BOOL_FALSE:
    return vm.false_;
  case SerializedData::LAMBDA: {
    Value env = deserializeFrom(vm, data);
    Value params = deserializeFrom(vm, data);
    Value body = deserializeFrom(vm, data);
    return make_lambda(vm, params, body, env);
  } break;
  }
  EXPECT(0);
  return 0;
}

Value deserialize(VM& vm, const char* data) {
  return deserializeFrom(vm, data);
}