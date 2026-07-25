/*
  ym2151_dispatch.c - Unified FM Core Dispatcher for mdxmini
*/

#include <stdlib.h>
#include <string.h>
#include "ym2151.h"

static int g_fm_core = 0; /* 0 = MAME, 1 = Nuked OPM */

void mdx_set_fm_core(int core)
{
    g_fm_core = core;
}

int mdx_get_fm_core(void)
{
    return g_fm_core;
}

/* External Core Initializers */
extern void *MAME_YM2151Init(int index, int clock, int rate);
extern void  MAME_YM2151Shutdown(void *chip);
extern void  MAME_YM2151ResetChip(void *chip);
extern void  MAME_YM2151WriteReg(void *chip, int r, int v);
extern void  MAME_YM2151UpdateOne(void *chip, SAMP **buffers, int length);

extern void *Nuked_YM2151Init(int index, int clock, int rate);
extern void  Nuked_YM2151Shutdown(void *chip);
extern void  Nuked_YM2151ResetChip(void *chip);
extern void  Nuked_YM2151WriteReg(void *chip, int r, int v);
extern void  Nuked_YM2151UpdateOne(void *chip, SAMP **buffers, int length);

typedef struct {
    int core_type;
    void *chip_inst;
} YM2151_Dispatch;

void *YM2151Init(int index, int clock, int rate)
{
    YM2151_Dispatch *d = (YM2151_Dispatch *)malloc(sizeof(YM2151_Dispatch));
    if (!d) return NULL;

    d->core_type = g_fm_core;

    if (d->core_type == 1) {
        d->chip_inst = Nuked_YM2151Init(index, clock, rate);
    } else {
        d->chip_inst = MAME_YM2151Init(index, clock, rate);
    }

    if (!d->chip_inst) {
        free(d);
        return NULL;
    }
    return d;
}

void YM2151Shutdown(void *chip)
{
    YM2151_Dispatch *d = (YM2151_Dispatch *)chip;
    if (!d) return;

    if (d->core_type == 1) Nuked_YM2151Shutdown(d->chip_inst);
    else MAME_YM2151Shutdown(d->chip_inst);

    free(d);
}

void YM2151ResetChip(void *chip)
{
    YM2151_Dispatch *d = (YM2151_Dispatch *)chip;
    if (!d) return;

    if (d->core_type == 1) Nuked_YM2151ResetChip(d->chip_inst);
    else MAME_YM2151ResetChip(d->chip_inst);
}

void YM2151WriteReg(void *chip, int r, int v)
{
    YM2151_Dispatch *d = (YM2151_Dispatch *)chip;
    if (!d) return;

    if (d->core_type == 1) Nuked_YM2151WriteReg(d->chip_inst, r, v);
    else MAME_YM2151WriteReg(d->chip_inst, r, v);
}

void YM2151UpdateOne(void *chip, SAMP **buffers, int length)
{
    YM2151_Dispatch *d = (YM2151_Dispatch *)chip;
    if (!d) return;

    if (d->core_type == 1) Nuked_YM2151UpdateOne(d->chip_inst, buffers, length);
    else MAME_YM2151UpdateOne(d->chip_inst, buffers, length);
}