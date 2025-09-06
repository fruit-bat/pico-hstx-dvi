#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "vt_term_test_utils.h"

void print_tabs(vt_tabs_t* tabs) {
    printf("tabs: [");
    for(vt_coord_t i = 0; i < tabs->w; ++i) {
        printf("%c", (vt_tabs_is_tab(tabs, i) ? 'T' : ' '));
    }
    printf("]\n");
}

// Print out the characters in the grid so we can see what is going on
void print_grid(vt_term_t *t) {
    vt_coord_t w = t->w;
    vt_coord_t h = t->h;
    for (vt_coord_t c = 0; c < w * 7 + 5; ++c) printf(c == (t->c + 1) ? "+" : "-");
    printf("\n");
    for (vt_coord_t r = 0; r < h; ++r) {
        printf(r == (t->r) ? "+" : "|");
        for (vt_coord_t c = 0; c < w; ++c) {
            vt_cell_t ct = t->rp[r][c];
            vt_char_t ch = vt_cell_get_char(ct);
            printf("%c", (uint8_t)ch);
        }
        printf(r == (t->r) ? "+" : "|");
        for (vt_coord_t c = 0; c < w; ++c) {
            vt_cell_t ct = t->rp[r][c];
            vt_cell_attr_t ca = vt_cell_get_attr(ct);
            vt_cell_colour_t ci = vt_cell_fg_get(ca);
            printf("%02X", (uint8_t)ci);
        }
        printf("|");
        for (vt_coord_t c = 0; c < w; ++c) {
            vt_cell_t ct = t->rp[r][c];
            vt_cell_attr_t ca = vt_cell_get_attr(ct);
            vt_cell_colour_t ci = vt_cell_bg_get(ca);
            printf("%02X", (uint8_t)ci);
        }
        printf("|");
        for (vt_coord_t c = 0; c < w; ++c) {
            vt_cell_t ct = t->rp[r][c];
            vt_cell_attr_t ca = vt_cell_get_attr(ct);
            vt_cell_flags_t fl = vt_cell_flags_get(ca);
            printf("%02X", (uint8_t)fl);
        }
        printf("|\n");
    }
    for (vt_coord_t c = 0; c < w * 7 + 5; ++c) printf(c == (t->c + 1) ? "+" : "-");
    printf("\n");
}

void check_grid_blank(vt_term_t *t) {
    // Create a cell with default attributes and a space character
    vt_cell_attr_t c1 = vt_cell_enc_attr(
        VT_TERM_DEFAULT_FG,
        VT_TERM_DEFAULT_BG,
        VT_TERM_DEFAULT_FLAGS
    );
    vt_cell_t c3 = vt_cell_combine(c1, 32);
    vt_coord_t w = t->w;
    vt_coord_t h = t->h;    
    for (vt_coord_t c = 0; c < w; ++c) {
        for (vt_coord_t r = 0; r < h; ++r) {
            vt_cell_t ct = t->rp[r][c];
            assert(ct == c3);
        }
    }
}

void set_grid_rows(vt_term_t *t) {
    // Create a cell with default attributes and a space character
    vt_cell_attr_t c1 = vt_cell_enc_attr(
        VT_TERM_DEFAULT_FG,
        VT_TERM_DEFAULT_BG,
        VT_TERM_DEFAULT_FLAGS
    );
    vt_coord_t w = t->w;
    vt_coord_t h = t->h;    

    // Put characters in the grid so we can test scroll up
    // AAAAA...
    // BBBBB...
    // etc.
    for (vt_coord_t c = 0; c < w; ++c) {
        for (vt_coord_t r = 0; r < h; ++r) {
            t->rp[r][c] = vt_cell_combine(c1, r + 'A');
        }
    } 
}

void set_grid_cols(vt_term_t *t) {
    // Create a cell with default attributes and a space character
    vt_cell_attr_t c1 = vt_cell_enc_attr(
        VT_TERM_DEFAULT_FG,
        VT_TERM_DEFAULT_BG,
        VT_TERM_DEFAULT_FLAGS
    );
    vt_coord_t w = t->w;
    vt_coord_t h = t->h;    

    // Put characters in the grid so we can test scroll up
    // ABCDE...
    // ABCDE...
    // etc.
    for (vt_coord_t c = 0; c < w; ++c) {
        for (vt_coord_t r = 0; r < h; ++r) {
            t->rp[r][c] = vt_cell_combine(c1, c + 'A');
        }
    } 
}

void check_grid_rows(vt_term_t *t, char* chs) {
    vt_coord_t w = t->w;
    vt_coord_t h = t->h;    
    for (vt_coord_t c = 0; c < w; ++c) {
        for (vt_coord_t r = 0; r < h; ++r) {
            vt_cell_t ct = t->rp[r][c];
            vt_char_t ch = vt_cell_get_char(ct);
            if (chs[r] != '?') assert(ch == (vt_char_t)(chs[r]));
        }
    }
}

void check_grid_row(vt_term_t *t, vt_coord_t r, char* chs) {
    vt_coord_t w = t->w;
    for (vt_coord_t c = 0; c < w; ++c) {
        vt_cell_t ct = t->rp[r][c];
        vt_char_t ch = vt_cell_get_char(ct);
        assert(ch == (vt_char_t)(chs[c]));
    }
}
