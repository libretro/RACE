/* Flavor modified sound.c and sound.h from NEOPOP
 *  which was originally based on sn76496.c from MAME
 *  some ideas also taken from NeoPop-SDL code

 *---------------------------------------------------------------------------
 * Originally from
 * NEOPOP : Emulator as in Dreamland
 *
 * Copyright (c) 2001-2002 by neopop_uk
 *---------------------------------------------------------------------------

 *---------------------------------------------------------------------------
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version. See also the license.txt file for
 *	additional informations.
 *---------------------------------------------------------------------------
 */


/************************************************************************
 *                                                                      *
 *	Portions, but not all of this source file are based on MAME v0.60	*
 *	File "sn76496.c". All copyright goes to the original author.		*
 *	The remaining parts, including DAC processing, by neopop_uk			*
 *                                                                      *
 ************************************************************************/

#include "types.h"

#include <string.h>

#include "neopopsound.h"
#include "neopop_blip.h"

/* ============================================================================= */

SoundChip toneChip;
SoundChip noiseChip;

/* ==== DAC */
#define DAC_BUFFERSIZE		(256 * 1024) /* at (256 * 1024) the PC version will crash on MS2 intro */

int dacLBufferRead, dacLBufferWrite, dacLBufferCount;
uint16_t dacBufferL[DAC_BUFFERSIZE];
int fixsoundmahjong;

/* ============================================================================= */

#define SOUNDCHIPCLOCK	(3072000)	/* Unverified / sounds correct */

#define MAX_OUTPUT 0x7fff
#define STEP 0x10000		/* Fixed point adjuster */

#define MAX_OUTPUT_STEP 0x7fff0000
#define STEP_SHIFT 16

static uint32_t VolTable[16];
static uint32_t UpdateStep = 0;	/* Number of steps during one sample. */

/* Formulas for noise generator */
/* bit0 = output */

/* noise feedback for white noise mode (verified on real SN76489 by John Kortink) */
#define FB_WNOISE 0x14002	/* (16bits) bit16 = bit0(out) ^ bit2 ^ bit15 */

/* noise feedback for periodic noise mode */
#define FB_PNOISE 0x08000	/* 15bit rotate */

/* noise generator start preset (for periodic noise) */
#define NG_PRESET 0x0f35

#define max(a,b) (a>b?a:b)
#define min(a,b) (a<b?a:b)

/* ============================================================================= */

static uint16_t sample_chip_tone(void)
{
   int i;
   int vol[3];
   unsigned int out;

   /* vol[] keeps track of how long each square wave stays */
   /* in the 1 position during the sample period. */
   vol[0] = vol[1] = vol[2] = /*vol[3] = */ 0;

   for (i = 0; i < 3; i++)
   {
      if (toneChip.Output[i]) vol[i] += toneChip.Count[i];
      toneChip.Count[i] -= STEP;

      /* Period[i] is the half period of the square wave. Here, in each */
      /* loop I add Period[i] twice, so that at the end of the loop the */
      /* square wave is in the same status (0 or 1) it was at the start. */
      /* vol[i] is also incremented by Period[i], since the wave has been 1 */
      /* exactly half of the time, regardless of the initial position. */
      /* If we exit the loop in the middle, Output[i] has to be inverted */
      /* and vol[i] incremented only if the exit status of the square */
      /* wave is 1. */

      while (toneChip.Count[i] <= 0)
      {
         toneChip.Count[i] += toneChip.Period[i];
         if (toneChip.Count[i] > 0)
         {
            toneChip.Output[i] ^= 1;
            if (toneChip.Output[i]) vol[i] += toneChip.Period[i];
            break;
         }
         toneChip.Count[i] += toneChip.Period[i];
         vol[i] += toneChip.Period[i];
      }
      if (toneChip.Output[i]) vol[i] -= toneChip.Count[i];
   }

   out = vol[0] * toneChip.Volume[0] + vol[1] * toneChip.Volume[1] +
      vol[2] * toneChip.Volume[2];

   if (out > MAX_OUTPUT_STEP)
      out = MAX_OUTPUT_STEP;

   return out>>STEP_SHIFT;
}

/* ============================================================================= */

static uint16_t sample_chip_noise(void)
{
   int vol3 = 0;
   unsigned int out;
   int left;

   /* vol[] keeps track of how long each square wave stays */
   /* in the 1 position during the sample period. */
   if (noiseChip.Volume[3])
   {
      left = STEP;
      do
      {
         int nextevent = min(noiseChip.Count[3],left);

         if (noiseChip.Output[3])
            vol3 += noiseChip.Count[3];
         noiseChip.Count[3] -= nextevent;
         if (noiseChip.Count[3] <= 0)
         {
            if (noiseChip.RNG & 1)
               noiseChip.RNG ^= noiseChip.NoiseFB;
            noiseChip.RNG >>= 1;
            noiseChip.Output[3] = noiseChip.RNG & 1;
            noiseChip.Count[3] += noiseChip.Period[3];
            if (noiseChip.Output[3])
               vol3 += noiseChip.Period[3];
         }
         if (noiseChip.Output[3])
            vol3 -= noiseChip.Count[3];

         left -= nextevent;
      } while (left > 0);
   }
   out = vol3 * noiseChip.Volume[3];

   if (out > MAX_OUTPUT_STEP)
      out = MAX_OUTPUT_STEP;

   return out>>STEP_SHIFT;
}

