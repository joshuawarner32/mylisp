
class Value;
class VM;

Value builtin_add(VM& vm, Value args);
Value builtin_sub(VM& vm, Value args);
Value builtin_mul(VM& vm, Value args);
Value builtin_div(VM& vm, Value args);
Value builtin_modulo(VM& vm, Value args);
Value builtin_is_cons(VM& vm, Value args);
Value builtin_cons(VM& vm, Value args);
Value builtin_first(VM& vm, Value args);
Value builtin_rest(VM& vm, Value args);
Value builtin_is_symbol(VM& vm, Value args);
Value builtin_is_equal(VM& vm, Value args);
Value builtin_is_nil(VM& vm, Value args);
Value builtin_is_int(VM& vm, Value args);
Value builtin_concat(VM& vm, Value args);
Value builtin_symbol_name(VM& vm, Value args);