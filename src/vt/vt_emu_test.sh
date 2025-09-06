#!/bin/bash
 cc -Wall \
 vt_cell.c \
 vt_emu.c \
 vt_term_test_utils.c \
 vt_parser.c \
 vt_term.c \
 vt_emu_test.c \
 vt_tabs.c \
 && ./a.out 
