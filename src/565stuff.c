// Copyright (c) 2024 Raspberry Pi (Trading) Ltd.

// Generate DVI output using the command expander and TMDS encoder in HSTX.

// This example requires an external digital video connector connected to
// GPIOs 12 through 19 (the HSTX-capable GPIOs) with appropriate
// current-limiting resistors, e.g. 270 ohms. The pinout used in this example
// matches the Pico DVI Sock board, which can be soldered onto a Pico 2:
// https://github.com/Wren6991/Pico-DVI-Sock

#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/structs/bus_ctrl.h"
#include "hardware/structs/hstx_ctrl.h"
#include "hardware/structs/hstx_fifo.h"
#include "hardware/structs/xip_ctrl.h"
#include "hardware/structs/sio.h"
#include "hardware/vreg.h"
#include "pico/multicore.h"
#include "pico/sem.h"
#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include <string.h>

#define ALIGNED __attribute__((aligned(4)))
// ----------------------------------------------------------------------------
// DVI constants

#define TMDS_CTRL_00 0x354u
#define TMDS_CTRL_01 0x0abu
#define TMDS_CTRL_10 0x154u
#define TMDS_CTRL_11 0x2abu

#define SYNC_V0_H0 (TMDS_CTRL_00 | (TMDS_CTRL_00 << 10) | (TMDS_CTRL_00 << 20))
#define SYNC_V0_H1 (TMDS_CTRL_01 | (TMDS_CTRL_00 << 10) | (TMDS_CTRL_00 << 20))
#define SYNC_V1_H0 (TMDS_CTRL_10 | (TMDS_CTRL_00 << 10) | (TMDS_CTRL_00 << 20))
#define SYNC_V1_H1 (TMDS_CTRL_11 | (TMDS_CTRL_00 << 10) | (TMDS_CTRL_00 << 20))

#define MODE_H_SYNC_POLARITY 0
#define MODE_H_ACTIVE_PIXELS 640
#define MODE_H_FRONT_PORCH   16
#define MODE_H_SYNC_WIDTH    64
#define MODE_H_BACK_PORCH    120

#define MODE_V_SYNC_POLARITY 0
#define MODE_V_ACTIVE_LINES 480
#define MODE_V_FRONT_PORCH   1
#define MODE_V_SYNC_WIDTH    3
#define MODE_V_BACK_PORCH    16
#define clockspeed 315000
#define clockdivisor 2
int X_TILE=80, Y_TILE=40;
uint8_t FRAMEBUFFER[(MODE_H_ACTIVE_PIXELS/2)*(MODE_V_ACTIVE_LINES/2)*2];
uint16_t ALIGNED HDMIlines[2][MODE_H_ACTIVE_PIXELS]={0};
uint8_t *WriteBuf=FRAMEBUFFER;
uint8_t *DisplayBuf=FRAMEBUFFER;
uint8_t *LayerBuf=FRAMEBUFFER;
uint16_t *tilefcols; 
uint16_t *tilebcols;
volatile int HRes;
volatile int VRes;
#define MODE_H_TOTAL_PIXELS ( \
    MODE_H_FRONT_PORCH + MODE_H_SYNC_WIDTH + \
    MODE_H_BACK_PORCH  + MODE_H_ACTIVE_PIXELS \
)
#define MODE_V_TOTAL_LINES  ( \
    MODE_V_FRONT_PORCH + MODE_V_SYNC_WIDTH + \
    MODE_V_BACK_PORCH  + MODE_V_ACTIVE_LINES \
)
volatile int HDMImode = 0;
#define HSTX_CMD_RAW         (0x0u << 12)
#define HSTX_CMD_RAW_REPEAT  (0x1u << 12)
#define HSTX_CMD_TMDS        (0x2u << 12)
#define HSTX_CMD_TMDS_REPEAT (0x3u << 12)
#define HSTX_CMD_NOP         (0xfu << 12)
#define SCREENMODE1         26
#define SCREENMODE2       27
#define SCREENMODE3       28
#define SCREENMODE4       29
#define SCREENMODE5       30
#define SCREENMODE6       31

