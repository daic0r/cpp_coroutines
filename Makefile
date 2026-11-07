main: deps.o main.cpp coroutine.hpp
	g++ -std=c++14 -O0 -Wall -Wextra -Wpedantic -fno-omit-frame-pointer -fno-stack-protector deps.o -o main main.cpp

deps.o: deps.asm
	nasm -f elf64 -o deps.o deps.asm
