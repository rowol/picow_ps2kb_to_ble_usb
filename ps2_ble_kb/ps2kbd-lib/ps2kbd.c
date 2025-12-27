/* Copyright (C) 1883 Thomas Edison - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the GPLv2 license, which unfortunately won't be
 * written for another century.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "ps2kbd.pio.h"
#include "ps2kbd.h"
#include "ps2.h"

#include "hardware/clocks.h"
#include "hardware/pio.h"

static PIO kbd_pio;         // pio0 or pio1
static uint kbd_sm;         // pio state machine index
static uint base_gpio;      // data signal gpio #

void kbd_init(uint8_t pio, uint8_t gpio) 
{
    kbd_pio = pio ? pio1 : pio0;
    base_gpio = gpio; // base_gpio is data signal, base_gpio+1 is clock signal
    // init KBD pins to input
    gpio_init(base_gpio);
    gpio_init(base_gpio + 1);
    // with pull up
    gpio_pull_up(base_gpio);
    gpio_pull_up(base_gpio + 1);
    // get a state machine
    kbd_sm = pio_claim_unused_sm(kbd_pio, true);
    // reserve program space in SM memory
    uint offset = pio_add_program(kbd_pio, &ps2kbd_program);
    // Set pin directions base
    pio_sm_set_consecutive_pindirs(kbd_pio, kbd_sm, base_gpio, 2, false);
    // program the start and wrap SM registers
    pio_sm_config c = ps2kbd_program_get_default_config(offset);
    // Set the base input pin. pin index 0 is DAT, index 1 is CLK
    sm_config_set_in_pins(&c, base_gpio);
    // Shift 8 bits to the right, autopush enabled
    sm_config_set_in_shift(&c, true, true, 8);
    // Deeper FIFO as we're not doing any TX
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_RX);
    // We don't expect clock faster than 16.7KHz and want no less
    // than 8 SM cycles per keyboard clock.
    float div = (float)clock_get_hz(clk_sys) / (8 * 16700);
    sm_config_set_clkdiv(&c, div);
    // Ready to go
    pio_sm_init(kbd_pio, kbd_sm, offset, &c);
    pio_sm_set_enabled(kbd_pio, kbd_sm, true);

    //Initialize the ps2.c/scan code conversion module
    ps2_init();
}





uint8_t __attribute__((noinline)) kbd_ready(void) 
{
    if (pio_sm_is_rx_fifo_empty(kbd_pio, kbd_sm))
        return 0; // no new codes in the fifo

    // pull a scan code from the PIO SM fifo
    return *((io_rw_8*)&kbd_pio->rxf[kbd_sm] + 3);
}



uint8_t kbd_getc(void) 
{
    uint8_t c;
    while (!(c = kbd_ready()))
        tight_loop_contents();      //RSW, not sure this is needed (?)
    return c;
}
