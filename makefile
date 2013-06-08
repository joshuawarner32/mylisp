MAKEFLAGS=-s

sources = $(wildcard src/*.c)
headers = $(wildcard src/*.h)
objects = $(foreach x,$(sources),$(patsubst src/%.c,build/%.c.o,$(x)))

executable = build/mylisp

.PHONY: boot test cloc

boot: $(executable) test
	echo "booting"
	${<} src/boot.ss

test: $(executable)
	echo "running tests"
	${<} --test

cloc: $(wildcard src/*.c) $(wildcard src/*.h)
	printf "lines of c: "
	(cloc $(^) --quiet --sql=-; echo "select sum(nCode) from t where Language in ('C', 'C/C++ Header');")|sqlite3 :memory:

$(executable): $(objects)
	mkdir -p $(dir ${@})
	printf "linking   %12s %12s      %12s %12s\n" "" "" $(dir ${@}) $(notdir ${@})
	clang -O0 -g3 -o ${@} ${^}

$(objects): build/%.c.o: src/%.c $(headers)
	mkdir -p $(dir ${@})
	printf "compiling %12s %12s   -> %12s %12s\n" $(dir ${<}) $(notdir ${<})  $(dir ${@}) $(notdir ${@})
	clang -Wall -Werror -Wextra -Wno-unused-parameter -O0 -g3 -c -std=c11 -o ${@} ${<}

.PHONY: clean
clean:
	echo "removing build"
	rm -rf build