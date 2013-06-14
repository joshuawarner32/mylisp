#ifndef MYLISP_VALUE_H_
#define MYLISP_VALUE_H_

#include <stddef.h>

class VM;

class Value;

typedef Value (*BuiltinFunc)(VM& vm, Value args);

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
    struct {
      Object* first;
      Object* rest;
    } as_cons;
    String as_string;
    struct {
      int value;
    } as_integer;
    String as_symbol;
    struct {
      const char* name;
      BuiltinFunc func;
    } as_builtin;
    bool as_bool;
    struct {
      Object* params;
      Object* body;
      Object* env;
    } as_lambda;
  };

  Object(Type type): type(type) {}

  inline void* operator new (size_t size, VM& vm);
};

class Value {
private:
  Object* obj;

public:
  inline Value(): obj(0) {}
  inline Value(Object* obj): obj(obj) {}

  inline Object*& getObj() { return obj; }
  inline Object* operator -> () { return obj; }
  inline bool operator == (const Value& other) const { return obj == other.obj; }
  inline bool operator != (const Value& other) const { return obj != other.obj; }

  inline bool isNil() const { return obj->type == Object::Type::Nil; }
  inline bool isCons() const { return obj->type == Object::Type::Cons; }
  inline bool isString() const { return obj->type == Object::Type::String; }
  inline bool isInteger() const { return obj->type == Object::Type::Integer; }
  inline bool isSymbol() const { return obj->type == Object::Type::Symbol; }
  inline bool isBuiltin() const { return obj->type == Object::Type::Builtin; }
  inline bool isBool() const { return obj->type == Object::Type::Bool; }
  inline bool isLambda() const { return obj->type == Object::Type::Lambda; }


  inline String& asStringUnsafe() const { return obj->as_string; }
  inline String& asSymbolUnsafe() const { return obj->as_symbol; }
  inline bool& asBoolUnsafe() const { return obj->as_bool; }

  String& asString(VM& vm) const;
  String& asSymbol(VM& vm) const;
  bool& asBool(VM& vm) const;

  operator bool () const = delete;
};

Value make_builtin(VM& vm, const char* name, BuiltinFunc func);

Value cons_first(VM& vm, Value o);

Value cons_rest(VM& vm, Value o);

size_t list_length(Value list);

Value list_prepend_n_objs(VM& vm, size_t len, Value obj, Value list);

int integer_value(VM& vm, Value o);

typedef Value Map;

Value map_lookup(VM& vm, Map map, Value key);

BuiltinFunc builtin_func(Value o);

const char* builtin_name(Value o);

Value make_builtin(VM& vm, const char* name, BuiltinFunc func);

Value lambda_params(Value o);

Value lambda_body(Value o);

Value lambda_env(Value o);

Value make_lambda(VM& vm, Value params, Value body, Value env);

bool obj_equal(Value a, Value b);

bool obj_mentions_symbol(Value obj, Value symbol);

#endif