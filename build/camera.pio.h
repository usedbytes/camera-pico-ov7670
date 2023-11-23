// -------------------------------------------------- //
// This file is autogenerated by pioasm; do not edit! //
// -------------------------------------------------- //

#pragma once

#if !PICO_NO_HARDWARE
#include "hardware/pio.h"
#endif

#define PIN_OFFS_VSYNC 0
#define PIN_OFFS_HREF 1
#define PIN_OFFS_D0 0
#define PIN_OFFS_D1 1
#define PIN_OFFS_D2 2
#define PIN_OFFS_D3 3
#define PIN_OFFS_D4 4
#define PIN_OFFS_D5 5
#define PIN_OFFS_D6 6
#define PIN_OFFS_D7 7
#define PIN_OFFS_PXCLK 8

// --------------- //
// camera_pio_byte //
// --------------- //

#define camera_pio_byte_wrap_target 0
#define camera_pio_byte_wrap 4

static const uint16_t camera_pio_byte_program_instructions[] = {
            //     .wrap_target
    0x20d4, //  0: wait   1 irq, 4 rel               
    0x21a8, //  1: wait   1 pin, 8               [1] 
    0x4008, //  2: in     pins, 8                    
    0x2028, //  3: wait   0 pin, 8                   
    0xc004, //  4: irq    nowait 4                   
            //     .wrap
};

#if !PICO_NO_HARDWARE
static const struct pio_program camera_pio_byte_program = {
    .instructions = camera_pio_byte_program_instructions,
    .length = 5,
    .origin = -1,
};

static inline pio_sm_config camera_pio_byte_program_get_default_config(uint offset) {
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + camera_pio_byte_wrap_target, offset + camera_pio_byte_wrap);
    return c;
}

static inline pio_sm_config camera_pio_get_byte_sm_config(PIO pio, uint sm, uint offset, uint base_pin_sm_s, uint bpp)
{
    pio_sm_config c = camera_pio_byte_program_get_default_config(offset);
    sm_config_set_in_pins(&c, base_pin_sm_s);
    sm_config_set_in_shift(&c, true, true, bpp);
    return c;
}

#endif

// ---------------- //
// camera_pio_frame //
// ---------------- //

#define camera_pio_frame_wrap_target 0
#define camera_pio_frame_wrap 17

#define camera_pio_frame_offset_loop_pixel 6u

static const uint16_t camera_pio_frame_program_instructions[] = {
            //     .wrap_target
    0x80a0, //  0: pull   block                      
    0x6040, //  1: out    y, 32                      
    0x80a0, //  2: pull   block                      
    0x20a0, //  3: wait   1 pin, 0                   
    0xa027, //  4: mov    x, osr                     
    0x22a1, //  5: wait   1 pin, 1               [2] 
    0xc025, //  6: irq    wait 5                     
    0x20c4, //  7: wait   1 irq, 4                   
    0xc025, //  8: irq    wait 5                     
    0x20c4, //  9: wait   1 irq, 4                   
    0xc025, // 10: irq    wait 5                     
    0x20c4, // 11: wait   1 irq, 4                   
    0xc025, // 12: irq    wait 5                     
    0x20c4, // 13: wait   1 irq, 4                   
    0x0046, // 14: jmp    x--, 6                     
    0x2021, // 15: wait   0 pin, 1                   
    0x0084, // 16: jmp    y--, 4                     
    0xc020, // 17: irq    wait 0                     
            //     .wrap
};

#if !PICO_NO_HARDWARE
static const struct pio_program camera_pio_frame_program = {
    .instructions = camera_pio_frame_program_instructions,
    .length = 18,
    .origin = -1,
};

static inline pio_sm_config camera_pio_frame_program_get_default_config(uint offset) {
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + camera_pio_frame_wrap_target, offset + camera_pio_frame_wrap);
    return c;
}

static inline void camera_pio_init_gpios(PIO pio, uint sm, uint base_pin_sm_0, uint base_pin_sm_s)
{
    pio_sm_set_consecutive_pindirs(pio, sm, base_pin_sm_0, 2, false);
    for (uint i = 0; i < 2; i++) {
        pio_gpio_init(pio, i + base_pin_sm_0);
    }
    pio_sm_set_consecutive_pindirs(pio, sm, base_pin_sm_s, 9, false);
    for (uint j = 0; j < 9; j++) {
        pio_gpio_init(pio, j + base_pin_sm_s);
    }
}
static inline pio_sm_config camera_pio_get_frame_sm_config(PIO pio, uint sm, uint offset, uint base_pin_sm_0)
{
    pio_sm_config c = camera_pio_frame_program_get_default_config(offset);
    sm_config_set_in_pins(&c, base_pin_sm_0);
    return c;
}
static inline bool camera_pio_frame_done(PIO pio)
{
    return pio_interrupt_get(pio, 0);
}
static inline void camera_pio_wait_for_frame_done(PIO pio)
{
    while (!camera_pio_frame_done(pio));
}
static inline void camera_pio_trigger_frame(PIO pio, uint32_t cols, uint32_t rows)
{
    pio_interrupt_clear(pio, 0);
    pio_sm_put_blocking(pio, 0, rows - 1);
    pio_sm_put_blocking(pio, 0, cols - 1);
}

