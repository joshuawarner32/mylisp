MAKEFLAGS=-s

sources = $(wildcard src/*.cpp)
headers = $(wildcard src/*.h)
objects = $(foreach x,$(sources),$(patsubst src/%.cpp,build/%.cpp.o,$(x)))

executable = build/mylisp

.PHONY: run boot test cloc

run: $(executable) test
	echo "running"
	${<} --transformer boot/boot.ss src/test.ss

boot: build/boot-1/boot.ss build/boot-2/boot.ss
	diff -wuq ${^}
	cp build/boot-2/boot.ss boot/boot.ss

define do-boot
	mkdir -p $(dir ${@})
	echo "booting ${@}"
	${<} --transformer $(word 2, ${^}) --transform-file $(word 3, ${^}) > ${@}
endef

build/boot-1/boot.ss: $(executable) boot/boot.ss src/boot.ss
	$(do-boot)
build/boot-2/boot.ss: $(executable) build/boot-1/boot.ss src/boot.ss
	$(do-boot)

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