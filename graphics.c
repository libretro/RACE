/*---------------------------------------------------------------------------
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version. See also the license.txt file for
 *	additional informations.
 *---------------------------------------------------------------------------
 */

/* graphics.cpp: implementation of the graphics class. */

#include "types.h"
#include "graphics.h"
#include "race-memory.h"

#if defined(ABGR1555)
#define RMASK 0x001f
#define GMASK 0x03e0
#define BMASK 0x7c00
#else
#define RMASK 0xf800
#define GMASK 0x07e0
#define BMASK 0x001f
#endif

extern struct ngp_screen* screen;
extern int gfx_hacks;

/*
 * 16 bit graphics buffers
 * At the moment there's no system which uses more than 16 bit
 * colors.
 * A graphics buffer holds number referencing to the color from
 * the "total palette" for that system.
 */
int    totalpalette[32*32*32];

/* Allows application of a 'dark filter' to reduce the
 * glare of white backgrounds when viewing NGP content
 * on a backlit screen
 * Note: This code has no consistency regarding variable
 * names - so we just use the normal libretro standard of
 * delimiter-separated words...
 */
static unsigned dark_filter_level = 0;

static unsigned short *drawBuffer;

/* NGP specific
 * precalculated pattern structures */
const unsigned char mypatterns[256*4] =
    {
        0,0,0,0, 0,0,0,1, 0,0,0,2, 0,0,0,3, 0,0,1,0, 0,0,1,1, 0,0,1,2, 0,0,1,3,
        0,0,2,0, 0,0,2,1, 0,0,2,2, 0,0,2,3, 0,0,3,0, 0,0,3,1, 0,0,3,2, 0,0,3,3,
        0,1,0,0, 0,1,0,1, 0,1,0,2, 0,1,0,3, 0,1,1,0, 0,1,1,1, 0,1,1,2, 0,1,1,3,
        0,1,2,0, 0,1,2,1, 0,1,2,2, 0,1,2,3, 0,1,3,0, 0,1,3,1, 0,1,3,2, 0,1,3,3,
        0,2,0,0, 0,2,0,1, 0,2,0,2, 0,2,0,3, 0,2,1,0, 0,2,1,1, 0,2,1,2, 0,2,1,3,
        0,2,2,0, 0,2,2,1, 0,2,2,2, 0,2,2,3, 0,2,3,0, 0,2,3,1, 0,2,3,2, 0,2,3,3,
        0,3,0,0, 0,3,0,1, 0,3,0,2, 0,3,0,3, 0,3,1,0, 0,3,1,1, 0,3,1,2, 0,3,1,3,
        0,3,2,0, 0,3,2,1, 0,3,2,2, 0,3,2,3, 0,3,3,0, 0,3,3,1, 0,3,3,2, 0,3,3,3,
        1,0,0,0, 1,0,0,1, 1,0,0,2, 1,0,0,3, 1,0,1,0, 1,0,1,1, 1,0,1,2, 1,0,1,3,
        1,0,2,0, 1,0,2,1, 1,0,2,2, 1,0,2,3, 1,0,3,0, 1,0,3,1, 1,0,3,2, 1,0,3,3,
        1,1,0,0, 1,1,0,1, 1,1,0,2, 1,1,0,3, 1,1,1,0, 1,1,1,1, 1,1,1,2, 1,1,1,3,
        1,1,2,0, 1,1,2,1, 1,1,2,2, 1,1,2,3, 1,1,3,0, 1,1,3,1, 1,1,3,2, 1,1,3,3,
        1,2,0,0, 1,2,0,1, 1,2,0,2, 1,2,0,3, 1,2,1,0, 1,2,1,1, 1,2,1,2, 1,2,1,3,
        1,2,2,0, 1,2,2,1, 1,2,2,2, 1,2,2,3, 1,2,3,0, 1,2,3,1, 1,2,3,2, 1,2,3,3,
        1,3,0,0, 1,3,0,1, 1,3,0,2, 1,3,0,3, 1,3,1,0, 1,3,1,1, 1,3,1,2, 1,3,1,3,
        1,3,2,0, 1,3,2,1, 1,3,2,2, 1,3,2,3, 1,3,3,0, 1,3,3,1, 1,3,3,2, 1,3,3,3,
        2,0,0,0, 2,0,0,1, 2,0,0,2, 2,0,0,3, 2,0,1,0, 2,0,1,1, 2,0,1,2, 2,0,1,3,
        2,0,2,0, 2,0,2,1, 2,0,2,2, 2,0,2,3, 2,0,3,0, 2,0,3,1, 2,0,3,2, 2,0,3,3,
        2,1,0,0, 2,1,0,1, 2,1,0,2, 2,1,0,3, 2,1,1,0, 2,1,1,1, 2,1,1,2, 2,1,1,3,
        2,1,2,0, 2,1,2,1, 2,1,2,2, 2,1,2,3, 2,1,3,0, 2,1,3,1, 2,1,3,2, 2,1,3,3,
        2,2,0,0, 2,2,0,1, 2,2,0,2, 2,2,0,3, 2,2,1,0, 2,2,1,1, 2,2,1,2, 2,2,1,3,
        2,2,2,0, 2,2,2,1, 2,2,2,2, 2,2,2,3, 2,2,3,0, 2,2,3,1, 2,2,3,2, 2,2,3,3,
        2,3,0,0, 2,3,0,1, 2,3,0,2, 2,3,0,3, 2,3,1,0, 2,3,1,1, 2,3,1,2, 2,3,1,3,
        2,3,2,0, 2,3,2,1, 2,3,2,2, 2,3,2,3, 2,3,3,0, 2,3,3,1, 2,3,3,2, 2,3,3,3,
        3,0,0,0, 3,0,0,1, 3,0,0,2, 3,0,0,3, 3,0,1,0, 3,0,1,1, 3,0,1,2, 3,0,1,3,
        3,0,2,0, 3,0,2,1, 3,0,2,2, 3,0,2,3, 3,0,3,0, 3,0,3,1, 3,0,3,2, 3,0,3,3,
        3,1,0,0, 3,1,0,1, 3,1,0,2, 3,1,0,3, 3,1,1,0, 3,1,1,1, 3,1,1,2, 3,1,1,3,
        3,1,2,0, 3,1,2,1, 3,1,2,2, 3,1,2,3, 3,1,3,0, 3,1,3,1, 3,1,3,2, 3,1,3,3,
        3,2,0,0, 3,2,0,1, 3,2,0,2, 3,2,0,3, 3,2,1,0, 3,2,1,1, 3,2,1,2, 3,2,1,3,
        3,2,2,0, 3,2,2,1, 3,2,2,2, 3,2,2,3, 3,2,3,0, 3,2,3,1, 3,2,3,2, 3,2,3,3,
        3,3,0,0, 3,3,0,1, 3,3,0,2, 3,3,0,3, 3,3,1,0, 3,3,1,1, 3,3,1,2, 3,3,1,3,
        3,3,2,0, 3,3,2,1, 3,3,2,2, 3,3,2,3, 3,3,3,0, 3,3,3,1, 3,3,3,2, 3,3,3,3,
    };

