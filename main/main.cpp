#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

#include "serialize.h"
#include "vm.h"

// #define ENABLE_DEBUG

#ifndef ENABLE_DEBUG
#define DEBUG_LOG(val)
#else
#define DEBUG_LOG(val) val
#endif

char* loadBytes(const char* file) {
  FILE* f = fopen(file, "rb");
  fseek(f, 0, SEEK_END);
  size_t len = ftell(f);
  fseek(f, 0, SEEK_SET);
  char* buf = (char*) malloc(len + 1);
  fread(buf, 1, len, f);
  fclose(f);
  buf[len] = 0;
  return buf;
}

void saveBytes(const char* file, String data) {
  FILE* f = fopen(file, "wb");
  fwrite(data.text, 1, data.length, f);
  fclose(f);
}

Value parse_file(VM& vm, const char* file, bool multiexpr) {
  char* buf = loadBytes(file);

  Value obj = vm.parse(buf, multiexpr);

  free(buf);

  return obj;
}

Value run_transform_file(VM& vm, const char* file) {
  Value obj = vm.transform(parse_file(vm, file, false));
  // vm.print(obj, printf("transformed:"));
  return obj;
}

int main(int argc, char** argv) {
  const char* transform_file = 0;
  const char* file = 0;
  const char* serialize_to = 0;
  const char* deserialize_from = 0;


  enum {
    START,
    TRANSFORM_FILE,
    SERIALIZE,
    DESERIALIZE,
  } state = START;

  for(int i = 1; i < argc; i++) {
    const char* arg = argv[i];
    switch(state) {
    case START:
      if(strcmp(arg, "--transform-file") == 0) {
        state = TRANSFORM_FILE;
      } else if(strcmp(arg, "--serialize") == 0) {
        state = SERIALIZE;
      } else if(strcmp(arg, "--deserialize") == 0) {
        state = DESERIALIZE;
      } else {
        file = arg;
        state = START;
      }
      break;
    case TRANSFORM_FILE:
      transform_file = arg;
      state = START;
      break;
    case SERIALIZE:
      serialize_to = arg;
      state = START;
      break;
    case DESERIALIZE:
      deserialize_from = arg;
      state = START;
      break;
    }
  }
  
  VM vm;

  if(state != START) {
    fprintf(stderr, "couldn't parse arguments %d\n", state);
  } else {
    if(transform_file) {
      if(file) {
        fprintf(stderr, "can't provide both --transform-file and file to run\n");
      } else {
        Value transformed = run_transform_file(vm, transform_file);
        if(serialize_to) {
          String data = serialize(transformed);
          saveBytes(serialize_to, data);
        } else {
          vm.print(transformed);
        }
        return 0;
      }
    } else if(serialize_to) {
      Value value = parse_file(vm, file, false);
      String data = serialize(value);
      saveBytes(serialize_to, data);
      return 0;
    } else if(deserialize_from) {
      const char* data = loadBytes(deserialize_from);
      Value value = deserialize(vm, data);
      vm.print(value);
      return 0;
    } else if(file) {
      Value transformed = run_transform_file(vm, file);
      // vm.print(transformed);
      Value moduleCall = vm.makeList(transformed, vm.objs.builtin_load_module);
      Value module = eval(vm, moduleCall, vm.nil);
      Value mainCall = vm.makeList(vm.makeList(module, vm.makeList(vm.syms.quote, vm.syms.main)));
      Value result = eval(vm, mainCall, vm.nil);

      vm.print(result);
      return 0;
    } else {
      fprintf(stderr, "must provide either --transform-file or file to run\n");
    }
  }
  return 1;
}

