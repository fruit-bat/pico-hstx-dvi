# Host tests for the Lisp interpreter
#
#   make         run all test variants
#   make repl    build a host REPL from ../src/lisp-repl.cpp
#
# Variants: double precision with sanitizers, single precision (LISP_FLOAT, as an option
# on the device), and DEBUG which runs the GC on every allocation to catch values that
# are not protected from it.

CXX ?= c++
CXXFLAGS = -std=c++17 -O2 -g -Wall -fno-strict-aliasing
SANITIZE = -fsanitize=address,undefined -fno-sanitize-recover=all
DEPS = lisp_test.cpp ../src/lisp.hpp ../src/init_lisp.h

all: test

test: lisp_test lisp_test_float lisp_test_gc
	./lisp_test
	./lisp_test_float
	./lisp_test_gc

lisp_test: $(DEPS)
	$(CXX) $(CXXFLAGS) $(SANITIZE) -o $@ lisp_test.cpp

lisp_test_float: $(DEPS)
	$(CXX) $(CXXFLAGS) $(SANITIZE) -DLISP_FLOAT -o $@ lisp_test.cpp

lisp_test_gc: $(DEPS)
	$(CXX) $(CXXFLAGS) -DDEBUG -o $@ lisp_test.cpp

repl: lisp

lisp: ../src/lisp-repl.cpp ../src/lisp.hpp ../src/init_lisp.h
	$(CXX) $(CXXFLAGS) -o $@ ../src/lisp-repl.cpp

clean:
	rm -f lisp_test lisp_test_float lisp_test_gc lisp

.PHONY: all test repl clean