/* target window */
/*
unsigned short p2[16] = {
                            0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080,
                            0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000, 0x8000
                        };
*/

#define BLIT_X_OFFSET 8
#define BLIT_Y_OFFSET 8
#define BLIT_OFFSET (BLIT_X_OFFSET + (BLIT_Y_OFFSET*SIZEX))
#define BLIT_WIDTH (160)
#define BLIT_HEIGHT (152)
#define SCREEN_X_OFFSET ((screen->w - BLIT_WIDTH)/2)
#define SCREEN_Y_OFFSET ((screen->h - BLIT_HEIGHT)/2)

/* (32)  32 is good for 480x272  -64 is for 320x240 (squish) */
#define PSP_FUDGE 0
/* extra fudge factor for PSP? */
#define SCREEN_OFFET (SCREEN_X_OFFSET + (SCREEN_Y_OFFSET*(screen->w+PSP_FUDGE)))

/*
 *
 * Palette Initialization
 *
 */

void (*palette_init)(DWORD dwRBitMask, DWORD dwGBitMask, DWORD dwBBitMask);

void palette_init32(DWORD dwRBitMask, DWORD dwGBitMask, DWORD dwBBitMask)
{
/*    dbg_print("in palette_init32(0x%X, 0x%X, 0x%X)\n", dwRBitMask, dwGBitMask, dwBBitMask);

    char RShiftCount = 0, GShiftCount = 0, BShiftCount = 0;
    char RBitCount = 0, GBitCount = 0, BBitCount = 0;
    int r,g,b;
    DWORD i;

    i = dwRBitMask;
    while (!(i&1))
    {
        i = i >> 1;
        RShiftCount++;
    }
    while (i&1)
    {
        i = i >> 1;
        RBitCount++;
    }
    i = dwGBitMask;
    while (!(i&1))
    {
        i = i >> 1;
        GShiftCount++;
    }
    while (i&1)
    {
        i = i >> 1;
        GBitCount++;
    }
    i = dwBBitMask;
    while (!(i&1))
    {
        i = i >> 1;
        BShiftCount++;
    }
    while (i&1)
    {
        i = i >> 1;
        BBitCount++;
    }
    switch(m_emuInfo.machine)
    {
        case NGP:
        case NGPC:
        case GAMEGEAR:
        case LYNX:
        case WONDERSWAN:
        case WONDERSWANCOLOR:
        case ADVISION:
        for (b=0; b<16; b++)
            for (g=0; g<16; g++)
                for (r=0; r<16; r++)
                    totalpalette[b*256+g*16+r] =
                        (((b<<(BBitCount-4))+(b>>(4-(BBitCount-4))))<<BShiftCount) +
                        (((g<<(GBitCount-4))+(g>>(4-(GBitCount-4))))<<GShiftCount) +
                        (((r<<(RBitCount-4))+(r>>(4-(RBitCount-4))))<<RShiftCount);
        break;
        case GAMEBOY:
        case GAMEBOYPOCKET:
        case GAMEBOYCOLOR:
        case SUPERGAMEBOY:
        case SUPERVISION:
        case NES:
        for (b=0; b<32; b++)
            for (g=0; g<32; g++)
                for (r=0; r<32; r++)
                    totalpalette[b*32*32+g*32+r] =
                        (((b<<(BBitCount-5))+(b>>(5-(BBitCount-5))))<<BShiftCount) +
                        (((g<<(GBitCount-5))+(g>>(5-(GBitCount-5))))<<GShiftCount) +
                        (((r<<(RBitCount-5))+(r>>(5-(RBitCount-5))))<<RShiftCount);
        break;
    }*/
}

/* RGB range: [0,1] */
static void darken_rgb(float *r, float *g, float *b)
{
    /* Note: This is *very* approximate...
     * - Should be done in linear colour space. It isn't.
     * - Should alter brightness by performing an RGB->HSL->RGB
     *   conversion. We just do simple linear scaling instead.
     * Basically, this is intended for use on devices that are
     8 too weak to run shaders (i.e. why would you want a 'dark filter'
     * if your device supports proper LCD shaders?). We therefore
     * cut corners for the sake of performance...
     *
     * Constants
     * > Luminosity factors: photometric/digital ITU BT.709
     *static const float luma_r = 0.2126f;
     *static const float luma_g = 0.7152f;
     *static const float luma_b = 0.0722f;
     * > Luminosity factors: Digital ITU BT.601
     *   (seems to produce better results than ITU BT.709)
     */
    static const float luma_r = 0.299f;
    static const float luma_g = 0.587f;
    static const float luma_b = 0.114f;
    /* Calculate luminosity */
    float luma = (luma_r * (*r)) + (luma_g * (*g)) + (luma_b * (*b));
    /* Get 'darkness' scaling factor
     * > User set 'dark filter' level scaled by current luminosity
     *   (i.e. lighter colours affected more than darker colours)
     */
    float dark_factor = 1.0f - (((float)(dark_filter_level) * 0.01f) * luma);
    dark_factor = dark_factor < 0.0f ? 0.0f : dark_factor;
    /* Perform scaling... */
    *r = (*r) * dark_factor;
    *g = (*g) * dark_factor;
    *b = (*b) * dark_factor;
}

