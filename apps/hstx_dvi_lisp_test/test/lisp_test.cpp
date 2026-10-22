// Host tests for the Lisp interpreter in ../src/lisp.hpp
//
// Each case reads and evaluates its input in a fresh interpreter with init.lisp loaded,
// and compares the printed values (separated by spaces) with the expected output.
// Errors print as ERR<n> and end the case. See the Makefile for the build variants.

#include "../src/lisp.hpp"
#include "../src/init_lisp.h"
#include <memory>
#include <string>
#include <unistd.h>
#include <fcntl.h>

typedef Lisp<8192,2048> TestLisp;

// Loop counts for the longer running cases, smaller when the GC runs on every allocation
#ifdef DEBUG
#define LOOPS "1000"
#define REPEATS "5"
#else
#define LOOPS "100000"
#define REPEATS "100"
#endif

struct Case {
  const char *name;
  const char *in;
  const char *out;
};

static const Case cases[] = {
  // numbers and arithmetic
  {"add",              "(+ 1 2 3)",                                 "6"},
  {"sub",              "(- 10 4 3)",                                "3"},
  {"negate",           "(- 5)",                                     "-5"},
  {"mul",              "(* 2 3 4)",                                 "24"},
  {"div",              "(/ 1 4)",                                   "0.25"},
  {"reciprocal",       "(/ 2)",                                     "0.5"},
  {"int",              "(int 3.7)",                                 "3"},
  {"int negative",     "(int -3.7)",                                "-3"},
  {"int large",        "(int -1e20)",                               "-1e+20"},
  {"number formats",   "-0.5 1e3 inf",                              "-0.5 1000 inf"},
  {"lt",               "(< 1 2) (< 2 1) (< -2 -1)",                 "#t () #t"},
  {"lt atoms",         "(< 'abc 'abd) (< 'b 'a)",                   "#t ()"},
  {"lt by type",       "(< 'a (cons 1 2)) (< (cons 1 2) 'a)",       "#t ()"},
  {"eq",               "(eq? 'a 'a) (eq? 'a 'b) (eq? 1 1) (eq? \"ab\" \"ab\")", "#t () #t #t"},
  // lists and printing
  {"quote",            "'(1 2 3)",                                  "(1 2 3)"},
  {"dotted",           "'(1 . 2) '(1 2 . 3)",                       "(1 . 2) (1 2 . 3)"},
  {"nested",           "'((1 2) (3 (4)) ())",                       "((1 2) (3 (4)) ())"},
  {"cons car cdr",     "(cons 1 '(2)) (car '(a b)) (cdr '(a b))",   "(1 2) a (b)"},
  {"constants",        "() #t",                                     "() #t"},
  {"type",             "(type 1) (type 'a) (type \"s\") (type '(1)) (type ())", "0 2 3 4 -1"},
  {"comment",          "; a comment\n(+ 1 1) ; another\n(+ 2 2)",   "2 4"},
  // special forms
  {"and or",           "(or () 3) (and 1 ()) (and) (or)",           "3 () #t ()"},
  {"if",               "(if 1 'yes 'no) (if () 'yes 'no) (if () 'yes)", "yes no ()"},
  {"cond",             "(cond ((< 2 1) 'a) (#t 'b))",               "b"},
  {"begin",            "(begin 1 2 3)",                             "3"},
  {"while",            "(define i 0) (while (< i 5) (setq i (+ i 1))) i", "i 5 5"},
  {"let",              "(let (x 2) (y 3) (* x y))",                 "6"},
  {"let*",             "(let* (x 2) (y (* x 3)) y)",                "6"},
  {"letrec",           "(letrec (f (lambda (n) (if (< n 1) 0 (+ n (f (- n 1)))))) (f 10))", "55"},
  {"setq",             "(define x 1) (setq x 5) x",                 "x 5 5"},
  {"set-car! set-cdr!", "(define p (cons 1 2)) (set-car! p 9) (set-cdr! p 8) p", "p 9 8 (9 . 8)"},
  // functions
  {"lambda",           "(define sq (lambda (x) (* x x))) (sq 7)",   "sq 49"},
  {"varargs",          "((lambda args args) 1 2 3) ((lambda (a . b) b) 1 2 3)", "(1 2 3) (2 3)"},
  {"closure counter",  "(define make (lambda (n) (lambda () (setq n (+ n 1))))) (define c (make 0)) (c) (c)", "make c 1 2"},
  {"factorial",        "(define f (lambda (n) (if (< n 2) 1 (* n (f (- n 1)))))) (f 10)", "f 3628800"},
  {"tail calls",       "(define loop (lambda (n) (if (< n 1) 'done (loop (- n 1))))) (loop " LOOPS ")", "loop done"},
  {"macro",            "(define unless (macro (c x) (list 'if c () x))) (unless () 'yes)", "unless yes"},
  {"backquote",        "(define x 5) `(a ,x b) `(a (b ,x))",        "x (a 5 b) (a (b 5))"},
  {"eval",             "(eval '(+ 1 2))",                           "3"},
  // strings
  {"string",           "\"hi there\"",                              "\"hi there\""},
  {"string build",     "(string \"ab\" 'cd 12)",                    "\"abcd12\""},
  {"string from list", "(string '(72 105))",                        "\"Hi\""},
  {"write print",      "(write \"a\" 1) (print \"a\")",             "a1() \"a\"()"},
  // errors
  {"catch throw",      "(catch (throw 42))",                        "(ERR . 42)"},
  {"catch error",      "(catch (car 1)) (+ 1 1)",                   "(ERR . 1) 2"},
  {"err not a pair",   "(car 1)",                                   "ERR1"},
  {"err unbound",      "undefined-symbol",                          "ERR3"},
  {"err cannot apply", "(1 2)",                                     "ERR4"},
  {"err arguments",    "((lambda (a b) a) 1)",                      "ERR5"},
  {"err syntax",       ")",                                         "ERR8"},
  {"err deep recursion", "(define d (lambda (n) (if (< n 1) 0 (+ 1 (d (- n 1)))))) (d 100000)", "d ERR6"},
  // memory
  {"gc cons",          "(define mk (lambda (n t) (if (< n 1) t (mk (- n 1) (cons n t))))) "
                       "(define rep (lambda (k) (if (< k 1) 'ok (begin (mk 1000 ()) (rep (- k 1)))))) (rep " REPEATS ")", "mk rep ok"},
  {"gc strings",       "(define s (lambda (k) (if (< k 1) 'ok (begin (string 'abcdefghij k) (s (- k 1)))))) (s " REPEATS "0)", "s ok"},
  {"out of memory and recover",
                       "(define mk (lambda (n t) (if (< n 1) t (mk (- n 1) (cons n t))))) (define big (mk 5000 ()))", "mk ERR7"},
  // init.lisp
  {"init predicates",  "(null? ()) (pair? '(1)) (number? 1) (symbol? 'a) (list? '(1 2))", "#t #t #t #t #t"},
  {"init compare",     "(> 2 1) (<= 1 1) (>= 1 2) (= 3 3)",         "#t #t () #t"},
  {"init equal",       "(equal? '(1 (2 \"x\")) '(1 (2 \"x\"))) (equal? '(1) '(2))", "#t ()"},
  {"init arithmetic",  "(abs -3) (mod 7 3) (gcd 12 18) (min 3 1 2) (max 3 1 2)", "3 1 6 1 3"},
  {"init lists",       "(length '(1 2 3)) (reverse '(1 2 3)) (append '(1) '(2 3) '(4)) (nth '(a b c) 1)", "3 (3 2 1) (1 2 3 4) b"},
  {"init higher order", "(filter odd? '(1 2 3 4 5)) (foldl + 0 '(1 2 3))", "(1 3 5) 6"},
  {"init defun",       "(defun twice (x) (* 2 x)) (twice 21)",      "twice 42"},
};

