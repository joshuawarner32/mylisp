#include "interpret.h"
#include "vm.h"


Value interpret(VM& vm, Frame* frame) {
  while(true) {
    unsigned char instr = *(frame->ip++);
    switch(instr) {
    case Opcode::Constant: {
      unsigned char cst = *(frame->ip++);
      unsigned char dst = *(frame->ip++);
      frame->values[dst] = frame->constants[cst];
    } break;
    case Opcode::Add: {
      unsigned char a = *(frame->ip++);
      unsigned char b = *(frame->ip++);
      unsigned char c = *(frame->ip++);
      frame->values[c] = vm.makeInteger(
        frame->values[a].asInteger(vm) +
        frame->values[b].asInteger(vm));
    } break;
    case Opcode::Subtract: {
      unsigned char a = *(frame->ip++);
      unsigned char b = *(frame->ip++);
      unsigned char c = *(frame->ip++);
      frame->values[c] = vm.makeInteger(
        frame->values[a].asInteger(vm) -
        frame->values[b].asInteger(vm));
    } break;
    case Opcode::Multiply: {
      unsigned char a = *(frame->ip++);
      unsigned char b = *(frame->ip++);
      unsigned char c = *(frame->ip++);
      frame->values[c] = vm.makeInteger(
        frame->values[a].asInteger(vm) *
        frame->values[b].asInteger(vm));
    } break;
    case Opcode::Divide: {
      unsigned char a = *(frame->ip++);
      unsigned char b = *(frame->ip++);
      unsigned char c = *(frame->ip++);
      frame->values[c] = vm.makeInteger(
        frame->values[a].asInteger(vm) /
        frame->values[b].asInteger(vm));
    } break;
    case Opcode::Modulo: {
      unsigned char a = *(frame->ip++);
      unsigned char b = *(frame->ip++);
      unsigned char c = *(frame->ip++);
      frame->values[c] = vm.makeInteger(
        frame->values[a].asInteger(vm) %
        frame->values[b].asInteger(vm));
    } break;
    case Opcode::Call: {
      // unsigned char idx = *(frame->ip++);
      return vm.makeInteger(5);
    } break;
    }
  }
}