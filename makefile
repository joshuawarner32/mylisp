MAKEFLAGS=-s

vm-sources = $(wildcard src/*.cpp)
vm-headers = $(wildcard src/*.h)
vm-objects = $(foreach x,$(vm-sources),$(patsubst src/%.cpp,build/src/%.cpp.o,$(x)))

test-sources = $(wildcard test/*.cpp)
test-headers = $(wildcard test/*.h)
test-objects = $(foreach x,$(test-sources),$(patsubst test/%.cpp,build/test/%.cpp.o,$(x)))

main-sources = $(wildcard main/*.cpp)
main-headers = $(wildcard main/*.h)
main-objects = $(foreach x,$(main-sources),$(patsubst main/%.cpp,build/main/%.cpp.o,$(x)))

objects = $(vm-objects) $(test-objects) $(main-objects)
headers = $(vm-headers) $(test-headers) $(main-headers)

executable = build/mylisp

test-executable = build/test-mylisp

embed-objects = build/transform-data.o build/prettyprint-data.o build/parse-data.o

.PHONY: run boot test cloc

run: $(executable) test
	echo "running"
	${<} src/test.ss

boot: build/boot-1/parse.ss.bin build/boot-1/transform.ss.bin build/boot-1/prettyprint.ss.bin
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

test: $(test-executable)
	echo "running tests"
	${<}

$(test-executable): $(vm-objects) $(test-objects) $(embed-objects)
	mkdir -p $(dir ${@})
	printf "linking   %12s %12s      %12s %12s\n" "" "" $(dir ${@}) $(notdir ${@})
	clang++ -O0 -g3 -o ${@} ${^}

cloc: $(wildcard src/*.cpp) $(wildcard src/*.h)
	printf "lines of c++: "
	(cloc $(^) --quiet --sql=-; echo "select sum(nCode) from t where Language in ('C++', 'C/C++ Header');")|sqlite3 :memory:

$(executable): $(vm-objects) $(main-objects) $(embed-objects)
	mkdir -p $(dir ${@})
	printf "linking   %12s %12s      %12s %12s\n" "" "" $(dir ${@}) $(notdir ${@})
	clang++ -O0 -g3 -o ${@} ${^}

$(objects): build/%.cpp.o: %.cpp $(headers)
	mkdir -p $(dir ${@})
	printf "compiling %12s %12s   -> %12s %12s\n" $(dir ${<}) $(notdir ${<})  $(dir ${@}) $(notdir ${@})
	clang++ -Wall -Werror -Wextra -Wno-unused-parameter -Isrc -O0 -g3 -c -std=c++11 -o ${@} ${<}

.PHONY: clean
clean:
	echo "removing build"
	rm -rf build