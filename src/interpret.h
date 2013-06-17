
class Value;
class VM;

class Opcode {
public:
  enum {
    Constant,
    Add,
    Subtract,
    Multiply,
    Divide,
    Modulo,
    Call
  };
};

class Frame {
public:
  Frame* lexicalScope;
  unsigned size;
  Value* values;
  const unsigned char* ip;
  Value* constants;
};

Value interpret(VM& vm, Frame* frame);