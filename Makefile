C = clang++ -march=native --std=c++1z -Wall -Wextra -ggdb -O3 -fno-rtti -fno-exceptions -DNDEBUG

all: main.cc
	$C -o main main.cc -lSDL2 -lGL

