PASS ?= /pass-debug.so

CXX = clang++-18
CC = clang-18
CXXFLAGS = -Wall -Wextra -std=c++20 -O3 -fplugin=$(PASS) -fpass-plugin=$(PASS)
CFLAGS = -Wall -Wextra -std=c17 -O3 -fplugin=$(PASS) -fpass-plugin=$(PASS)

all: edit-distance quicksort

edit-distance: edit-distance.cpp
	$(CXX) $(CXXFLAGS) -o edit-distance edit-distance.cpp

quicksort: quicksort.c
	$(CC) $(CFLAGS) -o quicksort quicksort.c

clean:
	rm -f edit-distance quicksort

.PHONY: all clean
