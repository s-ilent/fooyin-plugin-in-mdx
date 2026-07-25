/*
  ym2151_nuked.c - Nuked-OPM Core Wrapper
*/

#include <stdlib.h>
#include <string.h>
#include "opm.h"
#include "ym2151.h"

#define WRITE_QUEUE_SIZE 2048

typedef struct {
    uint8_t reg;
    uint8_t val;
} write_req_t;

typedef struct {
    opm_t chip;
    uint32_t clock;
    uint32_t rate;
    double clocks_per_sample;
    double clock_accumulator;
    int32_t last_out[2];

    write_req_t write_queue[WRITE_QUEUE_SIZE];
    int write_queue_head;
    int write_queue_tail;
    int write_state;
    write_req_t current_req;
} nuked_ym2151_t;

static void step_nuked_chip(nuked_ym2151_t *inst)
{
    /*
      OPM_Clock() runs OPM_DoRegWrite() BEFORE OPM_DoIO().

      state 0: If write_queue has pending write request:
               Pop request into current_req.
               Call OPM_Write(&chip, 0, current_req.reg) -> sets write_a = 1.
               Transition to state = 1.
      state 1: Idle clock. OPM_Clock runs:
               OPM_DoIO moves write_a to write_a_en (1).
               Transition to state = 2.
      state 2: OPM_Clock runs: OPM_DoRegWrite sees write_a_en == 1,
               latches reg_address = current_req.reg, and sets reg_address_ready = 1.
               OPM_DoIO clears write_a_en = 0.
               Call OPM_Write(&chip, 1, current_req.val) -> sets write_d = 1.
               Transition to state = 3.
      state 3: Idle clock. OPM_Clock runs:
               OPM_DoIO moves write_d to write_d_en (1).
               Transition to state = 4.
      state 4..35: OPM_Clock runs: OPM_DoRegWrite sees write_d_en == 1,
               latches reg_data = current_req.val, sets reg_data_ready = 1,
               and triggers mode writes or internal operator slot RAM writes.
               32 clock cycles allow Nuked-OPM's 32-slot pipeline to cycle through
               all operator slots and commit the register data.
               When state reaches 36, transition back to state = 0.
    */

    switch (inst->write_state) {
    case 0:
        if (inst->write_queue_head != inst->write_queue_tail) {
            inst->current_req = inst->write_queue[inst->write_queue_head];
            inst->write_queue_head = (inst->write_queue_head + 1) % WRITE_QUEUE_SIZE;
            OPM_Write(&inst->chip, 0, inst->current_req.reg);
            inst->write_state = 1;
        }
        break;

    case 1:
        inst->write_state = 2;
        break;

    case 2:
        OPM_Write(&inst->chip, 1, inst->current_req.val);
        inst->write_state = 3;
        break;

    case 3:
        inst->write_state = 4;
        break;

    default:
        inst->write_state++;
        if (inst->write_state >= 36) {
            inst->write_state = 0;
        }
        break;
    }

    OPM_Clock(&inst->chip, inst->last_out, NULL, NULL, NULL);
}

void *Nuked_YM2151Init(int index, int clock, int rate)
{
    (void)index;
    nuked_ym2151_t *inst = (nuked_ym2151_t *)malloc(sizeof(nuked_ym2151_t));
    if (!inst) return NULL;
    memset(inst, 0, sizeof(nuked_ym2151_t));

    inst->clock = clock ? (uint32_t)clock : 4000000;
    inst->rate = rate ? (uint32_t)rate : 44100;

    inst->clocks_per_sample = ((double)inst->clock / 2.0) / (double)inst->rate;
    inst->clock_accumulator = 0.0;

    OPM_Reset(&inst->chip, opm_flags_none);
    return inst;
}

void Nuked_YM2151Shutdown(void *chip)
{
    if (chip) free(chip);
}

void Nuked_YM2151ResetChip(void *chip)
{
    nuked_ym2151_t *inst = (nuked_ym2151_t *)chip;
    if (!inst) return;

    OPM_Reset(&inst->chip, opm_flags_none);
    inst->clock_accumulator = 0.0;
    inst->last_out[0] = 0;
    inst->last_out[1] = 0;
    inst->write_queue_head = 0;
    inst->write_queue_tail = 0;
    inst->write_state = 0;
}

void Nuked_YM2151WriteReg(void *chip, int r, int v)
{
    nuked_ym2151_t *inst = (nuked_ym2151_t *)chip;
    if (!inst) return;

    int next_tail = (inst->write_queue_tail + 1) % WRITE_QUEUE_SIZE;
    while (next_tail == inst->write_queue_head) {
        step_nuked_chip(inst);
    }

    inst->write_queue[inst->write_queue_tail].reg = (uint8_t)(r & 0xFF);
    inst->write_queue[inst->write_queue_tail].val = (uint8_t)(v & 0xFF);
    inst->write_queue_tail = next_tail;
}

void Nuked_YM2151UpdateOne(void *chip, SAMP **buffers, int length)
{
    nuked_ym2151_t *inst = (nuked_ym2151_t *)chip;
    if (!inst || !buffers || !buffers[0] || !buffers[1]) return;

    SAMP *bufL = buffers[0];
    SAMP *bufR = buffers[1];

    for (int i = 0; i < length; i++) {
        inst->clock_accumulator += inst->clocks_per_sample;
        while (inst->clock_accumulator >= 1.0) {
            step_nuked_chip(inst);
            inst->clock_accumulator -= 1.0;
        }

        int32_t out0 = inst->last_out[0];
        int32_t out1 = inst->last_out[1];
        if (out0 > 32767) out0 = 32767;
        else if (out0 < -32768) out0 = -32768;
        if (out1 > 32767) out1 = 32767;
        else if (out1 < -32768) out1 = -32768;

        bufL[i] = (SAMP)out0;
        bufR[i] = (SAMP)out1;
    }
}