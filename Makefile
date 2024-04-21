all:
	mkdir -p bin
	${CXX} src/main.cpp -o bin/cubeterm -funroll-loops
	strip bin/cubeterm