#endif

// --------------- //
// pixel_loop_yuyv //
// --------------- //

#define pixel_loop_yuyv_wrap_target 0
#define pixel_loop_yuyv_wrap 7

static const uint16_t pixel_loop_yuyv_program_instructions[] = {
            //     .wrap_target
    0xc025, //  0: irq    wait 5                     
    0x20c4, //  1: wait   1 irq, 4                   
    0xc025, //  2: irq    wait 5                     
    0x20c4, //  3: wait   1 irq, 4                   
    0xc025, //  4: irq    wait 5                     
    0x20c4, //  5: wait   1 irq, 4                   
    0xc025, //  6: irq    wait 5                     
    0x20c4, //  7: wait   1 irq, 4                   
            //     .wrap
};

#if !PICO_NO_HARDWARE
static const struct pio_program pixel_loop_yuyv_program = {
    .instructions = pixel_loop_yuyv_program_instructions,
    .length = 8,
    .origin = -1,
};

static inline pio_sm_config pixel_loop_yuyv_program_get_default_config(uint offset) {
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + pixel_loop_yuyv_wrap_target, offset + pixel_loop_yuyv_wrap);
    return c;
}
#endif

// ----------------- //
// pixel_loop_rgb565 //
// ----------------- //

#define pixel_loop_rgb565_wrap_target 0
#define pixel_loop_rgb565_wrap 7

static const uint16_t pixel_loop_rgb565_program_instructions[] = {
            //     .wrap_target
    0xc025, //  0: irq    wait 5                     
    0x20c4, //  1: wait   1 irq, 4                   
    0xc025, //  2: irq    wait 5                     
    0x20c4, //  3: wait   1 irq, 4                   
    0xa042, //  4: nop                               
    0xa042, //  5: nop                               
    0xa042, //  6: nop                               
    0xa042, //  7: nop                               
            //     .wrap
};

#if !PICO_NO_HARDWARE
static const struct pio_program pixel_loop_rgb565_program = {
    .instructions = pixel_loop_rgb565_program_instructions,
    .length = 8,
    .origin = -1,
};

static inline pio_sm_config pixel_loop_rgb565_program_get_default_config(uint offset) {
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + pixel_loop_rgb565_wrap_target, offset + pixel_loop_rgb565_wrap);
    return c;
}
#endif

// --------------- //
// pixel_loop_nv16 //
// --------------- //

#define pixel_loop_nv16_wrap_target 0
#define pixel_loop_nv16_wrap 7

static const uint16_t pixel_loop_nv16_program_instructions[] = {
            //     .wrap_target
    0xc025, //  0: irq    wait 5                     
    0x20c4, //  1: wait   1 irq, 4                   
    0xc026, //  2: irq    wait 6                     
    0x20c4, //  3: wait   1 irq, 4                   
    0xc025, //  4: irq    wait 5                     
    0x20c4, //  5: wait   1 irq, 4                   
    0xc026, //  6: irq    wait 6                     
    0x20c4, //  7: wait   1 irq, 4                   
            //     .wrap
};

#if !PICO_NO_HARDWARE
static const struct pio_program pixel_loop_nv16_program = {
    .instructions = pixel_loop_nv16_program_instructions,
    .length = 8,
    .origin = -1,
};

static inline pio_sm_config pixel_loop_nv16_program_get_default_config(uint offset) {
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + pixel_loop_nv16_wrap_target, offset + pixel_loop_nv16_wrap);
    return c;
}
#endif

// --------------- //
// pixel_loop_yu16 //
// --------------- //

#define pixel_loop_yu16_wrap_target 0
#define pixel_loop_yu16_wrap 7

static const uint16_t pixel_loop_yu16_program_instructions[] = {
            //     .wrap_target
    0xc025, //  0: irq    wait 5                     
    0x20c4, //  1: wait   1 irq, 4                   
    0xc026, //  2: irq    wait 6                     
    0x20c4, //  3: wait   1 irq, 4                   
    0xc025, //  4: irq    wait 5                     
    0x20c4, //  5: wait   1 irq, 4                   
    0xc027, //  6: irq    wait 7                     
    0x20c4, //  7: wait   1 irq, 4                   
            //     .wrap
};

#if !PICO_NO_HARDWARE
static const struct pio_program pixel_loop_yu16_program = {
    .instructions = pixel_loop_yu16_program_instructions,
    .length = 8,
    .origin = -1,
};

static inline pio_sm_config pixel_loop_yu16_program_get_default_config(uint offset) {
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + pixel_loop_yu16_wrap_target, offset + pixel_loop_yu16_wrap);
    return c;
}

static inline void camera_pio_patch_pixel_loop(PIO pio, uint offset, const pio_program_t *loop) {
    uint i;
    // TODO: Assert that length of program is 8?
    for (i = 0; i < loop->length; i++) {
        pio->instr_mem[offset + camera_pio_frame_offset_loop_pixel + i] = loop->instructions[i];
    }
}

#endif