#define MMFLOAT double
#define RGB(red, green, blue) (unsigned int) (((red & 0b11111111) << 16) | ((green  & 0b11111111) << 8) | (blue & 0b11111111))
const uint32_t MAP256DEF[256] =
{
		0x000000,0x000055,0x0000AA,0x0000FF,0x000100,0x002455,0x0024AA,0x0024FF,
		0x004800,0x004855,0x0048AA,0x0048FF,0x006B00,0x006B55,0x006BAA,0x006BFF,
		0x009100,0x009155,0x0091AA,0x0091FF,0x00B500,0x00B555,0x00B5aa,0x00B5ff,
		0x00D000,0x00D055,0x00D0AA,0x00D0FF,0x00FF00,0x00FF55,0x00FFAA,0x00FFFF,
		0x240000,0x240055,0x2400AA,0x2400FF,0x242400,0x242455,0x2424AA,0x2424FF,
		0x244800,0x244855,0x2448AA,0x2448FF,0x246B00,0x246B55,0x246BAA,0x246BFF,
		0x249100,0x249155,0x2491AA,0x2491FF,0x24B500,0x24B555,0x24B5aa,0x24B5ff,
		0x24D000,0x24D055,0x24D0AA,0x24D0FF,0x24FF00,0x24FF55,0x24FFAA,0x24FFFF,
		0x480000,0x480055,0x4800AA,0x4800FF,0x482400,0x482455,0x4824AA,0x4824FF,
		0x484800,0x484855,0x4848AA,0x4848FF,0x486B00,0x486B55,0x486BAA,0x486BFF,
		0x489100,0x489155,0x4891AA,0x4891FF,0x48B500,0x48B555,0x48B5aa,0x48B5ff,
		0x48D000,0x48D055,0x48D0AA,0x48D0FF,0x48FF00,0x48FF55,0x48FFAA,0x48FFFF,
		0x6B5000,0x6B0055,0x6B00AA,0x6B00FF,0x6B2400,0x6B2455,0x6B24AA,0x6B24FF,
		0x6B4800,0x6B4855,0x6B48AA,0x6B48FF,0x6B6B00,0x6B6B55,0x6B6BAA,0x6B6BFF,
		0x6B9100,0x6B9155,0x6B91AA,0x6B91FF,0x6BB500,0x6BB555,0x6BB5aa,0x6BB5ff,
		0x6BD000,0x6BD055,0x6BD0AA,0x6BD0FF,0x6BFF00,0x6BFF55,0x6BFFAA,0x6BFFFF,
		0x910000,0x910055,0x9100AA,0x9100FF,0x912400,0x912455,0x9124AA,0x9124FF,
		0x914800,0x914855,0x9148AA,0x9148FF,0x916B00,0x916B55,0x916BAA,0x916BFF,
		0x919100,0x919155,0x9191AA,0x9191FF,0x91B500,0x91B555,0x91B5aa,0x91B5ff,
		0x91D000,0x91D055,0x91D0AA,0x91D0FF,0x91FF00,0x91FF55,0x91FFAA,0x91FFFF,
		0xB50000,0xB50055,0xB500AA,0xB500FF,0xB02400,0xB02455,0xB024AA,0xB024FF,
		0xB04800,0xB04855,0xB048AA,0xB048FF,0xB06B00,0xB06B55,0xB06BAA,0xB06BFF,
		0xB09100,0xB09155,0xB091AA,0xB091FF,0xB0B500,0xB0B555,0xB0B5aa,0xB0B5ff,
		0xB0D000,0xB0D055,0xB0D0AA,0xB0D0FF,0xB5ff00,0xB5ff55,0xB5ffAA,0xB5ffFF,
		0xD00000,0xD00055,0xD000AA,0xD000FF,0xD02400,0xD02455,0xD024AA,0xD024FF,
		0xD04800,0xD04855,0xD048AA,0xD048FF,0xD06B00,0xD06B55,0xD06BAA,0xD06BFF,
		0xD09100,0xD09155,0xD091AA,0xD091FF,0xD0B500,0xD0B555,0xD0B5aa,0xD0B5ff,
		0xD0D000,0xD0D055,0xD0D0AA,0xD0D0FF,0xD0FF00,0xD0FF55,0xD0FFAA,0xD0FFFF,
		0xFF0000,0xFF0055,0xFF00AA,0xFF00FF,0xFF2400,0xFF2455,0xFF24AA,0xFF24FF,
		0xFF4800,0xFF4855,0xFF48AA,0xFF48FF,0xFF6B00,0xFF6B55,0xFF6BAA,0xFF6BFF,
		0xFF9100,0xFF9155,0xFF91AA,0xFF91FF,0xFFB500,0xFFB555,0xFFB5aa,0xFFB5ff,
		0xFFD000,0xFFD055,0xFFD0AA,0xFFD0FF,0xFFFF00,0xFFFF55,0xFFFFAA,0xFFFFFF
};
const uint32_t MAP16DEF[16] = {0x00,0xFF,0x4000,0x40ff,0x8000,0x80ff,0xff00,0xffff,0xff0000,0xff00FF,0xff4000,0xff40ff,0xff8000,0xff80ff,0xffff00,0xffffff};
const uint32_t MAP4DEF[4] = {0,0xFF,0xFF00,0xFF0000};
const uint32_t MAP2DEF[2] = {0,0xFFFFFF};
const uint32_t MAP4LDEF[4] = {0xFFFF,0xFF00FF,0xFFFF00,0xFFFFFF};
uint16_t map256[256];
uint16_t map16[16];
uint16_t map4[4];
uint16_t map4l[4];
uint16_t map2[2];
volatile uint8_t transparent=0;
int ytilecount=12, xtilecount=8; //default tile size is 8x12
//int Y_TILE, X_TILE;
#define MODE1SIZE  MODE_H_ACTIVE_PIXELS * MODE_V_ACTIVE_LINES /8
#define MODE2SIZE  (MODE_H_ACTIVE_PIXELS/2) * (MODE_V_ACTIVE_LINES/2)/2
#define MODE3SIZE  MODE_H_ACTIVE_PIXELS * MODE_V_ACTIVE_LINES/2
#define MODE4SIZE  (MODE_H_ACTIVE_PIXELS/2) * (MODE_V_ACTIVE_LINES/2)
#define MODE5SIZE  MODE_H_ACTIVE_PIXELS * MODE_V_ACTIVE_LINES/4
#define MODE6SIZE  (MODE_H_ACTIVE_PIXELS/2) * (MODE_V_ACTIVE_LINES/2)*2
void settiles(void){
//    Y_TILE==480/12;
    tilefcols=(uint16_t *)DisplayBuf+MODE1SIZE;
    tilebcols=tilefcols+(MODE_H_ACTIVE_PIXELS/8)*(MODE_V_ACTIVE_LINES/8); //minimum tilesize is 8x8
    memset(tilefcols,0xFF,(MODE_H_ACTIVE_PIXELS/8)*(MODE_V_ACTIVE_LINES/8)*sizeof(uint16_t));
    memset(tilebcols,0,(MODE_H_ACTIVE_PIXELS/8)*(MODE_V_ACTIVE_LINES/8)*sizeof(uint16_t));
}
// ----------------------------------------------------------------------------
// HSTX command lists

// Lists are padded with NOPs to be >= HSTX FIFO size, to avoid DMA rapidly
// pingponging and tripping up the IRQs.