#ifndef LISP_FLOAT
// Results that need double precision
static const Case double_cases[] = {
  {"factorial 20",     "(define f (lambda (n) (if (< n 2) 1 (* n (f (- n 1)))))) (f 20)", "f 2.43290200817664e+18"},
  {"div precision",    "(/ 1 3)",                                   "0.33333333333333331"},
};
#endif

static int quiet_errors = 1;

// A fresh interpreter with init.lisp loaded
static std::unique_ptr<TestLisp> make_lisp() {
  std::unique_ptr<TestLisp> lisp(new TestLisp);
  lisp->input(fmemopen(const_cast<char*>(init_lisp), sizeof(init_lisp) - 1, "r"));
  while (lisp->skip_file_space(), lisp->reading_file()) {
    lisp->unwind();
    lisp->eval(*lisp->push(lisp->read()), lisp->env);
  }
  return lisp;
}

// Evaluates every expression in src, returning the printed values separated by spaces
static std::string run(TestLisp &lisp, const char *src) {
  char *text = NULL;
  size_t len = 0;
  FILE *out = open_memstream(&text, &len);
  lisp.out = out;
  lisp.input(fmemopen(const_cast<char*>(src), strlen(src), "r"));
  for (int first = 1; lisp.skip_file_space(), lisp.reading_file(); first = 0) {
    if (!first)
      putc(' ', out);
    try {
      lisp.unwind();
      lisp.print(lisp.eval(*lisp.push(lisp.read()), lisp.env));
    }
    catch (int e) {
      lisp.closein();
      fprintf(out, "ERR%d", e);
    }
    catch (TestLisp::QUIT) {
      break;
    }
  }
  lisp.out = stdout;
  fclose(out);
  std::string s(text, len);
  free(text);
  return s;
}

