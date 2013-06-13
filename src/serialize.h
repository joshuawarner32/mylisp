#include <stddef.h>

class SerializedData {
public:
  enum Tag {
    NIL = 0,
    CONS,
    STRING,
    INTEGER,
    SYMBOL,
    BUILTIN,
    BOOL_TRUE,
    BOOL_FALSE,
    LAMBDA,
  };
};

class Data {
public:
  const char* data;
  size_t length;

  Data(const char* data, size_t length):
    data(data),
    length(length) {}
};

class StringBuffer {
public:
  size_t bufCapacity;
  char* buf;
  size_t used;

  StringBuffer();
  ~StringBuffer();

  void ensure(size_t len);
  void append(const char* text, size_t length);
  void append(char ch);
  char* str();
};

class Value;
class VM;
class StringBuffer;

void writeInt(StringBuffer& buf, int value);

int readInt(const char*& data);

void serializeTo(StringBuffer& buf, Value value);

Data serialize(Value value);

Value deserializeFrom(VM& vm, const char*& data);

Value deserialize(VM& vm, const char* data);