void palette_init16(DWORD dwRBitMask, DWORD dwGBitMask, DWORD dwBBitMask)
{
    char RShiftCount = 0, GShiftCount = 0, BShiftCount = 0;
    char RBitCount = 0, GBitCount = 0, BBitCount = 0;
    int  r,g,b;
    DWORD i;

    i = dwRBitMask;
    while (!(i&1))
    {
        i = i >> 1;
        RShiftCount++;
    }
    while (i&1)
    {
        i = i >> 1;
        RBitCount++;
    }
    i = dwGBitMask;
    while (!(i&1))
    {
        i = i >> 1;
        GShiftCount++;
    }
    while (i&1)
    {
        i = i >> 1;
        GBitCount++;
    }
    i = dwBBitMask;
    while (!(i&1))
    {
        i = i >> 1;
        BShiftCount++;
    }
    while (i&1)
    {
        i = i >> 1;
        BBitCount++;
    }
    switch(m_emuInfo.machine)
    {
        case NGP:
        case NGPC:
            if (dark_filter_level > 0)
            {
                static const float rgb_max     = 15.0f;
                static const float rgb_max_inv = 1.0f / 15.0f;
                float r_float, g_float, b_float;
                int r_final, g_final, b_final;

                /* Perform RGB assignment with colour conversion */
                for (b=0; b<16; b++)
                {
                    for (g=0; g<16; g++)
                    {
                        for (r=0; r<16; r++)
                        {
                            /* Convert colour range from [0,0xF] to [0,1] */
                            r_float = (float)(r) * rgb_max_inv;
                            g_float = (float)(g) * rgb_max_inv;
                            b_float = (float)(b) * rgb_max_inv;
                            /* Perform image darkening */
                            darken_rgb(&r_float, &g_float, &b_float);
                            /* Convert back to 4bit unsigned */
                            r_final = (int)((r_float * rgb_max) + 0.5f) & 0xF;
                            g_final = (int)((g_float * rgb_max) + 0.5f) & 0xF;
                            b_final = (int)((b_float * rgb_max) + 0.5f) & 0xF;

                            totalpalette[b*256+g*16+r] =
                                (((b_final<<(BBitCount-4))+(b_final>>(4-(BBitCount-4))))<<BShiftCount) +
                                (((g_final<<(GBitCount-4))+(g_final>>(4-(GBitCount-4))))<<GShiftCount) +
                                (((r_final<<(RBitCount-4))+(r_final>>(4-(RBitCount-4))))<<RShiftCount);
                        }
                    }
                }
            }
            else
            {
                /* Use fast RGB assignment */
                for (b=0; b<16; b++)
                    for (g=0; g<16; g++)
                        for (r=0; r<16; r++)
                            totalpalette[b*256+g*16+r] =
                                (((b<<(BBitCount-4))+(b>>(4-(BBitCount-4))))<<BShiftCount) +
                                (((g<<(GBitCount-4))+(g>>(4-(GBitCount-4))))<<GShiftCount) +
                                (((r<<(RBitCount-4))+(r>>(4-(RBitCount-4))))<<RShiftCount);
            }
            break;
    }
}

/* Again, there is no consistency in naming...
 * Most interface functions seem to use camel case,
 * so do the same here...
 */
void graphicsSetDarkFilterLevel(unsigned filterLevel)
{
    unsigned prev_dark_filter_level = dark_filter_level;

    dark_filter_level = filterLevel;
    dark_filter_level = (dark_filter_level > 100) ? 100 : dark_filter_level;

    if (dark_filter_level != prev_dark_filter_level)
        palette_init16(RMASK, GMASK, BMASK);      
}

void palette_init8(DWORD dwRBitMask, DWORD dwGBitMask, DWORD dwBBitMask)
{
}

void pngpalette_init(void)
{
/*    int r,g,b;

    switch(m_emuInfo.machine)
    {
        case NGP:
        case NGPC:
        case GAMEGEAR:
        case LYNX:
        case WONDERSWAN:
        case WONDERSWANCOLOR:
        case ADVISION:
        for (b=0; b<16; b++)
            for (g=0; g<16; g++)
                for (r=0; r<16; r++)
                {
                    totalpalette32[b*256+g*16+r] =
                        (((b<<(8-4))+(b>>(4-(8-4))))<<0) +
                        (((g<<(8-4))+(g>>(4-(8-4))))<<8) +
                        (((r<<(8-4))+(r>>(4-(8-4))))<<16);
                }
        break;
        case GAMEBOY:
        case GAMEBOYPOCKET:
        case GAMEBOYCOLOR:
        case SUPERGAMEBOY:
        case SUPERVISION:
        case NES:
        for (b=0; b<32; b++)
            for (g=0; g<32; g++)
                for (r=0; r<32; r++)
                {
                    totalpalette32[b*32*32+g*32+r] =
                        (((b<<(8-5))+(b>>(5-(8-5))))<<0) +
                        (((g<<(8-5))+(g>>(5-(8-5))))<<8) +
                        (((r<<(8-5))+(r>>(5-(8-5))))<<16);
                }
        break;
    }*/
}

