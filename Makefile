main: deps.o main.cpp coroutine.hpp coroutine.cpp
	g++ -std=c++17 -O0 -Wall -Wextra -Wpedantic -fno-omit-frame-pointer deps.o -o main main.cpp coroutine.cpp

deps.o: deps.asm
	nasm -f elf64 -o deps.o deps.asm
