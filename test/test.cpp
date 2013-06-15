#include <stdio.h>

#include "vm.h"
#include "serialize.h"

void testMakeList() {
  VM vm;

  Value one = vm.makeInteger(1);
  Value two = vm.makeInteger(2);
  Value three = vm.makeInteger(3);

  {
    Value a = vm.makeList();
    Value b = vm.nil;
    EXPECT(obj_equal(a, b));
  }

  {
    Value a = vm.makeList(one);
    Value b = vm.makeCons(one, vm.nil);
    EXPECT(obj_equal(a, b));
  }

  {
    Value a = vm.makeList(one, two);
    Value b = vm.makeCons(one, vm.makeCons(two, vm.nil));
    EXPECT(obj_equal(a, b));
  }

  {
    Value a = vm.makeList(one, two, three);
    Value b = vm.makeCons(one, vm.makeCons(two, vm.makeCons(three, vm.nil)));
    EXPECT(obj_equal(a, b));
  }

}

void testSymbols() {
  VM vm;

  Value a = vm.makeSymbol("a");
  Value b = vm.makeSymbol("b");
  Value a2 = vm.makeSymbol("a");

  Value one = vm.makeInteger(1);
  Value two = vm.makeInteger(2);

  EXPECT(a == a2);
  EXPECT(a != b);
  EXPECT(a2 != b);

  Value map = vm.makeList(
    vm.makeCons(a, one),
    vm.makeCons(b, two));

  EXPECT_INT_EQ(map_lookup(vm, map, a).asInteger(vm), 1);
  EXPECT_INT_EQ(map_lookup(vm, map, b).asInteger(vm), 2);
}

void testParse() {
  VM vm;

  {
    EXPECT_INT_EQ(vm.parse("1").asInteger(vm), 1);
    EXPECT_INT_EQ(vm.parse("42").asInteger(vm), 42);
  }

  {
    EXPECT(vm.parse("#t").asBool(vm) == true);
    EXPECT(vm.parse("#f").asBool(vm) == false);
  }

  {
    Value res = vm.parse("a");
    EXPECT(res.isSymbol());
  }

  {
    Value res = vm.parse("\"a\"");
    EXPECT(obj_equal(res, vm.makeString("a")));
  }

  {
    Value res = vm.parse("(a b)");
    Value a = vm.makeSymbol("a");
    Value b = vm.makeSymbol("b");
    EXPECT(cons_first(vm, res) == a);
    EXPECT(cons_first(vm, cons_rest(vm, res)) == b);
    EXPECT(cons_rest(vm, cons_rest(vm, res)).isNil());
  }
}

void testEval() {
  VM vm;

  Value plus = vm.syms.add;
  Value add = vm.objs.builtin_add;
  Value _if = vm.syms.if_;
  Value _true = vm.true_;
  Value _false = vm.false_;
  Value one = vm.makeInteger(1);
  Value two = vm.makeInteger(2);

  {
    EXPECT_INT_EQ(eval(vm, vm.makeInteger(1), vm.nil).asInteger(vm), 1);
    EXPECT_INT_EQ(eval(vm, vm.makeInteger(42), vm.nil).asInteger(vm), 42);
  }

  {
    Value list = vm.makeList(add, one, two);

    EXPECT_INT_EQ(eval(vm, list, vm.nil).asInteger(vm), 3);
  }

  {
    Value a = vm.makeSymbol("a");
    Value pair = vm.makeCons(a, vm.makeInteger(42));
    Value env = vm.makeCons(pair, vm.nil);
    EXPECT_INT_EQ(eval(vm, a, env).asInteger(vm), 42);
  }

  {
    Value list = vm.makeList(plus, one, two);

    Value pair = vm.makeCons(plus, add);
    Value env = vm.makeCons(pair, vm.nil);
    EXPECT_INT_EQ(eval(vm, list, env).asInteger(vm), 3);
  }

  {
    Value list = vm.makeList(_if, _true, one, two);
    EXPECT_INT_EQ(eval(vm, list, vm.nil).asInteger(vm), 1);
  }

  {
    Value list = vm.makeList(_if, _false, one, two);
    EXPECT_INT_EQ(eval(vm, list, vm.nil).asInteger(vm), 2);
  }

}

void testParseAndEval() {
  VM vm;

  {
    Value plus = vm.syms.add;
    Value add = vm.objs.builtin_add;

    Value pair = vm.makeCons(plus, add);
    Value env = vm.makeCons(pair, vm.nil);

    Value input = vm.parse("(+ 1 2)");
    Value res = eval(vm, input, env);

    EXPECT_INT_EQ(res.asInteger(vm), 3);
  }

  {
    Value scons = vm.syms.Cons;
    Value cons = vm.objs.builtin_cons;

    Value env = vm.makeList(vm.makeCons(scons, cons));

    Value input = vm.parse("((letlambdas ( ((myfunc x y) (cons x y)) ) myfunc) 1 2)");
    Value res = eval(vm, input, env);

    EXPECT(obj_equal(res, vm.makeCons(vm.makeInteger(1), vm.makeInteger(2))));
  }

  {
    Value input = vm.parse("((import core +) 1 2)");
    Value res = eval(vm, input, vm.nil);

    EXPECT_INT_EQ(res.asInteger(vm), 3);
  }

}

void testSerialize() {
  VM vm;

  {
    Value original = vm.nil;
    String serialized = serialize(original);
    Value deserialized = deserialize(vm, serialized.text);
    EXPECT(obj_equal(deserialized, original));
  }

  {
    Value original = vm.makeInteger(0);
    String serialized = serialize(original);
    Value deserialized = deserialize(vm, serialized.text);
    EXPECT(obj_equal(deserialized, original));
  }

  {
    Value original = vm.makeInteger(1);
    String serialized = serialize(original);
    Value deserialized = deserialize(vm, serialized.text);
    EXPECT(obj_equal(deserialized, original));
  }

  {
    Value a = vm.makeSymbol("some-random-symbol");
    Value original = vm.makeList(a,
      vm.makeInteger(1),
      vm.makeInteger(42),
      vm.makeInteger(129),
      vm.makeInteger(4),
      vm.makeInteger(5));
    String serialized = serialize(original);
    Value deserialized = deserialize(vm, serialized.text);
    EXPECT(obj_equal(deserialized, original));
  }
}

void testAll() {
  testMakeList();
  testSymbols();
  testParse();
  testEval();
  testParseAndEval();
  testSerialize();
}

int main(int argc, char** argv) {
  testAll();
  return 0;
}