bin/cubeterm: src/config.hpp src/main.cpp src/cube.cpp algs/*.txt
	mkdir -p bin
	${CXX} -std=c++11 -Wall -pedantic src/main.cpp -o bin/cubeterm -funroll-loops
	strip bin/cubeterm

src/config.hpp:
	cp src/config.def.hpp $@

sloc:
	sloccount src | tee sloc.nfo