/* ============================================================================= */

/* One-pole DC-blocking high-pass removes the PSG's DC offset so the output is
 * centered (matching the Mednafen/beetle-ngp reference) instead of unipolar.
 * y[n] = x[n] - x[n-1] + R*y[n-1], R for a ~10 Hz corner @ 44100 (below all
 * musical content, so bass is preserved). A double accumulator avoids the
 * fixed-point truncation bias that would otherwise reintroduce a DC offset. */
static double dcblock_xprev = 0.0;
static double dcblock_yprev = 0.0;
#define DCBLOCK_R 0.99858

void sound_update(uint16_t* chip_buffer, int length_bytes)
{
   length_bytes >>= 1; /* turn it into words */
   while (length_bytes)
   {
      /* Mix a mono track out of: (Tone + Noise) >> 1, then remove DC. */
      double x = (double)((sample_chip_tone() + sample_chip_noise()) >> 1);
      double y = x - dcblock_xprev + DCBLOCK_R * dcblock_yprev;
      int s;
      dcblock_xprev = x;
      dcblock_yprev = y;
      s = (int)(y >= 0.0 ? y + 0.5 : y - 0.5);
      if (s >  32767) s =  32767;
      if (s < -32768) s = -32768;
      *(chip_buffer++) = (uint16_t)(int16_t)s;

      length_bytes--;
   }
}

/* ============================================================================= */

void WriteSoundChip(SoundChip* chip, uint8_t data)
{
	/* Command */
	if (data & 0x80)
	{
		int r = (data & 0x70) >> 4;
		int c = r>>1;

		chip->LastRegister = r;
		chip->Register[r] = (chip->Register[r] & 0x3f0) | (data & 0x0f);

		switch(r)
      {
         case 0:	/* tone 0 : frequency */
         case 2:	/* tone 1 : frequency */
         case 4:	/* tone 2 : frequency */
            chip->Period[c] = UpdateStep * chip->Register[r];
            if (chip->Period[c] == 0)
               chip->Period[c] = UpdateStep;
            if (r == 4)
            {
               /* update noise shift frequency */
               if ((chip->Register[6] & 0x03) == 0x03)
                  chip->Period[3] = chip->Period[2]<<1;
            }
            break;

         case 1:	/* tone 0 : volume */
         case 3:	/* tone 1 : volume */
         case 5:	/* tone 2 : volume */
         case 7:	/* noise  : volume */
            chip->Volume[c] = VolTable[data & 0xF];
            break;

         case 6:	/* noise  : frequency, mode */
            {
               int n = chip->Register[6];
               chip->NoiseFB = (n & 4) ? FB_WNOISE : FB_PNOISE;
               n &= 3;
               /* N/512,N/1024,N/2048,Tone #2 output */
               chip->Period[3] = (n == 3) ? 2 * chip->Period[2] : (UpdateStep << (5+n));

               /* reset noise shifter */
               chip->RNG = NG_PRESET;
               chip->Output[3] = chip->RNG & 1;

            }
            break;
      }
	}
	else
	{
		int r = chip->LastRegister;
		int c = r/2;

		switch (r)
      {
         case 0:	/* tone 0 : frequency */
         case 2:	/* tone 1 : frequency */
         case 4:	/* tone 2 : frequency */
            chip->Register[r] = (chip->Register[r] & 0x0f) | ((data & 0x3f) << 4);
            chip->Period[c] = UpdateStep * chip->Register[r];
            if (chip->Period[c] == 0) chip->Period[c] = UpdateStep;
            if (r == 4)
            {
               /* update noise shift frequency */
               if ((chip->Register[6] & 0x03) == 0x03)
                  chip->Period[3] = chip->Period[2]<<1;
            }
            break;
      }
	}
}

/* ============================================================================= */

void dac_writeL(unsigned char data)
{
   unsigned i;
   static int conv=5;

   if (neopop_audio_accurate)
   {
      /* In band-limited mode the DAC change is emitted as a Blip transition at
       * the current timestamp; the fast-path ring buffer below is bypassed. */
      neopop_blip_dac(data);
      return;
   }

   /* pretend that conv=5.5 (44100/8000) conversion factor */

   if(conv==5)
      conv=6;
   else
   {
      conv=5;

      /* Arregla el sonido del Super Real Mahjong */
      if (fixsoundmahjong>500)
         conv=3;
   }    


   for(i=0;i<conv;i++)
   {
      /* Write to buffer */
      dacBufferL[dacLBufferWrite++] = (data-0x80)<<8;

      if (dacLBufferWrite == DAC_BUFFERSIZE)
         dacLBufferWrite = 0;

      /* Overflow? */
      dacLBufferCount++;
      if (dacLBufferCount == DAC_BUFFERSIZE)
         dacLBufferCount = 0;
   }

}
 