static uint32_t vblank_line_vsync_off[] = {
    HSTX_CMD_RAW_REPEAT | MODE_H_FRONT_PORCH,
    SYNC_V1_H1,
    HSTX_CMD_RAW_REPEAT | MODE_H_SYNC_WIDTH,
    SYNC_V1_H0,
    HSTX_CMD_RAW_REPEAT | (MODE_H_BACK_PORCH + MODE_H_ACTIVE_PIXELS),
    SYNC_V1_H1,
    HSTX_CMD_NOP
};

static uint32_t vblank_line_vsync_on[] = {
    HSTX_CMD_RAW_REPEAT | MODE_H_FRONT_PORCH,
    SYNC_V0_H1,
    HSTX_CMD_RAW_REPEAT | MODE_H_SYNC_WIDTH,
    SYNC_V0_H0,
    HSTX_CMD_RAW_REPEAT | (MODE_H_BACK_PORCH + MODE_H_ACTIVE_PIXELS),
    SYNC_V0_H1,
    HSTX_CMD_NOP
};

static uint32_t vactive_line[] = {
    HSTX_CMD_RAW_REPEAT | MODE_H_FRONT_PORCH,
    SYNC_V1_H1,
    HSTX_CMD_NOP,
    HSTX_CMD_RAW_REPEAT | MODE_H_SYNC_WIDTH,
    SYNC_V1_H0,
    HSTX_CMD_NOP,
    HSTX_CMD_RAW_REPEAT | MODE_H_BACK_PORCH,
    SYNC_V1_H1,
    HSTX_CMD_TMDS       | MODE_H_ACTIVE_PIXELS
};
int rgb(int r, int g, int b) {
    return RGB(r, g, b);
}
uint16_t RGB555(uint32_t c){
    return ((c & 0xf8)>>3) | ((c& 0xf800)>>6) | ((c & 0xf80000)>>9);
}
void DrawRectangle555(int x1, int y1, int x2, int y2, int c){
    int x,y,t;
    uint16_t col=((c & 0xf8)>>3) | ((c& 0xf800)>>6) | ((c & 0xf80000)>>9);
    if(x1 < 0) x1 = 0;
    if(x1 >= HRes) x1 = HRes - 1;
    if(x2 < 0) x2 = 0; 
    if(x2 >= HRes) x2 = HRes - 1;
    if(y1 < 0) y1 = 0;
    if(y1 >= VRes) y1 = VRes - 1;
    if(y2 < 0) y2 = 0;
    if(y2 >= VRes) y2 = VRes - 1;
    if(x2 <= x1) { t = x1; x1 = x2; x2 = t; }
    if(y2 <= y1) { t = y1; y1 = y2; y2 = t; }
    for(y=y1;y<=y2;y++){
        uint16_t *p=(uint16_t *)((uint8_t *)(WriteBuf+((y*HRes+x1)*2)));
        for(x=x1;x<=x2;x++){
            *p++=col;
       }
    }
}

void DrawRectangle256(int x1, int y1, int x2, int y2, int c){
    int x,y,t;
    uint8_t colour =((c & 0b111000000000000000000000)>>16) | ((c & 0b1110000000000000)>>11) | ((c & 0b11000000)>>6);
    if(x1 < 0) x1 = 0;
    if(x1 >= HRes) x1 = HRes - 1;
    if(x2 < 0) x2 = 0; 
    if(x2 >= HRes) x2 = HRes - 1;
    if(y1 < 0) y1 = 0;
    if(y1 >= VRes) y1 = VRes - 1;
    if(y2 < 0) y2 = 0;
    if(y2 >= VRes) y2 = VRes - 1;
    if(x2 <= x1) { t = x1; x1 = x2; x2 = t; }
    if(y2 <= y1) { t = y1; y1 = y2; y2 = t; }
    for(y=y1;y<=y2;y++){
        uint8_t *p=WriteBuf+(y*HRes+x1);
        for(x=x1;x<=x2;x++){
            *p++=colour;
        }
    }
}

