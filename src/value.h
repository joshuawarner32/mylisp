#ifndef MYLISP_VALUE_H_
#define MYLISP_VALUE_H_

#include <stddef.h>

class VM;

class Value;

typedef Value (*BuiltinFunc)(VM& vm, Value args);

class Object;

class String {
public:
  const char* text;
  size_t length;

  inline String(const char* text, size_t length): text(text), length(length) {}
  String(const char* text);

  inline String substr(size_t begin, size_t length) const {
    return String(text + begin, length);
  }

  bool operator == (const String& other) const;
};

class Cons;
class Lambda;

class Value {
private:
  Object* obj;

public:
  inline Value(): obj(0) {}
  inline Value(Object* obj): obj(obj) {}

  inline Object*& getObj() { return obj; }
  inline Object* operator -> () { return obj; }
  bool operator == (const Value& other) const;
  inline bool operator != (const Value& other) const { return !(*this == other); }

  inline bool isNil() const;
  inline bool isCons() const;
  inline bool isString() const;
  inline bool isInteger() const;
  inline bool isSymbol() const;
  inline bool isBuiltin() const;
  inline bool isBool() const;
  inline bool isLambda() const;

  inline Cons& asConsUnsafe() const;
  inline String& asStringUnsafe() const;
  inline String& asSymbolUnsafe() const;
  inline bool& asBoolUnsafe() const;
  inline int& asIntegerUnsafe() const;
  inline Lambda& asLambdaUnsafe() const;

  Cons& asCons(VM& vm) const;
  String& asString(VM& vm) const;
  String& asSymbol(VM& vm) const;
  bool& asBool(VM& vm) const;
  int& asInteger(VM& vm) const;
  Lambda& asLambda(VM& vm) const;

  operator bool () const = delete;
};

class Cons {
public:
  Value first;
  Value rest;
};

class Lambda {
public:
  Value params;
  Value body;
  Value env;
};

class Object {
public:
  enum class Type {
    Nil,
    Cons,
    String,
    Integer,
    Symbol,
    Builtin,
    Bool,
    Lambda
  };

  Type type;

  union {
    Cons as_cons;
    String as_string;
    int as_integer;
    String as_symbol;
    struct {
      const char* name;
      BuiltinFunc func;
    } as_builtin;
    bool as_bool;
    Lambda as_lambda;
  };

  Object(Type type): type(type) {}

  inline void* operator new (size_t size, VM& vm);
};

bool Value::isNil() const { return obj->type == Object::Type::Nil; }
bool Value::isCons() const { return obj->type == Object::Type::Cons; }
bool Value::isString() const { return obj->type == Object::Type::String; }
bool Value::isInteger() const { return obj->type == Object::Type::Integer; }
bool Value::isSymbol() const { return obj->type == Object::Type::Symbol; }
bool Value::isBuiltin() const { return obj->type == Object::Type::Builtin; }
bool Value::isBool() const { return obj->type == Object::Type::Bool; }
bool Value::isLambda() const { return obj->type == Object::Type::Lambda; }

Cons& Value::asConsUnsafe() const { return obj->as_cons; }
String& Value::asStringUnsafe() const { return obj->as_string; }
String& Value::asSymbolUnsafe() const { return obj->as_symbol; }
bool& Value::asBoolUnsafe() const { return obj->as_bool; }
int& Value::asIntegerUnsafe() const { return obj->as_integer; }
Lambda& Value::asLambdaUnsafe() const { return obj->as_lambda; }

Value make_builtin(VM& vm, const char* name, BuiltinFunc func);

size_t list_length(Value list);

typedef Value Map;

Value map_lookup(VM& vm, Map map, Value key);

BuiltinFunc builtin_func(Value o);

const char* builtin_name(Value o);

Value make_builtin(VM& vm, const char* name, BuiltinFunc func);

Value make_lambda(VM& vm, Value params, Value body, Value env);

typedef Value Map;

Value map_lookup(VM& vm, Map map, Value key);

#endif