/*
 *
 * Neogeo Pocket & Neogeo Pocket color rendering
 *
 */

/* NOTA Color para juegos b/n */
static const unsigned short bwTable[8] =
    {
        /* NOTA  nose,nose,window,nose,nose,nose,nose,nose
         * B, G,R */
        0x0FFF, 0x0DDD, 0x0BBB, 0x0999, 0x0777, 0x0555, 0x0333, 0x0000
    };

/* used for rendering the sprites */

typedef struct
{
    unsigned short offset;    /* offset in graphics buffer to blit, Flavor hopes 16bits is good enough */
    unsigned short pattern;   /* pattern code including palette number */
    unsigned short *tilept;   /* pointer into the tile description */
    unsigned short *palette;  /* palette used to render this sprite */
}
SPRITE;

typedef struct
{
    unsigned short *gbp;    /* (0,x) base for drawing */
    unsigned char count[152];
    SPRITE   sprite[152][64];
}
SPRITEDEFS;

SPRITEDEFS spriteDefs[3];    /* 4 priority levels */

/* definitions of types and variables that are used internally during
 * rendering of a screen */

typedef struct
{
    unsigned short *gbp;     /* pointer into graphics buffer */
    unsigned char oldScrollX;    /* keep an eye on the old and previous values */
    unsigned char *newScrollX;   /* of the scroll values */
    unsigned char oldScrollY;
    unsigned char *newScrollY;
    unsigned short *tileBase;    /* start address of the tile table this structure represents */
    short   tile[21];    /* the tile code */
    unsigned short *palettes[21];   /* palettes associated with tiles */
    unsigned short *tilept[21];   /* tile lines to render */
    unsigned short *palette;    /* palette for the tiles this VSYNC */
}
TILECACHE;

unsigned short palettes[16*4+16*4+16*4]; /* placeholder for the converted palette */


TILECACHE  tCBack;      /* tile pointer cache for the back buffer */
TILECACHE  tCFront;     /* tile pointer cache for the front buffer */

static INLINE void set_paletteCol(
      unsigned short *palette_table,
      unsigned short *sprite,
      unsigned short *front,
      unsigned short *back)
{
   int i;
   /* initialize palette table
    *
    * initialize back plane palette table
    */

   for(i=0;i<16*4;i++)
      sprite[i] = NGPC_TO_RGB565(palette_table[i]);

   /* initialize front plane palette table */
   for(i=0;i<16*4;i++)
      front[i] = NGPC_TO_RGB565(palette_table[i+16*4]);

   /* initialize sprite palette table (?) */
   for(i=0;i<16*4;i++)
      back[i] = NGPC_TO_RGB565(palette_table[i+16*4*2]);
}

static INLINE void set_paletteBW(
      unsigned short *palette_table,
      unsigned short *sprite,
      unsigned short *front,
      unsigned short *back)
{
    int i;
    unsigned char *pt = ((unsigned char *)palette_table)-0x0100; /* get b/w color table */

    /* initialize palette table */
    for(i=0;i<4;i++)
    {
        /* initialize sprite palette table */
        sprite[i] = NGPC_TO_RGB565(bwTable[pt[i] & 0x07]);
        sprite[4+i] = NGPC_TO_RGB565(bwTable[pt[4+i] & 0x07]);
        /* initialize front plane palette table */
        front[i] = NGPC_TO_RGB565(bwTable[pt[8+i] & 0x07]);
        front[4+i] = NGPC_TO_RGB565(bwTable[pt[8+4+i] & 0x07]);
        /* initialize back plane palette table */
        back[i] = NGPC_TO_RGB565(bwTable[pt[16+i] & 0x07]);
        back[4+i] = NGPC_TO_RGB565(bwTable[pt[16+4+i] & 0x07]);
    }
}

/* I think there's something wrong with this on the GP2X, because CFC2's intro screen is all red */
static INLINE void lineClear(TILECACHE *tC, unsigned short col)
{
   int i;
   for(i = 0; i < 21 * 8; i++)
      tC->gbp[i] = col;
}

static INLINE void clipLeftRight(SPRITEDEFS *sprDef, unsigned short OOWCol)
{
    int  i,j;

    j = *wndTopLeftX + 8;
    if (j > (NGPC_SIZEX+8))
        j = NGPC_SIZEX+8;
    for(i=8; i<j; i++)
        sprDef->gbp[i] = OOWCol;
    j = j + *wndSizeX;
    if (j > (NGPC_SIZEX+8))
        j = NGPC_SIZEX+8;
    for(i=j; i < (NGPC_SIZEX+8); i++)
        sprDef->gbp[i] = OOWCol;
}

static INLINE void lineFront(TILECACHE *tC)
{
    int    i;
    unsigned char a,b;
    const unsigned char *p2;
    unsigned short *gb;

    gb = tC->gbp;
    for (i=0;i<21;i++)
    {
        a = *(((unsigned char *)tC->tilept[i])+1);
        b = *((unsigned char *)tC->tilept[i]);
        if (tC->tile[i]&0x8000)
        {
            p2 = &mypatterns[b*4];
            if (p2[3])
                gb[0] = tC->palettes[i][p2[3]];
            if (p2[2])
                gb[1] = tC->palettes[i][p2[2]];
            if (p2[1])
                gb[2] = tC->palettes[i][p2[1]];
            if (p2[0])
                gb[3] = tC->palettes[i][p2[0]];
            p2 = &mypatterns[a*4];
            if (p2[3])
                gb[4] = tC->palettes[i][p2[3]];
            if (p2[2])
                gb[5] = tC->palettes[i][p2[2]];
            if (p2[1])
                gb[6] = tC->palettes[i][p2[1]];
            if (p2[0])
                gb[7] = tC->palettes[i][p2[0]];
        }
        else
        {
            p2 = &mypatterns[a*4];
            if (p2[0])
                gb[0] = tC->palettes[i][p2[0]];
            if (p2[1])
                gb[1] = tC->palettes[i][p2[1]];
            if (p2[2])
                gb[2] = tC->palettes[i][p2[2]];
            if (p2[3])
                gb[3] = tC->palettes[i][p2[3]];
            p2 = &mypatterns[b*4];
            if (p2[0])
                gb[4] = tC->palettes[i][p2[0]];
            if (p2[1])
                gb[5] = tC->palettes[i][p2[1]];
            if (p2[2])
                gb[6] = tC->palettes[i][p2[2]];
            if (p2[3])
                gb[7] = tC->palettes[i][p2[3]];
        }
        if (tC->tile[i]&0x4000)
            tC->tilept[i]-= 1;
        else
            tC->tilept[i]+= 1;
        gb+= 8;
    }
}

