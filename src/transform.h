
class Value;
class Object;
class VM;

class Transformer {
public:
  Object* data;
  VM& vm;

  Transformer(VM& vm, const char* impl = 0);

  Value transform(Value v);
};