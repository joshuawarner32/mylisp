
#include <stddef.h>

class VM;

class Value;

typedef Value (*BuiltinFunc)(VM& vm, Value args);

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
    struct {
      const char* value;
    } as_string;
    struct {
      int value;
    } as_integer;
    struct {
      const char* name;
    } as_symbol;
    struct {
      const char* name;
      BuiltinFunc func;
    } as_builtin;
    struct {
      bool value;
    } as_bool;
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

  operator bool () const = delete;
};

Value make_builtin(VM& vm, const char* name, BuiltinFunc func);

Value cons_first(VM& vm, Value o);

Value cons_rest(VM& vm, Value o);

size_t list_length(Value list);

Value list_prepend_n_objs(VM& vm, size_t len, Value obj, Value list);

Value make_string(VM& vm, const char* value);

const char* string_value(Value o);

Value make_integer(VM& vm, int value);

int integer_value(Value o);

const char* symbol_name(Value o);

typedef Value Map;

Value map_lookup(VM& vm, Map map, Value key);

Value make_symbol_with_length(VM& vm, const char* begin, size_t length);

BuiltinFunc builtin_func(Value o);

const char* builtin_name(Value o);

Value make_builtin(VM& vm, const char* name, BuiltinFunc func);

bool bool_value(Value o);

Value lambda_params(Value o);

Value lambda_body(Value o);

Value lambda_env(Value o);

Value make_lambda(VM& vm, Value params, Value body, Value env);

bool obj_equal(Value a, Value b);

bool obj_mentions_symbol(Value obj, Value symbol);