static INLINE void lineSprite(SPRITEDEFS *sprDefs)
{
   int i;
   SPRITE   *spr;
   unsigned short *gb;
   unsigned char a,b;
   const unsigned char *p2;

   for (i=sprDefs->count[*scanlineY];i>0;i--)
   {
      spr = &sprDefs->sprite[*scanlineY][i-1];
      gb = &sprDefs->gbp[spr->offset];
      a = *(((unsigned char *)spr->tilept)+1);
      b = *((unsigned char *)spr->tilept);
      if (spr->pattern&0x8000)
      {
         p2 = &mypatterns[b*4];
         if (p2[3])
            gb[0] = spr->palette[p2[3]];
         if (p2[2])
            gb[1] = spr->palette[p2[2]];
         if (p2[1])
            gb[2] = spr->palette[p2[1]];
         if (p2[0])
            gb[3] = spr->palette[p2[0]];
         p2 = &mypatterns[a*4];
         if (p2[3])
            gb[4] = spr->palette[p2[3]];
         if (p2[2])
            gb[5] = spr->palette[p2[2]];
         if (p2[1])
            gb[6] = spr->palette[p2[1]];
         if (p2[0])
            gb[7] = spr->palette[p2[0]];
      }
      else
      {
         p2 = &mypatterns[a*4];
         if (p2[0])
            gb[0] = spr->palette[p2[0]];
         if (p2[1])
            gb[1] = spr->palette[p2[1]];
         if (p2[2])
            gb[2] = spr->palette[p2[2]];
         if (p2[3])
            gb[3] = spr->palette[p2[3]];
         p2 = &mypatterns[b*4];
         if (p2[0])
            gb[4] = spr->palette[p2[0]];
         if (p2[1])
            gb[5] = spr->palette[p2[1]];
         if (p2[2])
            gb[6] = spr->palette[p2[2]];
         if (p2[3])
            gb[7] = spr->palette[p2[3]];
      }
   }
}

/* sort all the sprites according to their priorities */
static INLINE void spriteSortAll(unsigned int bw)
{
    unsigned int spriteCode;
    unsigned char x, y, prevx=0, prevy=0;
    unsigned int i, j, k, scanline;
    SPRITE *spr;

    SPRITEDEFS *SprPri40 = &spriteDefs[0];
    SPRITEDEFS *SprPri80 = &spriteDefs[1];
    SPRITEDEFS *SprPriC0 = &spriteDefs[2];

    /* initialize the number of sprites in each structure */
    memset(SprPri40->count, 0, 152);
    memset(SprPri80->count, 0, 152);
    memset(SprPriC0->count, 0, 152);
    for (i=0;i<64;i++)
    {
        spriteCode = *((unsigned short *)(sprite_table+4*i));

        if ((spriteCode<=0xff) || ((spriteCode & 0x1800)==0))
            continue;

        if (spriteCode & 0x0400)
            prevx+= *(sprite_table+4*i+2);
        else
            prevx = *(sprite_table+4*i+2) + 8;
        x = prevx + *scrollSpriteX;

        if (spriteCode & 0x0200)
            prevy+= *(sprite_table+4*i+3);
        else
            prevy = *(sprite_table+4*i+3) + 8;
        y = prevy + *scrollSpriteY;

        if (x>167 || y>151)
            continue;

        for(k=0; k<8; k++)
        {
            scanline = y+k-8;
            if(scanline < 0 || scanline >= 152)
                continue;

                switch (spriteCode & 0x1800)
                {
                    /* case order reversed because priority 3 (and 2) sprites occur most of the time */
                    case 0x1800:
                    spr = &SprPriC0->sprite[scanline][SprPriC0->count[scanline]++];
                    break;
                    case 0x1000:
                    spr = &SprPri80->sprite[scanline][SprPri80->count[scanline]++];
                    break;
                    case 0x0800:
                    spr = &SprPri40->sprite[scanline][SprPri40->count[scanline]++];
                    break;
                }
                j = scanline+8 - y;
                spr->pattern = spriteCode;
                spr->offset = x;
                spr->tilept = (unsigned short *)(pattern_table + 16*(spriteCode & 0x01ff)
                                        + ((spriteCode&0x4000) ? (7-j)*2 : j*2));
                spr->palette = ((bw) ? &palettes[(spriteCode>>11)&0x04]
                                : &palettes[(*(sprite_palette_numbers+i)&0x0F)*4]);
        }
    }
}