void DrawRectangle16(int x1, int y1, int x2, int y2, int c){
    int x,y,x1p,x2p,t;
//    unsigned char mask;
    unsigned char colour = ((c & 0x800000)>> 20) | ((c & 0xC000)>>13) | ((c & 0x80)>>7);
    unsigned char bcolour=(colour<<4) | colour;
    if(x1 < 0) x1 = 0;
    if(x1 >= HRes) x1 = HRes - 1;
    if(x2 < 0) x2 = 0;
    if(x2 >= HRes) x2 = HRes - 1;
    if(y1 < 0) y1 = 0;
    if(y1 >= VRes) y1 = VRes - 1;
    if(y2 < 0) y2 = 0;
    if(y2 >= VRes) y2 = VRes - 1;
    if(x2 <= x1) { t = x1; x1 = x2; x2 = t; }
    if(y2 <= y1) { t = y1; y1 = y2; y2 = t; }
    for(y=y1;y<=y2;y++){
        x1p=x1;
        x2p=x2;
        uint8_t *p=(uint8_t *)(((uint32_t) WriteBuf)+(y*(HRes>>1))+(x1>>1));
        if((x1 % 2) == 1){
            *p &=0x0F;
            *p |=(colour<<4);
            p++;
            x1p++;
        }
        if((x2 % 2) == 0){
            uint8_t *q=(uint8_t *)(((uint32_t) WriteBuf)+(y*(HRes>>1))+(x2>>1));
            *q &=0xF0;
            *q |= colour;
            x2p--;
        }
        for(x=x1p;x<x2p;x+=2){
            *p++=bcolour;
        }
    }
}
void DrawRectangle2(int x1, int y1, int x2, int y2, int c){
    int x,y,x1p, x2p, t;
    unsigned char mask;
    unsigned char *p;
    if(x1 < 0) x1 = 0;
    if(x1 >= HRes) x1 = HRes - 1;
    if(x2 < 0) x2 = 0;
    if(x2 >= HRes) x2 = HRes - 1;
    if(y1 < 0) y1 = 0;
    if(y1 >= VRes) y1 = VRes - 1;
    if(y2 < 0) y2 = 0;
    if(y2 >= VRes) y2 = VRes - 1;
    if(x2 <= x1) { t = x1; x1 = x2; x2 = t; }
    if(y2 <= y1) { t = y1; y1 = y2; y2 = t; }
    if(x2 <= x1) { t = x1; x1 = x2; x2 = t; }
    if(y2 <= y1) { t = y1; y1 = y2; y2 = t; }
    if(x1==x2){
        for(y=y1;y<=y2;y++){
            p=&WriteBuf[(y*(HRes>>3))+(x1>>3)];
            mask=1<<(x1 % 8); //get the bit position for this bit
            if(c){
                *p|=mask;
            } else {
                *p&=(~mask);
            }
        }
    } else {
        for(y=y1;y<=y2;y++){
            x1p=x1;
            x2p=x2;
            if((x1 % 8) !=0){
                p=&WriteBuf[(y*(HRes>>3))+(x1>>3)];
                for(x=x1;x<=x2 && (x % 8)!=0;x++){
                    mask=1<<(x % 8); //get the bit position for this bit
                    if(c){
                        *p|=mask;
                    } else {
                        *p&=(~mask);
                    }
                    x1p++;
                }
            }
            if(x1p-1!=x2 && (x2 % 8)!=7){
                p=&WriteBuf[(y*(HRes>>3))+(x2p>>3)];
                for(x=(x2 & 0xFFF8);x<=x2 ;x++){
                    mask=1<<(x % 8); //get the bit position for this bit
                    if(c){
                        *p|=mask;
                    } else {
                        *p&=(~mask);
                    }
                    x2p--;
                }
            }
            p=&WriteBuf[(y*(HRes>>3))+(x1p>>3)];
            for(x=x1p;x<x2p;x+=8){
                if(c){
                    *p++=0xFF;
                } else {
                    *p++=0;
                }
            }
        }
    }
}
void __not_in_flash_func(DrawRectangle4)(int x1, int y1, int x2, int y2, int c){
    int x,y,x1p, x2p, t;
    unsigned char mask, undo;
    unsigned char *p;
    if(x1 < 0) x1 = 0;
    if(x1 >= HRes) x1 = HRes - 1;
    if(x2 < 0) x2 = 0;
    if(x2 >= HRes) x2 = HRes - 1;
    if(y1 < 0) y1 = 0;
    if(y1 >= VRes) y1 = VRes - 1;
    if(y2 < 0) y2 = 0;
    if(y2 >= VRes) y2 = VRes - 1;
    if(x2 <= x1) { t = x1; x1 = x2; x2 = t; }
    if(y2 <= y1) { t = y1; y1 = y2; y2 = t; }
    if(x2 <= x1) { t = x1; x1 = x2; x2 = t; }
    if(y2 <= y1) { t = y1; y1 = y2; y2 = t; }
    unsigned char colour = ((c & 0x800000)>> 22) | ((c & 0x80)>>7);
    if(x1==x2){
        for(y=y1;y<=y2;y++){
            p=&WriteBuf[(y*(HRes>>2))+(x1>>2)];
            mask=colour<<((x1 % 4)<<1); //get the bit positions for this pixel
            undo=3<<((x1 % 4)<<1);
            *p&=(~undo);
            *p|=mask;
        }
   }  else {
        for(y=y1;y<=y2;y++){
//            busy_wait_us(20);
            x1p=x1;
            x2p=x2;
            if((x1 % 4) !=0){
                p=&WriteBuf[(y*(HRes>>2))+(x1>>2)];
                for(x=x1;x<=x2 && (x % 4)!=0;x++){
                    mask=colour<<((x % 4)<<1); //get the bit positions for this pixel
                    undo=3<<((x % 4)<<1);
                    *p&=(~undo);
                    *p|=mask;
                    x1p++;
                }
            }
            if(x1p-1!=x2 && (x2 % 4)!=3){
                p=&WriteBuf[(y*(HRes>>2))+(x2p>>2)];
                for(x=(x2 & 0xFFFC);x<=x2 ;x++){
                    mask=colour<<((x % 4)<<1); //get the bit positions for this pixel
                    undo=3<<((x % 4)<<1);
                    *p&=(~undo);
                    *p|=mask;
                    x2p--;
                }
            }
            p=&WriteBuf[(y*(HRes>>2))+(x1p>>2)];
            for(x=x1p;x<x2p;x+=4){
                *p++=(colour | (colour<<2) | (colour<<4) | (colour<<6));
            }
        }
    }
}

void (*DrawRectangle)(int x1, int y1, int x2, int y2, int c) = (void (*)(int , int , int , int , int ))DrawRectangle16;

void DrawPixel(int x, int y, int c) {
    DrawRectangle(x, y, x, y, c);
}
void DrawLine(int x1, int y1, int x2, int y2, int w, int c) {


    if(y1 == y2) {
        DrawRectangle(x1, y1, x2, y2 + w - 1, c);                   // horiz line
        return;
    }
    if(x1 == x2) {
        DrawRectangle(x1, y1, x2 + w - 1, y2, c);                   // vert line
        return;
    }
    int  dx, dy, sx, sy, err, e2;
    dx = abs(x2 - x1); sx = x1 < x2 ? 1 : -1;
    dy = -abs(y2 - y1); sy = y1 < y2 ? 1 : -1;
    err = dx + dy;
    while(1) {
        DrawPixel(x1, y1, c);
        e2 = 2 * err;
        if (e2 >= dy) {
            if (x1 == x2) break;
            err += dy; x1 += sx;
        }
        if (e2 <= dx) {
            if (y1 == y2) break;
            err += dx; y1 += sy;
        }
    }
}