void dac_update(uint16_t* dac_buffer, int length_bytes)
{
	while (length_bytes > 1)
	{
		/* Mix the DAC sample onto the PSG output. A clamped additive mix is
		 * correct; the previous bitwise OR discarded the DAC contribution
		 * whenever its bits overlapped the PSG sample (and produced garbage
		 * for negative DAC values). */
		{
			int s = (int)(int16_t)*dac_buffer + (int)(int16_t)dacBufferL[dacLBufferRead];
			if (s >  32767) s =  32767;
			if (s < -32768) s = -32768;
			*dac_buffer = (uint16_t)(int16_t)s;
		}
		dac_buffer++;
		dacBufferL[dacLBufferRead] = 0;  /* silence? */

		length_bytes -= 2;	/* 1 byte = 8 bits */

		if (dacLBufferCount > 0)
		{
			dacLBufferCount--;

			/* Advance the DAC read */
			if (++dacLBufferRead == DAC_BUFFERSIZE)
				dacLBufferRead = 0;
		}
	}
}

/*============================================================================= */

/*Resets the sound chips, also used whenever sound options are changed */
void sound_init(int SampleRate)
{
	int i;
	double out;

	/* the base clock for the tone generators is the chip clock divided by 16; */
	/* for the noise generator, it is clock / 256. */
	/* Here we calculate the number of steps which happen during one sample */
	/* at the given sample rate. No. of events = sample rate / (clock/16). */
	/* STEP is a multiplier used to turn the fraction into a fixed point */
	/* number. */
	UpdateStep = (uint32_t)(((double)STEP * SampleRate * 16) / SOUNDCHIPCLOCK);

	/* Initialise Left Chip */
	memset(&toneChip, 0, sizeof(SoundChip));

	/* Initialise Right Chip */
	memset(&noiseChip, 0, sizeof(SoundChip));

	/* Default register settings */
	for (i = 0;i < 8;i+=2)
	{
		toneChip.Register[i] = 0;
		toneChip.Register[i + 1] = 0x0f;	/* volume = 0 */
		noiseChip.Register[i] = 0;
		noiseChip.Register[i + 1] = 0x0f;	/* volume = 0 */
	}

	for (i = 0;i < 4;i++)
	{
		toneChip.Output[i] = 0;
		toneChip.Period[i] = toneChip.Count[i] = UpdateStep;
		noiseChip.Output[i] = 0;
		noiseChip.Period[i] = noiseChip.Count[i] = UpdateStep;
	}

	/* Build the volume table */
	out = MAX_OUTPUT / 3;

	/* build volume table (2dB per step) */
	for (i = 0;i < 15;i++)
	{
		VolTable[i] = (uint32_t)out;
		out /= 1.258925412;	/* = 10 ^ (2/20) = 2dB */
	}
	VolTable[15] = 0;

	/* Clear the DAC buffer */
	for (i = 0; i < DAC_BUFFERSIZE; i++)
		dacBufferL[i] = 0;

	dacLBufferCount = 0;
	dacLBufferRead  = 0;
	dacLBufferWrite = 0;

	/* Initialise the band-limited (accurate) path as well; it is only used
	 * when neopop_audio_accurate is set, but keeping it in sync here means a
	 * mid-game quality switch starts from a clean state. */
	neopop_blip_init(SampleRate);
}

/* ============================================================================= */

void system_sound_chipreset(void)
{
   /* Initialises sound chips, matching frequencies */
   sound_init(44100);
}

int sound_system_init(void)
{
   system_sound_chipreset();	/* Resets chips */
   return 1;
}

/* Accessors for the band-limited (Blip) audio path, so it reads exactly the
 * same decoded oscillator state that sample_chip_tone/sample_chip_noise use,
 * rather than re-decoding the registers (which split tone frequency and volume
 * across the two T6W28 chips and is easy to get wrong). */
int neopop_sound_tone_divider(int chan)
{
   if (chan < 0 || chan > 2 || UpdateStep == 0)
      return 1;
   /* Period[c] == UpdateStep * raw_divider; recover the raw divider. */
   return (int)(toneChip.Period[chan] / UpdateStep);
}

int neopop_sound_tone_volume(int chan)
{
   if (chan < 0 || chan > 2)
      return 0;
   return toneChip.Volume[chan];
}

int neopop_sound_noise_divider(void)
{
   if (UpdateStep == 0)
      return 1;
   return (int)(noiseChip.Period[3] / UpdateStep);
}

int neopop_sound_noise_volume(void)
{
   return noiseChip.Volume[3];
}

int neopop_sound_noise_feedback_periodic(void)
{
   /* FB_PNOISE (periodic) vs FB_WNOISE (white). Match on the periodic mask. */
   return (noiseChip.NoiseFB == FB_PNOISE) ? 1 : 0;
}