/* initialize drawing/blitting of a screen */
static void graphicsBlitInit(void)
{
    /* buffers 0 and 1
     * definitions for the back frame */
    tCBack.gbp   = &drawBuffer[8*SIZEX + (8-((*scrollBackX)&7))];
    tCBack.newScrollX = scrollBackX;
    tCBack.newScrollY = scrollBackY;
    tCBack.tileBase  = tile_table_back;
    tCBack.palette  = &palettes[16*4+16*4];
    /* definitions for the front frame */
    tCFront.gbp   = &drawBuffer[8*SIZEX + (8-((*scrollFrontX)&7))];
    tCFront.newScrollX = scrollFrontX;
    tCFront.newScrollY = scrollFrontY;
    tCFront.tileBase = tile_table_front;
    tCFront.palette  = &palettes[16*4];
    /* definitions for sprite priorities */
    spriteDefs[0].gbp = &drawBuffer[8*SIZEX];
    spriteDefs[1].gbp = &drawBuffer[8*SIZEX];
    spriteDefs[2].gbp = &drawBuffer[8*SIZEX];
    /* force recalculations for first line */
    tCBack.oldScrollX = *tCBack.newScrollX;
    tCBack.oldScrollY = *tCBack.newScrollY+1;   /* force calculation of structure data */
    tCFront.oldScrollX = *tCFront.newScrollX;
    tCFront.oldScrollY = *tCFront.newScrollY+1;  /* force calculation of structure data */
    /* start drawing at line 0 */
}

static INLINE void RenderTileCache(TILECACHE *tC, unsigned int bw)
{
    int    i;
    unsigned char line;
    unsigned short *temp;

    if ((tC->oldScrollX != *tC->newScrollX) ||
            (tC->oldScrollY != *tC->newScrollY) ||
            (((*tC->newScrollY + *scanlineY)&7) == 0))
    {
        /* update the structure to reflect the changed scroll registers
         * first update pointer into render buffer */
        tC->gbp = tC->gbp + (tC->oldScrollX&7) - (*tC->newScrollX&7);

        tC->oldScrollX = *tC->newScrollX;
        tC->oldScrollY = *tC->newScrollY;

        temp = tC->tileBase + (((tC->oldScrollY + *scanlineY)&0xf8)<<2);
        for (i=0;i<21;i++)
        {
            tC->tile[i] = *(temp + (((tC->oldScrollX>>3) + i)&31));
            line = (*tC->newScrollY + *scanlineY)&7;
            tC->palettes[i] = ((bw) ? tC->palette + ((tC->tile[i] & 0x2000) ? 4 : 0)
                                       : tC->palette + ((tC->tile[i]>>7) & 0x3C));
            tC->tilept[i] = (unsigned short *)(pattern_table + ((tC->tile[i] & 0x01ff)<<4));
            tC->tilept[i]+= ((tC->tile[i]&0x4000) ? 7-line : line);
        }
    }
}

/*
 * THOR'S GRAPHIC CORE
 */

typedef struct
{
	unsigned char flip;
	unsigned char x;
	unsigned char pal;
} MYSPRITE;

typedef struct
{
	unsigned short tile;
	unsigned char id;
} MYSPRITEREF;

typedef struct
{
	unsigned char count[152];
	MYSPRITEREF refs[152][64];
} MYSPRITEDEF;

MYSPRITEDEF mySprPri40,mySprPri80,mySprPriC0;
MYSPRITE mySprites[64];
unsigned short myPalettes[192];

void sortSprites(unsigned int bw)
{
    unsigned int spriteCode;
    unsigned char x, y, prevx=0, prevy=0;
    unsigned int i, j;
    unsigned short tile;
    MYSPRITEREF *spr;

    /* initialize the number of sprites in each structure */
    memset(mySprPri40.count, 0, 152);
    memset(mySprPri80.count, 0, 152);
    memset(mySprPriC0.count, 0, 152);

    for (i=0;i<64;i++)
    {
        spriteCode = *((unsigned short *)(sprite_table+4*i));

        prevx = (spriteCode & 0x0400 ? prevx : 0) + *(sprite_table+4*i+2);
        x = prevx + *scrollSpriteX;

        prevy = (spriteCode & 0x0200 ? prevy : 0) + *(sprite_table+4*i+3);
        y = prevy + *scrollSpriteY;

        if ((x>167 && x<249) || (y>151 && y<249) || (spriteCode<=0xff) || ((spriteCode & 0x1800)==0))
            continue;

		mySprites[i].x = x;
		mySprites[i].pal = bw ? ((spriteCode>>11)&0x04) : ((sprite_palette_numbers[i]&0xf)<<2);
		mySprites[i].flip = spriteCode>>8;
		tile = (spriteCode & 0x01ff)<<3;

        for (j = 0; j < 8; ++j,++y)
        {
        	if (y>151)
        		continue;
            switch (spriteCode & 0x1800)
            {
                case 0x1800:
                spr = &mySprPriC0.refs[y][mySprPriC0.count[y]++];
                break;
                case 0x1000:
                spr = &mySprPri80.refs[y][mySprPri80.count[y]++];
                break;
                case 0x0800:
                spr = &mySprPri40.refs[y][mySprPri40.count[y]++];
                break;
                default: continue;
            }
            spr->id = i;
            spr->tile = tile + (spriteCode&0x4000 ? 7-j : j);
        }
    }
}