void DrawCircle(int x, int y, int radius, int w, int c, int fill, MMFLOAT aspect) {
   int a, b, P;
   int A, B;
   int asp; 
   MMFLOAT aspect2;
   if(w>1){
	   if(fill>=0){ // thick border with filled centre
		   DrawCircle(x,y,radius,0,c,c,aspect);
		    aspect2=((aspect*(MMFLOAT)radius)-(MMFLOAT)w)/((MMFLOAT)(radius-w));
		   DrawCircle(x,y,radius-w,0,fill,fill,aspect2);
	   } else { /*//thick border with empty centre
		   	int r1=radius-w,r2=radius, xs=-1,xi=0, i,j,k,m, ll=radius;
		    if(aspect>1.0)ll=(int)((MMFLOAT)radius*aspect);
		    int ints_per_line=RoundUptoInt((ll*2)+1)/32;
		    uint32_t *br=(uint32_t *)GetTempMemory(((ints_per_line+1)*((r2*2)+1))*4);
		    DrawFilledCircle(x, y, r2, r2, 1, ints_per_line, br, aspect, aspect);
		    aspect2=((aspect*(MMFLOAT)r2)-(MMFLOAT)w)/((MMFLOAT)r1);
		    DrawFilledCircle(x, y, r1, r2, 0, ints_per_line, br, aspect, aspect2);
		    x=(int)((MMFLOAT)x+(MMFLOAT)r2*(1.0-aspect));
		 	for(j=0;j<r2*2+1;j++){
		 		for(i=0;i<ints_per_line;i++){
		 			k=br[i+j*ints_per_line];
		 			for(m=0;m<32;m++){
		 				if(xs==-1 && (k & 0x80000000)){
		 					xs=m;
		 					xi=i;
		 				}
		 				if(xs!=-1 && !(k & 0x80000000)){
							DrawRectangle(x-r2+xs+xi*32, y-r2+j, x-r2+m+i*32, y-r2+j, c);
		 					xs=-1;
		 				}
		 				k<<=1;
		 			}
		 		}
				if(xs!=-1){
					DrawRectangle(x-r2+xs+xi*32, y-r2+j, x-r2+m+i*32, y-r2+j, c);
					xs=-1;
				}
			}*/
	   }

   } else { //single thickness outline
	   int w1=w,r1=radius;
	   if(fill>=0){
		   while(w >= 0 && radius > 0) {
		       a = 0;
		       b = radius;
		       P = 1 - radius;
		       asp = aspect * (MMFLOAT)(1 << 10);

		       do {
		         A = (a * asp) >> 10;
		         B = (b * asp) >> 10;
		         if(fill >= 0 && w >= 0) {
		             DrawRectangle(x-A, y+b, x+A, y+b, fill);
		             DrawRectangle(x-A, y-b, x+A, y-b, fill);
		             DrawRectangle(x-B, y+a, x+B, y+a, fill);
		             DrawRectangle(x-B, y-a, x+B, y-a, fill);
		         }
		          if(P < 0)
		             P+= 3 + 2*a++;
		          else
		             P+= 5 + 2*(a++ - b--);

		        } while(a <= b);
		        w--;
		        radius--;
		   }
	   }
	   if(c!=fill){
		   w=w1; radius=r1;
		   while(w >= 0 && radius > 0) {
		       a = 0;
		       b = radius;
		       P = 1 - radius;
		       asp = aspect * (MMFLOAT)(1 << 10);
		       do {
		         A = (a * asp) >> 10;
		         B = (b * asp) >> 10;
		         if(w) {
		             DrawPixel(A+x, b+y, c);
		             DrawPixel(B+x, a+y, c);
		             DrawPixel(x-A, b+y, c);
		             DrawPixel(x-B, a+y, c);
		             DrawPixel(B+x, y-a, c);
		             DrawPixel(A+x, y-b, c);
		             DrawPixel(x-A, y-b, c);
		             DrawPixel(x-B, y-a, c);
		         }
		          if(P < 0)
		             P+= 3 + 2*a++;
		          else
		             P+= 5 + 2*(a++ - b--);

		        } while(a <= b);
		        w--;
		        radius--;
		   }
	   }
   }
}
// ----------------------------------------------------------------------------
// DMA logic

#define DMACH_PING 0
#define DMACH_PONG 1

// First we ping. Then we pong. Then... we ping again.
static bool dma_pong = false;

// A ping and a pong are cued up initially, so the first time we enter this
// handler it is to cue up the second ping after the first ping has completed.
// This is the third scanline overall (-> =2 because zero-based).
volatile uint v_scanline = 2;

// During the vertical active period, we take two IRQs per scanline: one to
// post the command list, and another to post the pixels.
static bool vactive_cmdlist_posted = false;
volatile uint vblank=0;
void __not_in_flash_func(dma_irq_handler)() {
    // dma_pong indicates the channel that just finished, which is the one
    // we're about to reload.
    uint ch_num = dma_pong ? DMACH_PONG : DMACH_PING;
    dma_channel_hw_t *ch = &dma_hw->ch[ch_num];
    dma_hw->intr = 1u << ch_num;
    dma_pong = !dma_pong;
 
    if (v_scanline >= MODE_V_FRONT_PORCH && v_scanline < (MODE_V_FRONT_PORCH + MODE_V_SYNC_WIDTH)) {
        ch->read_addr = (uintptr_t)vblank_line_vsync_on;
        ch->transfer_count = count_of(vblank_line_vsync_on);
        vblank=1;
    } else if (v_scanline < MODE_V_FRONT_PORCH + MODE_V_SYNC_WIDTH + MODE_V_BACK_PORCH) {
        ch->read_addr = (uintptr_t)vblank_line_vsync_off;
        ch->transfer_count = count_of(vblank_line_vsync_off);
        vblank=1;
    } else if (!vactive_cmdlist_posted) {
        ch->read_addr = (uintptr_t)vactive_line;
        ch->transfer_count = count_of(vactive_line);
        vactive_cmdlist_posted = true;
        vblank=0;
    } else {
//        ch->read_addr = (uintptr_t)&FRAMEBUFFER[(v_scanline - (MODE_V_TOTAL_LINES - MODE_V_ACTIVE_LINES)) * MODE_H_ACTIVE_PIXELS];
        ch->read_addr = (uintptr_t)HDMIlines[v_scanline & 1];
        ch->transfer_count = MODE_H_ACTIVE_PIXELS/2;
        vactive_cmdlist_posted = false;
    }

    if (!vactive_cmdlist_posted) {
        v_scanline = (v_scanline + 1) % MODE_V_TOTAL_LINES;
    } 
}
// ----------------------------------------------------------------------------
// Main program



