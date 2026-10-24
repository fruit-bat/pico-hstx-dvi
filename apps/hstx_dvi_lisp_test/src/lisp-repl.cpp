// lisp-repl.cpp C++17 REPL demo by Robert A. van Engelen 2022 BSD-3 license
// Adapted for the Pico: the interpreter lives on the heap, init.lisp is built in and,
// on the device, the REPL runs on its own stack (see lisp_main below).
// To build on a PC: c++ -std=c++17 -O2 -fno-strict-aliasing -o lisp lisp-repl.cpp

#include "lisp.hpp"
#include "init_lisp.h"
#include <memory>

// a Lisp interpreter with 8192 cells pool and 2048 cells stack/heap (about 80K)
typedef Lisp<8192,2048> MyLisp;

// Runs a REPL session until (quit) or the end of the input, returns nonzero after (quit)
static int repl(uintptr_t stack_limit) {
  std::unique_ptr<MyLisp> lisp(new MyLisp);
  lisp->stack_limit = stack_limit;
  lisp->input(fmemopen(const_cast<char*>(init_lisp), sizeof(init_lisp) - 1, "r"));
  while (1) {
    try {
      // Values of the expressions read from files (init.lisp, (load ...)) are not shown
      lisp->skip_file_space();
      MyLisp::I quiet = lisp->reading_file();
      lisp->unwind();
      if (!quiet) {
        putchar('\n');
        lisp->prompt("%u+%u>");
      }
      MyLisp::L x = lisp->eval(*lisp->push(lisp->read()), lisp->env);
      if (!quiet)
        lisp->print(x);
    }
    catch (int i) {
      lisp->closein();
      printf("\e[31;1mERR %d: %s\e[m", i, lisp->error(i));
    }
    catch (MyLisp::QUIT) {
      printf("Bye!\n");
      return 1;
    }
  }
}

#ifdef PICO_ON_DEVICE

// eval, read, print and the GC recurse in C, using about 0.4K of stack per level of nesting.
// Core 0's own stack is a 4K scratch bank that also holds time critical DVI code, so the
// REPL runs on this stack in main RAM instead.
#define LISP_STACK_BYTES (64 * 1024)
// Room left for interrupts and library calls when the interpreter raises err(6)
#define LISP_STACK_MARGIN (4 * 1024)

static uint64_t lisp_stack[LISP_STACK_BYTES / sizeof(uint64_t)];

static void repl_on_lisp_stack() {
  try {
    repl((uintptr_t)lisp_stack + LISP_STACK_MARGIN);
  }
  catch (...) {
    // Nothing may unwind past the stack switch
    printf("\e[31;1mLisp: unexpected exception\e[m\n");
  }
}

// Calls fn() with sp at stack_top and the stack limit (MSPLIM) at stack_bottom, so an overflow
// faults instead of overwriting memory. The previous sp and MSPLIM are restored afterwards.
static void __attribute__((noinline)) call_on_stack(void (*fn)(), void *stack_top, void *stack_bottom) {
  __asm volatile (
    "mrs r5, msplim\n"
    "msr msplim, %2\n"        // lower the limit before moving sp below the old one
    "mov r4, sp\n"
    "mov sp, %1\n"
    "blx %0\n"
    "mov sp, r4\n"
    "msr msplim, r5\n"
    :
    : "r"(fn), "r"(stack_top), "r"(stack_bottom)
    : "r0", "r1", "r2", "r3", "r4", "r5", "r12", "lr", "memory", "cc");
}

extern "C" int lisp_main(int argc, char **argv) {
  // printf and putchar go straight to the Pico stdio drivers while fprintf and putc go through
  // newlib's stdout; without buffering both reach the screen in the order they were written
  setvbuf(stdout, NULL, _IONBF, 0);
  call_on_stack(repl_on_lisp_stack, lisp_stack + sizeof(lisp_stack) / sizeof(lisp_stack[0]), lisp_stack);
  return 0;
}

#else

int main(int argc, char **argv) {
  // Allow 7.5M of the (usually 8M) stack before raising err(6)
  char base;
  repl((uintptr_t)&base - 7680 * 1024);
  return 0;
}

#endif