void drawSprites(unsigned short* draw,
				 MYSPRITEREF *sprites,int count,
				 int x0,int x1)
{
	unsigned short*pal;
	unsigned int pattern,pix,cnt;
	MYSPRITE *spr;
	int i,cx;

	for (i=count-1;i>=0;--i)
	{
		pattern = patterns[sprites[i].tile];
		if (pattern==0)
			continue;

		spr = &mySprites[sprites[i].id];

		if (spr->x>248)
			cx = spr->x-256;
        else
			cx = spr->x;

		if (cx+8<=x0 || cx>=x1)
			continue;

		pal = &myPalettes[spr->pal];

        if (cx<x0)
        {
			cnt = 8-(x0-cx);
			if (spr->flip&0x80)
			{
                pattern>>=((8-cnt)<<1);
                for (cx=x0;pattern;++cx)
                {
                    pix = pattern & 0x3;
                    if (pix)
                        draw[cx] = pal[pix];
                    pattern>>=2;
                }
			}
			else
			{
                pattern &= (0xffff>>((8-cnt)<<1));
                for (cx = x0+cnt-1;pattern;--cx)
                {
                    pix = pattern & 0x3;
                    if (pix)
                        draw[cx] = pal[pix];
                    pattern>>=2;
                }
			}
        }
        else if (cx+7<x1)
        {
			if (spr->flip&0x80)
			{
                for (;pattern;++cx)
                {
                    pix = pattern & 0x3;
                    if (pix)
                        draw[cx] = pal[pix];
                    pattern>>=2;
                }
			}
			else
			{
                for (cx+=7;pattern;--cx)
                {
                    pix = pattern & 0x3;
                    if (pix)
                        draw[cx] = pal[pix];
                    pattern>>=2;
                }
			}
        }
        else
        {
			cnt = x1-cx;
			if (spr->flip&0x80)
			{
                pattern &= (0xffff>>((8-cnt)<<1));
                for (;pattern;++cx)
                {
                    pix = pattern & 0x3;
                    if (pix)
                        draw[cx] = pal[pix];
                    pattern>>=2;
                }
			}
			else
			{
                pattern>>=((8-cnt)<<1);
                for (cx+=cnt-1;pattern;--cx)
                {
                    pix = pattern & 0x3;
                    if (pix)
                        draw[cx] = pal[pix];
                    pattern>>=2;
                }
			}
        }
	}
}

void drawScrollPlane(unsigned short* draw,
					 unsigned short* tile_table,int scrpal,
					 unsigned char dx,unsigned char dy,
					 int x0,int x1,unsigned int bw)
{
	unsigned short*tiles;
	unsigned short*pal;
	unsigned int pattern;
	unsigned int j,count,pix,idy,tile;
	int i,x2;

	dx+=x0;
	tiles = tile_table+((dy>>3)<<5)+(dx>>3);

	count = 8-(dx&0x7);
	dx &= 0xf8;
	dy &= 0x7;
	idy = 7-dy;

	i = x0;

    if (count<8)
    {
		tile = *(tiles++);
		pattern = patterns[(((tile&0x1ff))<<3) + (tile&0x4000 ? idy:dy)];
		if (pattern)
		{
			pal = &myPalettes[scrpal + (bw ? (tile&0x2000 ? 4 : 0) : ((tile>>7)&0x3c))];
			if (tile&0x8000)
			{
                pattern>>=((8-count)<<1);
                for (j=i;pattern;++j)
                {
                    pix = pattern & 0x3;
                    if (pix)
                        draw[j] = pal[pix];
                    pattern>>=2;
                }
			}
			else
			{
                pattern &= (0xffff>>((8-count)<<1));
                for (j=i+count-1;pattern;--j)
                {
                    pix = pattern & 0x3;
                    if (pix)
                        draw[j] = pal[pix];
                    pattern>>=2;
                }
			}
		}
        i+=count;
		dx+=8;
		if (dx==0)
			tiles-=32;
    }

    x2 = i+((x1-i)&0xf8);

	for (;i<x2;i+=8)
	{
		tile = *(tiles++);
		pattern = patterns[(((tile&0x1ff))<<3) + (tile&0x4000 ? idy:dy)];
		if (pattern)
		{
			pal = &myPalettes[scrpal + (bw ? (tile&0x2000 ? 4 : 0) : ((tile>>7)&0x3c))];
			if (tile&0x8000)
			{
                for (j=i;pattern;++j)
                {
                    pix = pattern & 0x3;
                    if (pix)
                        draw[j] = pal[pix];
                    pattern>>=2;
                }
			}
			else
			{
                for (j=i+7;pattern;--j)
                {
                    pix = pattern & 0x3;
                    if (pix)
                        draw[j] = pal[pix];
                    pattern>>=2;
                }
			}
		}
		dx+=8;
		if (dx==0)
			tiles-=32;
	}

	if (x2!=x1)
	{
        count = x1-x2;
		tile = *(tiles++);
		pattern = patterns[(((tile&0x1ff))<<3) + (tile&0x4000 ? idy:dy)];
		if (pattern)
		{
			pal = &myPalettes[scrpal + (bw ? (tile&0x2000 ? 4 : 0) : ((tile>>7)&0x3c))];
			if (tile&0x8000)
			{
                pattern &= (0xffff>>((8-count)<<1));
                for (j=i;pattern;++j)
                {
                    pix = pattern & 0x3;
                    if (pix)
                        draw[j] = pal[pix];
                    pattern>>=2;
                }
			}
			else
			{
			    pattern>>=((8-count)<<1);
                for (j=i+count-1;pattern;--j)
                {
                    pix = pattern & 0x3;
                    if (pix)
                        draw[j] = pal[pix];
                    pattern>>=2;
                }
			}
		}
	}
}

