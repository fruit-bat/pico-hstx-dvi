#pragma once
/* Copyright (c) 2025 fruit-bat
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of the copyright holder nor the
 *     names of contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS,
 * COPYRIGHT HOLDERS, OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdint.h>
#include "vt_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

// Set      Reset   Action
// ESC[0m		    reset all modes (styles and colors)
// ESC[1m	ESC[22m	set bold mode.
// ESC[2m	ESC[22m	set dim/faint mode.
// ESC[3m	ESC[23m	set italic mode.
// ESC[4m	ESC[24m	set underline mode.
// ESC[5m	ESC[25m	set blinking mode
// ESC[7m	ESC[27m	set inverse/reverse mode
// ESC[8m	ESC[28m	set hidden/invisible mode
// ESC[9m	ESC[29m	set strikethrough mode.
#define VT_CELL_FLAGS_NORMAL        (0)
#define VT_CELL_FLAGS_BOLD          (1<<0)
#define VT_CELL_FLAGS_DIM           (1<<1)
#define VT_CELL_FLAGS_ITALIC        (1<<2)
#define VT_CELL_FLAGS_UNDERLINE     (1<<3)
#define VT_CELL_FLAGS_BLINK         (1<<4)
#define VT_CELL_FLAGS_REVERSE       (1<<5)
#define VT_CELL_FLAGS_INVISIBLE     (1<<6)
#define VT_CELL_FLAGS_STRIKETHROUGH (1<<7)

typedef uint32_t vt_cell_t;
typedef uint32_t vt_cell_attr_t;
typedef uint8_t  vt_cell_flags_t;
typedef uint8_t  vt_cell_colour_t;

static inline vt_cell_t vt_cell_combine(vt_cell_attr_t cell, vt_char_t ch) {
    return cell | ch;
}

static inline vt_cell_attr_t vt_cell_get_attr(vt_cell_t cell) {
    return cell & ~0xff;
}

static inline vt_cell_attr_t vt_cell_flags_add(vt_cell_attr_t cell, vt_cell_flags_t flags)
{
    return cell | (((uint32_t)flags) << 24);
}

static inline vt_cell_attr_t vt_cell_fg_set(vt_cell_attr_t cell, vt_cell_colour_t fgci)
{
    return (cell & ~(0xff << 8)) | (((uint32_t)fgci) << 8);
}

static inline vt_cell_attr_t vt_cell_bg_set(vt_cell_attr_t cell, vt_cell_colour_t bgci)
{
    return (cell & ~(0xff << 16)) | (((uint32_t)bgci) << 16);
}

static inline vt_cell_colour_t vt_cell_fg_get(vt_cell_attr_t cell)
{
    return (cell >> 8) & 0xff;
}

static inline vt_cell_colour_t vt_cell_bg_get(vt_cell_attr_t cell)
{
    return (cell >> 16) & 0xff;
}

static inline vt_cell_attr_t vt_cell_flags_clear(vt_cell_attr_t cell, vt_cell_flags_t flags)
{
    return cell & ~(((uint32_t)flags) << 24);
}

static inline vt_cell_attr_t vt_cell_flags_get(vt_cell_attr_t cell)
{
    return cell >> 24;
}

static inline vt_char_t vt_cell_get_char(vt_cell_t cell) {
    return cell & 0xff;
} 

static inline vt_cell_attr_t vt_cell_enc_attr(
    const vt_cell_colour_t fgci,
    const vt_cell_colour_t bgci,
    const vt_cell_flags_t flags
){
    return (((uint32_t)fgci) << 8) | (((uint32_t)bgci) << 16) | (((uint32_t)flags) << 24);
}

#ifdef __cplusplus
}
#endif
