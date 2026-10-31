Lisp REPL on the DVI terminal

A port of Robert van Engelen's Lisp interpreter, see https://github.com/Robert-van-Engelen/lisp

Type at a USB serial terminal; output appears on the DVI display. `init.lisp` (list functions,
`defun`, `map`, `filter`, ...) is built in and loaded at start up. `(quit)` starts a fresh
interpreter.

Notes
- The interpreter (about 80K) is allocated on the heap and the REPL runs on its own 64K stack
  in main RAM, because core 0's stack is a 4K scratch bank shared with the DVI interrupt code.
  Deep recursion raises `ERR 6: stack over` instead of overflowing it.
- `src/lisp.c` is the upstream C version, kept for reference; it is not built.

Tests

The interpreter can be built and tested on a PC:

    cd test
    make          # double and single precision builds with sanitizers, and a build that
                  # runs the garbage collector on every allocation
    make repl     # ./lisp is the same REPL, reading from stdin
