all: src/config.hpp
	mkdir -p bin
	${CXX} src/main.cpp -o bin/cubeterm -funroll-loops
	strip bin/cubeterm

src/config.hpp:
	cp src/config.def.hpp $@