static int failures = 0, passes = 0;

static void check(const char *name, const std::string &got, const char *want) {
  if (got == want) {
    ++passes;
    return;
  }
  ++failures;
  printf("FAIL %s\n  want: %s\n  got:  %s\n", name, want, got.c_str());
}

static void run_cases(const Case *c, size_t n) {
  for (size_t i = 0; i < n; ++i) {
    std::unique_ptr<TestLisp> lisp = make_lisp();
    check(c[i].name, run(*lisp, c[i].in), c[i].out);
  }
}

// An error must leave the interpreter usable, including running out of pool cells
static void test_recovery() {
  std::unique_ptr<TestLisp> lisp = make_lisp();
  run(*lisp, "(define mk (lambda (n t) (if (< n 1) t (mk (- n 1) (cons n t))))) (define big (mk 5000 ()))");
  check("recover: after out of memory", run(*lisp, "(mk 5 ()) (length (mk 3000 ()))"), "(1 2 3 4 5) 3000");
  run(*lisp, "(car 1)");
  check("recover: after error", run(*lisp, "(+ 1 2)"), "3");
}

// The C stack check raises ERR6 instead of overflowing the C stack
static void test_stack_limit() {
  std::unique_ptr<TestLisp> lisp = make_lisp();
  run(*lisp, "(define d (lambda (n) (if (< n 1) 0 (+ 1 (d (- n 1))))))");
  lisp->stack_limit = (uintptr_t)__builtin_frame_address(0) - 32 * 1024;
  check("stack limit: shallow", run(*lisp, "(d 10)"), "10");
  check("stack limit: deep eval", run(*lisp, "(d 1000)"), "ERR6");
  check("stack limit: deep read", run(*lisp, "'((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((("), "ERR6");
  lisp->stack_limit = 0;
  check("stack limit: off", run(*lisp, "(d 100)"), "100");
}

// Terminal input: \r and \r\n line ends, backspace editing and the end of the input
static void test_terminal() {
  FILE *saved = stdin;
  std::unique_ptr<TestLisp> lisp = make_lisp();
  char *text = NULL;
  size_t len = 0;
  FILE *out = open_memstream(&text, &len);
  static const char typed[] = "(+ 1 2)\r(* 2 3)\r\n(- 9 x\b1)\n";
  stdin = fmemopen(const_cast<char*>(typed), sizeof(typed) - 1, "r");
  lisp->out = out;
  fflush(stdout);                               // hide the prompts
  int saved_stdout = dup(1), null_fd = open("/dev/null", O_WRONLY);
  dup2(null_fd, 1);
  int quit = 0;
  try {
    for (int i = 0; i < 4; ++i) {
      lisp->unwind();
      lisp->print(lisp->eval(*lisp->push(lisp->read()), lisp->env));
      putc(' ', out);
    }
  }
  catch (TestLisp::QUIT) {
    quit = 1;
  }
  fflush(stdout);
  dup2(saved_stdout, 1);
  close(saved_stdout);
  close(null_fd);
  fclose(stdin);
  stdin = saved;
  lisp->out = stdout;
  fclose(out);
  check("terminal: line ends and backspace", std::string(text, len), "3 6 8 ");
  check("terminal: quit at end of input", quit ? "yes" : "no", "yes");
  free(text);
}

int main(int argc, char **argv) {
  if (argc > 1 && !strcmp(argv[1], "-v"))
    quiet_errors = 0;
  if (quiet_errors && !freopen("/dev/null", "w", stderr))  // error messages from ERR()
    return 2;
  if (!freopen("/dev/null", "r", stdin))        // a case must never wait for the terminal
    return 2;
  run_cases(cases, sizeof(cases) / sizeof(cases[0]));
#ifndef LISP_FLOAT
  run_cases(double_cases, sizeof(double_cases) / sizeof(double_cases[0]));
#endif
  test_recovery();
  test_stack_limit();
  test_terminal();
  printf("%d passed, %d failed\n", passes, failures);
  return failures != 0;
}
