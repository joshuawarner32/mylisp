Where to begin?

As the saying goes, everyone and their grandmother has an implementation of lisp.  This is largely because of two factors: Lisp is among the easiest languages to implement; and once it has been implemented, it's surprisingly powerful.  I leave the reader to determine whether that power is a good thing.

Before I go further, I would like to suggest reading Peter Michaux's "Scheme from Scratch" series, which largely inspired this one.

In the following posts, I will go through implementing my own, personal variant of lisp.  The goal is to explore some of the challenges in implementing a scheme interpreter from scratch, and more interestingly, implementing significant parts of the VM / compiler in the language itself.  I would encourage you to follow along.  As you do so, don't be afraid to experiment and make the language your own.  I'll note some interesting alternative implementations that come to mind as we go.

The lisp variant I'm implementing bears resemblence to other lisps - it's closer to scheme than common lisp in its minimalism, and many of the same base principles are there - but don't go looking for a clone of an existing langauge.  If that's what you want, go no further.  You have been warned.

In my defence, I've made most of the changes in the name of keeping the implementation simple.  Others I've made eliminate mutability except in very select places, much in the spirit of Clojure.  Here's a summary of the changes:

* No mutable scope, including the file-level scope
* Single internal scope-introducing form: "letlambdas".  Let, lambda, and define forms all compile down to letlambdas.
* MORE

-- c++ style notes

With that, let's get started.

I always begin every C++ project with Hello World, as a quick sanity check of the build system.

main.cpp:
```c++
#include <stdio.h>
int main(int argc, char** argv) {
  printf("Hello World\n");
  return 0;
}
```

run like this:

```bash
clang++ -std=c++11 main.cpp -o mylisp
./mylisp
# Surprise! - prints "Hello World"
```

Now, the first order of business is implementing the core value type, which represents any possible value in the running system.  Because this is a lisp (and thus homoiconic), this is also the type we'll use to represent programs.

```c++
class Object {
public:
  enum class Type {
    Nil,
    Pair,
    Integer,
    Symbol
  };

  Type type;

  inline Object(Type type): type(type) {}

  MORE
};
```

Next, we'll need a class to represent the global state of the VM.  This manages all memory allocated by lisp.  Currently, this is just a simple region / bump-pointer allocator that makes sure all of the memory is cleaned up when the VM is destroyed.  This class also keeps references to symbols that c++ code will need to reference.  Many other implementations take the approach of using a bunch of global state to store this information.  That's a perfectly fine way to go, but I find a reentrant system is almost always preferable to a non-reentrant one.

```c++
class VM {
public:
  MORE
};
```

Let's start writing some tests to verify that everything works:  

```c++
void testStartVMAndAllocate() {
  VM vm;
  Object* obj = new(vm) Object(Object::Type::Nil);
}
```