uint32_t core1stack[128];
void __not_in_flash_func(HDMICore)(void){
    int last_line=2,load_line, line_to_load, Line_dup;
    for(int i=0;i<256;i++)map256[i]=RGB555(MAP256DEF[i]);
    for(int i=0;i<16;i++)map16[i]=RGB555(MAP16DEF[i]);
    for(int i=0;i<4;i++){
        map4[i]=RGB555(MAP4DEF[i]);
        map4l[i]=RGB555(MAP4LDEF[i]);
    }
    for(int i=0;i<2;i++)map2[i]=RGB555(MAP2DEF[i]);

        // Configure HSTX's TMDS encoder for RGB332
        hstx_ctrl_hw->expand_tmds =
            29 << HSTX_CTRL_EXPAND_TMDS_L0_ROT_LSB   |
            4  << HSTX_CTRL_EXPAND_TMDS_L0_NBITS_LSB |
            2 << HSTX_CTRL_EXPAND_TMDS_L1_ROT_LSB   |
            4  << HSTX_CTRL_EXPAND_TMDS_L1_NBITS_LSB |
            7 << HSTX_CTRL_EXPAND_TMDS_L2_ROT_LSB   |
            4  << HSTX_CTRL_EXPAND_TMDS_L2_NBITS_LSB ;
        // Pixels (TMDS) come in 4 8-bit chunks. Control symbols (RAW) are an
        // entire 32-bit word.
        hstx_ctrl_hw->expand_shift =
            2 << HSTX_CTRL_EXPAND_SHIFT_ENC_N_SHIFTS_LSB |
            16 << HSTX_CTRL_EXPAND_SHIFT_ENC_SHIFT_LSB |
            1 << HSTX_CTRL_EXPAND_SHIFT_RAW_N_SHIFTS_LSB |
            0 << HSTX_CTRL_EXPAND_SHIFT_RAW_SHIFT_LSB; 

        // Serial output config: clock period of 5 cycles, pop from command
        // expander every 5 cycles, shift the output shiftreg by 2 every cycle.
        hstx_ctrl_hw->csr = 0;
        hstx_ctrl_hw->csr =
            HSTX_CTRL_CSR_EXPAND_EN_BITS |
            5u << HSTX_CTRL_CSR_CLKDIV_LSB |
            5u << HSTX_CTRL_CSR_N_SHIFTS_LSB |
            2u << HSTX_CTRL_CSR_SHIFT_LSB |
            HSTX_CTRL_CSR_EN_BITS; 

    // Note we are leaving the HSTX clock at the SDK default of 125 MHz; since
    // we shift out two bits per HSTX clock cycle, this gives us an output of
    // 250 Mbps, which is very close to the bit clock for 480p 60Hz (252 MHz).
    // If we want the exact rate then we'll have to reconfigure PLLs.

    // HSTX outputs 0 through 7 appear on GPIO 12 through 19.
    // Pinout on Pico DVI sock:
    //
    //   GP12 D0+  GP13 D0-
    //   GP14 CK+  GP15 CK-
    //   GP16 D2+  GP17 D2-
    //   GP18 D1+  GP19 D1-

    // Assign clock pair to two neighbouring pins:
    hstx_ctrl_hw->bit[2] = HSTX_CTRL_BIT0_CLK_BITS;
    hstx_ctrl_hw->bit[3] = HSTX_CTRL_BIT0_CLK_BITS | HSTX_CTRL_BIT0_INV_BITS;
    for (uint lane = 0; lane < 3; ++lane) {
        // For each TMDS lane, assign it to the correct GPIO pair based on the
        // desired pinout:
        static const int lane_to_output_bit[3] = {0, 6, 4};
        int bit = lane_to_output_bit[lane];
        // Output even bits during first half of each HSTX cycle, and odd bits
        // during second half. The shifter advances by two bits each cycle.
        uint32_t lane_data_sel_bits =
            (lane * 10    ) << HSTX_CTRL_BIT0_SEL_P_LSB |
            (lane * 10 + 1) << HSTX_CTRL_BIT0_SEL_N_LSB;
        // The two halves of each pair get identical data, but one pin is inverted.
        hstx_ctrl_hw->bit[bit    ] = lane_data_sel_bits;
        hstx_ctrl_hw->bit[bit + 1] = lane_data_sel_bits | HSTX_CTRL_BIT0_INV_BITS;
    }

    for (int i = 12; i <= 19; ++i) {
        gpio_set_function(i, 0); // HSTX
    }

    // Both channels are set up identically, to transfer a whole scanline and
    // then chain to the opposite channel. Each time a channel finishes, we
    // reconfigure the one that just finished, meanwhile the opposite channel
    // is already making progress.
    dma_channel_config c;
    c = dma_channel_get_default_config(DMACH_PING);
    channel_config_set_chain_to(&c, DMACH_PONG);
    channel_config_set_dreq(&c, DREQ_HSTX);
    dma_channel_configure(
        DMACH_PING,
        &c,
        &hstx_fifo_hw->fifo,
        vblank_line_vsync_off,
        count_of(vblank_line_vsync_off),
        false
    );
    c = dma_channel_get_default_config(DMACH_PONG);
    channel_config_set_chain_to(&c, DMACH_PING);
    channel_config_set_dreq(&c, DREQ_HSTX);
    dma_channel_configure(
        DMACH_PONG,
        &c,
        &hstx_fifo_hw->fifo,
        vblank_line_vsync_off,
        count_of(vblank_line_vsync_off),
        false
    );

    dma_hw->ints0 = (1u << DMACH_PING) | (1u << DMACH_PONG);
    dma_hw->inte0 = (1u << DMACH_PING) | (1u << DMACH_PONG);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_irq_handler);
    irq_set_enabled(DMA_IRQ_0, true);

    bus_ctrl_hw->priority = 1;
    dma_channel_start(DMACH_PING);

    while (1){
        if(v_scanline!=last_line){
            last_line=v_scanline;
            load_line=v_scanline - (MODE_V_TOTAL_LINES - MODE_V_ACTIVE_LINES);
            Line_dup=load_line>>1;
            line_to_load = last_line & 1;
            uint16_t *p=HDMIlines[line_to_load];
            if(load_line>=0 && load_line<MODE_V_ACTIVE_LINES){
                __dmb();
                switch(HDMImode){
                case SCREENMODE1: //640x480x2 colour
                    uint16_t *fcol=tilefcols+load_line/ytilecount, *bcol=tilebcols+load_line/ytilecount; //get the relevant tile
                    for(int i=0; i<MODE_H_ACTIVE_PIXELS/8 ; i++){
                        int d=DisplayBuf[load_line*MODE_H_ACTIVE_PIXELS/8+i];
                        *p++ = (d&0x1) ? *fcol : *bcol;
                        d>>=1;
                        *p++ = (d&0x1) ? *fcol : *bcol;
                        d>>=1;
                        *p++ = (d&0x1) ? *fcol : *bcol;
                        d>>=1;
                        *p++ = (d&0x1) ? *fcol : *bcol;
                        d>>=1;
                        *p++ = (d&0x1) ? *fcol : *bcol;
                        d>>=1;
                        *p++ = (d&0x1) ? *fcol : *bcol;
                        d>>=1;
                        *p++ = (d&0x1) ? *fcol : *bcol;
                        d>>=1;
                        *p++ = (d&0x1) ? *fcol : *bcol;
                        fcol++;
                        bcol++;
                    }
                    break;
                case SCREENMODE2: //320x240x16 colour with support for a top layer
                    int pp= (Line_dup)*MODE_H_ACTIVE_PIXELS/4;
                    uint8_t l,d;
                    for(int i=0; i<MODE_H_ACTIVE_PIXELS/4 ; i++){
                        l=LayerBuf[pp+i];d=DisplayBuf[pp+i];
                        if((l&0xf)!=transparent){
                            *p++=map16[l&0xf];
                            *p++=map16[l&0xf];
                        } else {
                            *p++=map16[d&0xf];
                            *p++=map16[d&0xf];
                        }
                        d>>=4;l>>=4;
                        if((l&0xf)!=transparent){
                            *p++=map16[l&0xf];
                            *p++=map16[l&0xf];
                        } else {
                            *p++=map16[d&0xf];
                            *p++=map16[d&0xf];
                        }
                    }
                    break;
                case SCREENMODE3: //640x480x16 colour
                    for(int i=0; i<MODE_H_ACTIVE_PIXELS/2 ; i++){
                        int d=DisplayBuf[load_line*MODE_H_ACTIVE_PIXELS/2+i];
                        *p++=map16[d&0xf];
                        d>>=4;
                        *p++=map16[d&0xf];
                    }
                    break;
                case SCREENMODE4: //320x240x256 colour
                    for(int i=0; i<MODE_H_ACTIVE_PIXELS/2 ; i++){
                        int d=DisplayBuf[(Line_dup)*MODE_H_ACTIVE_PIXELS/2+i];
                        int l=LayerBuf[(Line_dup)*MODE_H_ACTIVE_PIXELS/2+i];
                        if(l!=transparent){
                            *p++=map256[l];
                            *p++=map256[l];
                        } else {
                            *p++=map256[d];
                            *p++=map256[d];
                        }
                    }
                    break;
                case SCREENMODE5: //640x480x4 colour
                    for(int i=0; i<MODE_H_ACTIVE_PIXELS/4 ; i++){
                        int d=DisplayBuf[load_line*MODE_H_ACTIVE_PIXELS/4+i];
                        int l=LayerBuf[load_line*MODE_H_ACTIVE_PIXELS/4+i];
                        if(l!=transparent){
                            *p++=map4l[l & 0x03];
                        } else {
                            *p++=map4[d & 0x03];
                        }
                        d>>=2;
                        if(l!=transparent){
                            *p++=map4l[l & 0x03];
                        } else {
                            *p++=map4[d & 0x03];
                        }
                        d>>=2;
                        if(l!=transparent){
                            *p++=map4l[l & 0x03];
                        } else {
                            *p++=map4[d & 0x03];
                        }
                        d>>=2;
                        if(l!=transparent){
                            *p++=map4l[l & 0x03];
                        } else {
                            *p++=map4[d & 0x03];
                        }
                    }
                    break;
                case SCREENMODE6: //320x240xRGB555 colour
                    for(int i=0; i<MODE_H_ACTIVE_PIXELS/2 ; i++){
                        uint8_t* d=&DisplayBuf[(Line_dup)*MODE_H_ACTIVE_PIXELS+i*2];
                        int c=*d++;
                        c|=((*d++)<<8);
                        *p++=c;
                        *p++=c;
                    }
                    break;
                default:
                }
            }
        }
    }
}
int main(void) {
    int t;
    vreg_set_voltage(VREG_VOLTAGE_1_30); 
    set_sys_clock_khz(clockspeed, false);
    clock_configure(
        clk_peri,
        0,                                                // No glitchless mux
        CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS, // System PLL on AUX mux
        clockspeed * 1000,                               // Input frequency
        clockspeed * 1000                                // Output (must be same as no divider)
    );
    clock_configure(
        clk_hstx,
        0,                                                // No glitchless mux
        CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS, // System PLL on AUX mux
        clockspeed * 1000,                               // Input frequency
        clockspeed/clockdivisor * 1000                                // Output (must be same as no divider)
    );
    stdio_init_all();
    gpio_set_function(47, GPIO_FUNC_XIP_CS1); // CS for PSRAM
    xip_ctrl_hw->ctrl|=XIP_CTRL_WRITABLE_M1_BITS;
    HDMImode=0;
    multicore_launch_core1_with_stack(HDMICore,core1stack,512);
    core1stack[0]=0x12345678;
  
//Mode 1
    settiles();
    uint16_t *x=tilefcols,*b=tilebcols;
    for(int i=0;i<MODE_H_ACTIVE_PIXELS/8*MODE_V_ACTIVE_LINES/12;i++){
        x[i]=RGB555(rgb(rand() % 255, rand() %255, rand() % 255));
        b[i]=0xFFFF;
    }
    HDMImode=SCREENMODE1;HRes=MODE_H_ACTIVE_PIXELS;VRes=MODE_V_ACTIVE_LINES;
    t = ((HRes > VRes) ? HRes : VRes) / 7;
    DrawRectangle=DrawRectangle2;
    WriteBuf=DisplayBuf;
    while(time_us_64()<10000000){
        busy_wait_us(5000);
        DrawCircle(rand() % HRes, rand() % VRes, (rand() % t) + t/5, 1, 0, rgb(rand() % 255, rand() %255, rand() % 255), 1);
    }

//Mode 2
    HDMImode=0;
    HRes=MODE_H_ACTIVE_PIXELS/2;VRes=MODE_V_ACTIVE_LINES/2;
    t = ((HRes > VRes) ? HRes : VRes) / 7;
    DrawRectangle=DrawRectangle16;
    LayerBuf=DisplayBuf+MODE2SIZE;
    memset((void *)LayerBuf,0,MODE5SIZE);
    WriteBuf=(uint8_t*)LayerBuf;
    HDMImode=SCREENMODE2;
    DrawRectangle(100,75,HRes-100-1,VRes-75-1,0xFF00);
    WriteBuf=DisplayBuf;
    while(time_us_64()<20000000){
        busy_wait_us(5000);
        DrawCircle(rand() % HRes, rand() % VRes, (rand() % t) + t/5, 1, 0, rgb(rand() % 255, rand() %255, rand() % 255), 1);
    }
 
//Mode 3
    HDMImode=0;HRes=MODE_H_ACTIVE_PIXELS;VRes=MODE_V_ACTIVE_LINES;
    t = ((HRes > VRes) ? HRes : VRes) / 7;
    DrawRectangle=DrawRectangle16;
    LayerBuf=DisplayBuf;
    HDMImode=SCREENMODE3;
    while(time_us_64()<30000000){
        busy_wait_us(5000);
        DrawCircle(rand() % HRes, rand() % VRes, (rand() % t) + t/5, 1, 0, rgb(rand() % 255, rand() %255, rand() % 255), 1);
    }

//Mode 4
    HDMImode=0;HRes=MODE_H_ACTIVE_PIXELS/2;VRes=MODE_V_ACTIVE_LINES/2;
    t = ((HRes > VRes) ? HRes : VRes) / 7;
    DrawRectangle=DrawRectangle256;
    LayerBuf=DisplayBuf+MODE4SIZE;
    memset((void *)LayerBuf,0,MODE4SIZE);
    WriteBuf=(uint8_t*)LayerBuf;
    HDMImode=SCREENMODE4;
    DrawRectangle(100,75,HRes-100-1,VRes-75-1,0xFF);
    WriteBuf=DisplayBuf;
    while(time_us_64()<40000000){
        busy_wait_us(5000);
        DrawCircle(rand() % HRes, rand() % VRes, (rand() % t) + t/5, 1, 0, rgb(rand() % 255, rand() %255, rand() % 255), 1);
    }

//Mode 5    
    HDMImode=0;
    HRes=MODE_H_ACTIVE_PIXELS;VRes=MODE_V_ACTIVE_LINES;
    t = ((HRes > VRes) ? HRes : VRes) / 7;
    DrawRectangle=DrawRectangle4;
    LayerBuf=DisplayBuf+MODE5SIZE;
    memset((void *)LayerBuf,0,MODE5SIZE);
    WriteBuf=(uint8_t*)LayerBuf;
    HDMImode=SCREENMODE5;
    DrawRectangle(200,150,HRes-200-1,VRes-150-1,0xFF0000);
    WriteBuf=DisplayBuf;
    while(time_us_64()<50000000){
        busy_wait_us(5000);
        DrawCircle(rand() % HRes, rand() % VRes, (rand() % t) + t/5, 1, 0, rgb(rand() % 255, rand() %255, rand() % 255), 1);
    }

//Mode 6
    HDMImode=0;HRes=MODE_H_ACTIVE_PIXELS/2;VRes=MODE_V_ACTIVE_LINES/2;
    t = ((HRes > VRes) ? HRes : VRes) / 7;
    DrawRectangle=DrawRectangle555;
    LayerBuf=DisplayBuf;
    WriteBuf=DisplayBuf;
    HDMImode=SCREENMODE6; 
    while(time_us_64()<60000000){
        busy_wait_us(5000);
        DrawCircle(rand() % HRes, rand() % VRes, (rand() % t) + t/5, 1, 0, rgb(rand() % 255,rand() % 255,rand() % 255), 1);
    }
    printf("Clockspeed %ld\r\n",clock_get_hz(clk_hstx));


while(1){}


}
