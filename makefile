MAKEFLAGS=-s

sources = $(wildcard src/*.cpp)
headers = $(wildcard src/*.h)
objects = $(foreach x,$(sources),$(patsubst src/%.cpp,build/%.cpp.o,$(x)))

executable = build/mylisp

.PHONY: run boot test cloc

run: $(executable) test
	echo "running"
	${<} boot/boot.ss src/test.ss

boot: $(executable) test
	echo "booting 1"
	${<} boot/boot.ss src/boot.ss > build/boot-1.ss
	echo "booting 2"
	${<} build/boot-1.ss src/boot.ss > build/boot-2.ss
	diff -wuq build/boot-1.ss build/boot-2.ss && cp build/boot-2.ss boot/boot.ss && echo "success"

test: $(executable)
	echo "running tests"
	${<} --test

cloc: $(wildcard src/*.cpp) $(wildcard src/*.h)
	printf "lines of c++: "
	(cloc $(^) --quiet --sql=-; echo "select sum(nCode) from t where Language in ('C++', 'C/C++ Header');")|sqlite3 :memory:

$(executable): $(objects)
	mkdir -p $(dir ${@})
	printf "linking   %12s %12s      %12s %12s\n" "" "" $(dir ${@}) $(notdir ${@})
	clang++ -O0 -g3 -o ${@} ${^}

$(objects): build/%.cpp.o: src/%.cpp $(headers)
	mkdir -p $(dir ${@})
	printf "compiling %12s %12s   -> %12s %12s\n" $(dir ${<}) $(notdir ${<})  $(dir ${@}) $(notdir ${@})
	clang++ -Wall -Werror -Wextra -Wno-unused-parameter -O0 -g3 -c -std=c++11 -o ${@} ${<}

.PHONY: clean
clean:
	echo "removing build"
	rm -rf build