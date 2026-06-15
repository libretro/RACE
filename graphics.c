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

/*
 *
 * Palette Initialization
 *
 */

void (*palette_init)(uint32_t dwRBitMask, uint32_t dwGBitMask, uint32_t dwBBitMask);

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

void palette_init16(uint32_t dwRBitMask, uint32_t dwGBitMask, uint32_t dwBBitMask)
{
    char RShiftCount = 0, GShiftCount = 0, BShiftCount = 0;
    char RBitCount = 0, GBitCount = 0, BBitCount = 0;
    int  r,g,b;
    uint32_t i;

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

unsigned short palettes[16*4+16*4+16*4]; /* placeholder for the converted palette */

/* I think there's something wrong with this on the GP2X, because CFC2's intro screen is all red */
/* sort all the sprites according to their priorities */

/* initialize drawing/blitting of a screen */
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
			/* Full 8-pixel sprite row: expand each pattern byte through the
			 * mypatterns LUT and write the pixels straight out (bit-identical
			 * to the per-pixel shift loop, verified for all pattern values). */
			unsigned short* gb = &draw[cx];
			const unsigned char* p2;
			unsigned char lo = pattern & 0xff;
			unsigned char hi = (pattern >> 8) & 0xff;
			if (spr->flip&0x80)
			{
				p2 = &mypatterns[lo<<2];
				if (p2[3]) gb[0] = pal[p2[3]];
				if (p2[2]) gb[1] = pal[p2[2]];
				if (p2[1]) gb[2] = pal[p2[1]];
				if (p2[0]) gb[3] = pal[p2[0]];
				p2 = &mypatterns[hi<<2];
				if (p2[3]) gb[4] = pal[p2[3]];
				if (p2[2]) gb[5] = pal[p2[2]];
				if (p2[1]) gb[6] = pal[p2[1]];
				if (p2[0]) gb[7] = pal[p2[0]];
			}
			else
			{
				p2 = &mypatterns[hi<<2];
				if (p2[0]) gb[0] = pal[p2[0]];
				if (p2[1]) gb[1] = pal[p2[1]];
				if (p2[2]) gb[2] = pal[p2[2]];
				if (p2[3]) gb[3] = pal[p2[3]];
				p2 = &mypatterns[lo<<2];
				if (p2[0]) gb[4] = pal[p2[0]];
				if (p2[1]) gb[5] = pal[p2[1]];
				if (p2[2]) gb[6] = pal[p2[2]];
				if (p2[3]) gb[7] = pal[p2[3]];
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
			/* Plot a full 8-pixel tile row by expanding each byte of the
			 * pattern through the mypatterns LUT (byte -> 4 pixel indices)
			 * and writing the 8 pixels straight out, skipping transparent
			 * (index 0) pixels. This avoids the data-dependent shift loop and
			 * is bit-identical to it (verified for all pattern values). */
			{
				unsigned short* gb = &draw[i];
				const unsigned char* p2;
				unsigned char lo = pattern & 0xff;
				unsigned char hi = (pattern >> 8) & 0xff;
				if (tile&0x8000)
				{
					/* horizontal flip: reverse pixel order */
					p2 = &mypatterns[lo<<2];
					if (p2[3]) gb[0] = pal[p2[3]];
					if (p2[2]) gb[1] = pal[p2[2]];
					if (p2[1]) gb[2] = pal[p2[1]];
					if (p2[0]) gb[3] = pal[p2[0]];
					p2 = &mypatterns[hi<<2];
					if (p2[3]) gb[4] = pal[p2[3]];
					if (p2[2]) gb[5] = pal[p2[2]];
					if (p2[1]) gb[6] = pal[p2[1]];
					if (p2[0]) gb[7] = pal[p2[0]];
				}
				else
				{
					p2 = &mypatterns[hi<<2];
					if (p2[0]) gb[0] = pal[p2[0]];
					if (p2[1]) gb[1] = pal[p2[1]];
					if (p2[2]) gb[2] = pal[p2[2]];
					if (p2[3]) gb[3] = pal[p2[3]];
					p2 = &mypatterns[lo<<2];
					if (p2[0]) gb[4] = pal[p2[0]];
					if (p2[1]) gb[5] = pal[p2[1]];
					if (p2[2]) gb[6] = pal[p2[2]];
					if (p2[3]) gb[7] = pal[p2[3]];
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
			unsigned short* draw = &drawBuffer[(*scanlineY)*(screen->pitch)];
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

/* Point the renderer at a specific destination buffer and scanline stride
 * (in pixels). Used to render straight into the frontend's software
 * framebuffer when one is available, falling back to screen->pixels. */
void graphics_set_render_target(void *pixels, int pitch_pixels)
{
    drawBuffer    = (unsigned short*)pixels;
    screen->pitch = pitch_pixels;
}

BOOL graphics_init(void)
{
    palette_init = palette_init16;
    palette_init(RMASK, GMASK, BMASK);
    drawBuffer = (unsigned short*)screen->pixels;
    screen->pitch = screen->w;

    switch(m_emuInfo.machine)
    {
        case NGP:
            bgTable  = (unsigned short *)bwTable;
            oowTable = (unsigned short *)bwTable;
            *scanlineY = 0;
            break;
        case NGPC:
			bgTable  = (unsigned short *)get_address(0x000083E0);
			oowTable  = (unsigned short *)get_address(0x000083F0);
         *scanlineY = 0;
         break;
    }
    return TRUE;
}
