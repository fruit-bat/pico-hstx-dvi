#include "inv_pallet.h"

// Not const so the palettes live in RAM; the renderer reads them every sprite row
hstx_dvi_pixel_t pallet1_Green[] = {
    HSTX_DVI_PIXEL_RGB(0, 200, 0), // Green
};

hstx_dvi_pixel_t pallet1_Red[] = {
    HSTX_DVI_PIXEL_RGB(200, 0, 0), // Red
};

hstx_dvi_pixel_t pallet1_Blue[] = {
    HSTX_DVI_PIXEL_RGB(40, 40, 200), // Blue
};

hstx_dvi_pixel_t pallet1_Purple[] = {
    HSTX_DVI_PIXEL_RGB(200, 0, 200), // Purple
};

hstx_dvi_pixel_t pallet1_White[] = {
    HSTX_DVI_PIXEL_RGB(200, 200, 200), // White
};

hstx_dvi_pixel_t pallet1_Yellow[] = {
    HSTX_DVI_PIXEL_RGB(240, 200, 0), // Yellow
};

hstx_dvi_pixel_t* __not_in_flash_func(inv_pallet_green)() {
    return pallet1_Green;
}

hstx_dvi_pixel_t* __not_in_flash_func(inv_pallet_red)() {
    return pallet1_Red;
}

hstx_dvi_pixel_t* __not_in_flash_func(inv_pallet_blue)() {
    return pallet1_Blue;
}

hstx_dvi_pixel_t* __not_in_flash_func(inv_pallet_purple)() {
    return pallet1_Purple;
}

hstx_dvi_pixel_t* __not_in_flash_func(inv_pallet_white)() {
    return pallet1_White;
}

hstx_dvi_pixel_t* __not_in_flash_func(inv_pallet_yellow)() {
    return pallet1_Yellow;
}
