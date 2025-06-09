LLVM-CONFIG ?= llvm-config

CC = $(shell $(LLVM-CONFIG) --bindir)/clang
CXX = $(shell $(LLVM-CONFIG) --bindir)/clang++
CFLAGS = -fPIC -Wall -Wextra -Wno-unused-parameter -O3
CXXFLAGS = -fPIC -Wall -Wextra -Wno-unused-parameter -O3 -flto=full -I$(shell $(LLVM-CONFIG) --includedir)
LDFLAGS = -flto=full

.PHONY: all
all: pass-skip.so

pass-skip.o: pass.cpp $(wildcard *.h*)
	$(CXX) -c -o $@ $(CXXFLAGS) -DWITH_CLANG_PLUGIN -DPIPELINE=0 $<

pass-debug.o: pass.cpp $(wildcard *.h*)
	$(CXX) -c -o $@ $(CXXFLAGS) -DWITH_CLANG_PLUGIN -DPIPELINE=0 -DDEBUG_LOGGING $<

pass-no-plugin-skip.o: pass.cpp $(wildcard *.h*)
	$(CXX) -c -o $@ $(CXXFLAGS) -DPIPELINE=0 $<

pass-no-plugin-debug.o: pass.cpp $(wildcard *.h*)
	$(CXX) -c -o $@ $(CXXFLAGS) -DPIPELINE=0 -DDEBUG_LOGGING $<

%.so: %.o
	$(CXX) $(LDFLAGS) -lxxhash -shared -o $@ $^
	@strip $@

.PHONY: format
format:
	clang-format -i *.cpp *.hpp *.c

.PHONY: lint
lint:
	clang-tidy *.cpp --

.PHONY: clean
clean:
	@rm -f *.o *.so *.ll
