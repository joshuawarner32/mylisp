MAKEFLAGS=-s

sources = $(wildcard src/*.cpp)
headers = $(wildcard src/*.h)
objects = $(foreach x,$(sources),$(patsubst src/%.cpp,build/%.cpp.o,$(x)))

executable = build/mylisp

.PHONY: run boot test cloc

run: $(executable) test
	echo "running"
	${<} src/prettyprint.ss

boot: build/boot-1/boot.ss.bin build/boot-2/boot.ss.bin
	diff -q ${^}
	cp build/boot-2/boot.ss.bin boot/boot.ss.bin

define do-boot
	mkdir -p $(dir ${@})
	echo "writing ${@}"
	${<} --transform-file $(word 3, ${^}) --serialize ${@}
endef

build/boot-1/boot.ss.bin: $(executable) boot/boot.ss.bin src/boot.ss
	$(do-boot)
build/boot-2/boot.ss.bin: $(executable) build/boot-1/boot.ss.bin src/boot.ss
	$(do-boot)

build/boot-data.c: boot/boot.ss.bin
	mkdir -p $(dir ${@})
	echo "const char binary_boot_data[] = {" > ${@}
	xxd -i < ${<} >> ${@}
	echo "};" >> ${@}

build/boot-data.o: build/boot-data.c
	clang ${<} -c -o ${@}

test: $(executable)
	echo "running tests"
	${<} --test

cloc: $(wildcard src/*.cpp) $(wildcard src/*.h)
	printf "lines of c++: "
	(cloc $(^) --quiet --sql=-; echo "select sum(nCode) from t where Language in ('C++', 'C/C++ Header');")|sqlite3 :memory:

$(executable): $(objects) build/boot-data.o
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