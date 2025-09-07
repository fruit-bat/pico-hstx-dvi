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

#include "vt_pallet.h"

// https://en.wikipedia.org/wiki/ANSI_escape_code
void vt_pallet_make_256(
    void *d,
    vt_pallet_make_cb_t cb
) {

    // 0 - 7  Standard colours
    // 8 - 15 Bright colours
    static uint8_t p16[16][3] = {
        {0,  0,  0  }, // Black
        {170,0,  0  }, // Red
        {0,  170,0  }, // Green
        {170,85, 0  }, // Yellow
        {0,  0,  170}, // Blue
        {170,0,  170}, // Magenta
        {0,  170,170}, // Cyan
        {170,170,170}, // White
        {85, 85, 85 }, // Bright Black
        {255,85, 85 }, // Bright Red
        {85, 255,85 }, // Bright Green
        {255,255,85 }, // Bright Yellow
        {85, 85, 255}, // Bright Blue
        {255,85, 255}, // Bright Magenta
        {255,255,0  }, // Bright Cyan
        {255,255,255}, // Bright White
    };

    for(uint8_t i = 0; i < 16; ++i) {
        vt_pallet_make_cb(
            d, 
            i, 
            p16[i][0], 
            p16[i][1], 
            p16[i][2]
        );
    }
 
    // colors 16-231 are a 6x6x6 color cube
    for (uint8_t r = 0; r < 6; ++r) {
        for (uint8_t g = 0; g < 6; ++g) {
            for (uint8_t b = 0; b < 6; ++b) {
                uint32_t i = 16 + (r * 36) + (g * 6) + b;
                vt_pallet_make_cb(
                    d, 
                    i, 
                    r ? r * 40 + 55 : 0, 
                    g ? g * 40 + 55 : 0, 
                    b ? b * 40 + 55 : 0
                );
            }
        }
    }

    // colors 232-255 are a grayscale ramp, intentionally leaving out black and white
    for(uint8_t j = 0; j <  24; ++j) {
        uint32_t i = j + 232;
        uint8_t l = j * 10 + 8;
        vt_pallet_make_cb(
            d, 
            i, 
            l, 
            l, 
            l
        );
    }
}
