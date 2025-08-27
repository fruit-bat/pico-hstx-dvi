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
#include "vt_ansi_seq.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
        uint8_t row[(VT_SCREEN_MAX_COLS+7)>>3];
        vt_coord_t w;
    } vt_tabs_t;

    void vt_tabs_init(
        vt_tabs_t *t,  // The tabs structure
        vt_coord_t w,  // The width of the screen
        vt_coord_t tw  // The width of tabs (default 8)
    );

    void vt_tabs_clear_all(
        vt_tabs_t *t   // The tabs structure
    );

    vt_coord_t vt_tabs_next(
        vt_tabs_t *t,  // The tabs structure
        vt_coord_t c   // The cursor column
    );

    vt_coord_t vt_tabs_prev(
        vt_tabs_t *t,  // The tabs structure
        vt_coord_t c   // The cursor column
    );

    bool vt_tabs_is_tab(
        vt_tabs_t *t,  // The tabs structure
        vt_coord_t c   // The column
    );

    void vt_tabs_set_tab(
        vt_tabs_t *t,  // The tabs structure
        vt_coord_t c   // The column
    );
    
    void vt_tabs_clear_tab(
        vt_tabs_t *t,  // The tabs structure
        vt_coord_t c   // The column
    );

#ifdef __cplusplus
}
#endif
