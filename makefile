MAKEFLAGS=-s

sources = $(wildcard src/*.cpp)
headers = $(wildcard src/*.h)
objects = $(foreach x,$(sources),$(patsubst src/%.cpp,build/%.cpp.o,$(x)))

executable = build/mylisp

.PHONY: run boot test cloc

run: $(executable) test
	echo "running"
	${<} src/test.ss

boot: build/boot-1/transform.ss.bin build/boot-1/prettyprint.ss.bin build/boot-1/parse.ss.bin
	cp ${^} boot

build/boot-1/%.ss.bin: $(executable) src/%.ss
	mkdir -p $(dir ${@})
	echo "writing ${@}"
	${<} --transform-file $(word 2, ${^}) --serialize ${@}

build/transform-data.gen.c: boot/transform.ss.bin
	mkdir -p $(dir ${@})
	echo "const char binary_transform_data[] = {" > ${@}
	xxd -i < ${<} >> ${@}
	echo "};" >> ${@}

build/prettyprint-data.gen.c: boot/prettyprint.ss.bin
	mkdir -p $(dir ${@})
	echo "const char binary_prettyprint_data[] = {" > ${@}
	xxd -i < ${<} >> ${@}
	echo "};" >> ${@}

build/parse-data.gen.c: boot/parse.ss.bin
	mkdir -p $(dir ${@})
	echo "const char binary_parse_data[] = {" > ${@}
	xxd -i < ${<} >> ${@}
	echo "};" >> ${@}

%.o: %.gen.c
	clang ${<} -c -o ${@}

test: $(executable)
	echo "running tests"
	${<} --test

cloc: $(wildcard src/*.cpp) $(wildcard src/*.h)
	printf "lines of c++: "
	(cloc $(^) --quiet --sql=-; echo "select sum(nCode) from t where Language in ('C++', 'C/C++ Header');")|sqlite3 :memory:

$(executable): $(objects) build/transform-data.o build/prettyprint-data.o build/parse-data.o
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