void myGraphicsBlitLine(unsigned char render)  /* NOTA */
{
	int i,x0,x1;
    if (*scanlineY < 152)
    {
        if(render)
        {
#if __LIBRETRO__
			unsigned short* draw = &drawBuffer[(*scanlineY)*(screen->w)];
#else
			unsigned short* draw = &drawBuffer[(*scanlineY)*(screen->w+PSP_FUDGE)];/* extra fudge factor for PSP??? */
#endif
			unsigned short bgcol;
            unsigned int bw = (m_emuInfo.machine == NGP);
            unsigned short OOWCol = NGPC_TO_RGB565(oowTable[*oowSelect & 0x07]);
            unsigned short* pal;
            unsigned short* mempal;

			if (*scanlineY==0)
				sortSprites(bw);
			if (*scanlineY<*wndTopLeftY || *scanlineY>*wndTopLeftY+*wndSizeY || *wndSizeX==0 || *wndSizeY==0)
			{

				for (i=0;i<NGPC_SIZEX;i++)
					draw[i] = OOWCol;
			}
			else
			{
				if (((*scanlineY)&7) == 0)
				{
		            if (bw)
    		        {
        		        for(i=0;i<4;i++)
            		    {
	                	    myPalettes[i]     = NGPC_TO_RGB565(bwTable[bw_palette_table[i]    & 0x07]);
		                    myPalettes[4+i]   = NGPC_TO_RGB565(bwTable[bw_palette_table[4+i]  & 0x07]);
    		                myPalettes[64+i]  = NGPC_TO_RGB565(bwTable[bw_palette_table[8+i]  & 0x07]);
        		            myPalettes[68+i]  = NGPC_TO_RGB565(bwTable[bw_palette_table[12+i] & 0x07]);
            		        myPalettes[128+i] = NGPC_TO_RGB565(bwTable[bw_palette_table[16+i] & 0x07]);
                		    myPalettes[132+i] = NGPC_TO_RGB565(bwTable[bw_palette_table[20+i] & 0x07]);
		                }
    		        }
        		    else
					{
						pal = myPalettes;
						mempal = palette_table;

						for (i=0;i<192;i++)
							*(pal++) = NGPC_TO_RGB565(*(mempal++));
        		    }
				}


	            if(*bgSelect & 0x80)
    	            bgcol = NGPC_TO_RGB565(bgTable[*bgSelect & 0x07]);
        	    else if(bw)
	                bgcol = NGPC_TO_RGB565(bwTable[0]);
    	        else
        	        bgcol = NGPC_TO_RGB565(bgTable[0]); /* maybe 0xFFF? */
        	        
        	    /* NOTA arregla algo en m pure tlcsMemWriteB(0x83E0, 0xFF);     */
        	        

				x0 = *wndTopLeftX;
				x1 = x0+*wndSizeX;
				if (x1>NGPC_SIZEX)
					x1 = NGPC_SIZEX;

				for (i=x0;i<x1;i++)
					draw[i] = bgcol;

				if (mySprPri40.count[*scanlineY])
					drawSprites(draw,mySprPri40.refs[*scanlineY],mySprPri40.count[*scanlineY],x0,x1);

	            if (*frame1Pri & 0x80)
	            {
        			drawScrollPlane(draw,tile_table_front,64,*scrollFrontX,*scrollFrontY+*scanlineY,x0,x1,bw);
	            	if (mySprPri80.count[*scanlineY])
						drawSprites(draw,mySprPri80.refs[*scanlineY],mySprPri80.count[*scanlineY],x0,x1);
		        	 
		        	/* NOTA  Wrestling Madness && Big Bang Pro Wrestling */
		        	if (mainrom[0x000020] != 0x66)
		        	drawScrollPlane(draw,tile_table_back,128,*scrollBackX,*scrollBackY+*scanlineY,x0,x1,bw);
		        	
		        	else{
		        	if (*scrollBackY > 0)
		        	drawScrollPlane(draw,tile_table_back,128,*scrollBackX,*scrollBackY+*scanlineY,x0,x1,bw);
		        	else 
		        	drawScrollPlane(draw,tile_table_back,128,1,*scrollBackY+*scanlineY,x0,x1,bw);}
	            
	            }
	            else
	            {
		        	drawScrollPlane(draw,tile_table_back,128,*scrollBackX,*scrollBackY+*scanlineY,x0,x1,bw);
					if (mySprPri80.count[*scanlineY])
						drawSprites(draw,mySprPri80.refs[*scanlineY],mySprPri80.count[*scanlineY],x0,x1);
	    	    	drawScrollPlane(draw,tile_table_front,64,*scrollFrontX,*scrollFrontY+*scanlineY,x0,x1,bw);
	            }

				if (mySprPriC0.count[*scanlineY])
					drawSprites(draw,mySprPriC0.refs[*scanlineY],mySprPriC0.count[*scanlineY],x0,x1);

				for (i=0;i<x0;i++)
					draw[i] = OOWCol;
				for (i=x1;i<NGPC_SIZEX;i++)
					draw[i] = OOWCol;

	        }
        }
        if (*scanlineY == 151)
        {
            /* start VBlank period */
            tlcsMemWriteB(0x00008010,tlcsMemReadB(0x00008010) | 0x40);
            graphics_paint(render);
        }
        *scanlineY+= 1;
    }
    else if (*scanlineY == 198)
    {
        /* stop VBlank period */
        tlcsMemWriteB(0x00008010,tlcsMemReadB(0x00008010) & ~0x40);

        *scanlineY = 0;
    }
    else
        *scanlineY+= 1;
}


/*
 *
 * General Routines
 *
 */

BOOL graphics_init(void)
{
#ifdef __LIBRETRO__
    palette_init = palette_init16;
    palette_init(RMASK, GMASK, BMASK);
    drawBuffer = (unsigned short*)screen->pixels;
#else
    dbg_print("in graphics_init\n");

    switch (screen->format->BitsPerPixel)
    {
        case 8:
        palette_init = palette_init8;
        break;
        case 16:
        palette_init = palette_init16;
        break;
        case 32:
        palette_init = palette_init32;
        break;
    }

    drawBuffer = ((unsigned short*) screen->pixels) + SCREEN_OFFET;

    palette_init(screen->format->Rmask,
                 screen->format->Gmask, screen->format->Bmask);

    pngpalette_init();
#endif

    switch(m_emuInfo.machine)
    {
        case NGP:
            bgTable  = (unsigned short *)bwTable;
            oowTable = (unsigned short *)bwTable;
            graphicsBlitInit();
            *scanlineY = 0;
            break;
        case NGPC:
			bgTable  = (unsigned short *)get_address(0x000083E0);
			oowTable  = (unsigned short *)get_address(0x000083F0);
         graphicsBlitInit();
         *scanlineY = 0;
         break;
    }
    return TRUE;
}
