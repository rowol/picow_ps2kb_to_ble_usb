/* Copyright (C) 1883 Thomas Edison - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the GPLv2 license, which unfortunately won't be
 * written for another century.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <stdio.h>

#include "ps2kbd.pio.h"
#include "ps2kbd.h"
#include "ps2.h"

#include "hardware/clocks.h"
#include "hardware/pio.h"

#include "pico/time.h"



//keyboard suport pio number. pio0 or pio1
#define KB_PIO pio1

// KBD data and clock inputs, must be consecutive with data in the lower position.
// GPIO number of data pin, clk pin must be on next adjacent GPIO
#define KB_DAT_GPIO 14 // PS/2 data
//#define CLK_GPIO 15  // PS/2 clock (RSW: This is not used, kbd_init uses DAT_GPIO+1 for the clock)



static PIO kbd_pio;         // pio0 or pio1
static uint kbd_sm;         // pio state machine index
static uint base_gpio;      // data signal gpio #



void kbd_init(void)
{
   kbd_pio = KB_PIO;
   base_gpio = KB_DAT_GPIO;   // base_gpio is data signal, base_gpio+1 is clock signal
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
   sm_config_set_in_shift(&c, true, true, 1 + 8 + 1 + 1);  // Include start, 8 data, parity, and stop bits

   // RSW set up pin to use for jumps
   //  Crucially, configure which specific pin the JMP PIN instruction will read
   //  (All pins are visible to PIO input logic, shouldn't need to pio_gpio_init or gpio_set_dir on the pin)
   sm_config_set_jmp_pin(&c, base_gpio);  // Use the data pin for "jmp pin" tests
   // pio_gpio_init(kbd_pio, base_gpio);
   // gpio_set_dir(base_gpio, GPIO_IN);

   // Deeper FIFO as we're not doing any TX
   sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_RX);
   // We don't expect clock faster than 16.7KHz and want no less
   // than 8 SM cycles per keyboard clock.
   float div = (float)clock_get_hz(clk_sys) / (8 * 16700);
   sm_config_set_clkdiv(&c, div);
   // Ready to go
   pio_sm_init(kbd_pio, kbd_sm, offset, &c);
   pio_sm_set_enabled(kbd_pio, kbd_sm, true);

   // Initialize the ps2.c/scan code conversion module
   ps2_init();
}


static void kbd_reset(void)
{
   pio_sm_clear_fifos(kbd_pio, kbd_sm);
   pio_sm_restart(kbd_pio, kbd_sm);
}


uint8_t __attribute__((noinline)) kbd_ready(void) 
{ 
   #define START_BIT    0x00200000
   #define PARITY_BIT   0x40000000
   #define STOP_BIT     0x80000000

   #define SCANCODE_SHIFT 22


   if (pio_sm_is_rx_fifo_empty(kbd_pio, kbd_sm))
      return 0; // no new codes in the fifo

   // pull a scan code from the PIO SM fifo
   // if start, stop, and parity don't check out, toss the character, reset the PIO, and dump any keys we think are pressed
   uint32_t dw = kbd_pio->rxf[kbd_sm];
   
   if (dw & START_BIT) {
      printf("PS/2 sync error, start bit not low\n");
      kbd_reset();
      ps2_clear_all_keys();   //Okay to dump all pressed keys?
      return 0;
   }

   if (!(dw & STOP_BIT)) {
      printf("PS/2 sync error, stop bit not high\n");
      kbd_reset();
      ps2_clear_all_keys();   //Okay to dump all pressed keys?
      return 0;
   }


   uint8_t scancode = (dw >> SCANCODE_SHIFT) & 0xFF;  // Pull scancode byte out without start, partiy, and stop bits

   //Check scancode parity (PS/2 uses odd parity, i.e. 1 for even number of high bits)
   bool bOddParity = !(__builtin_popcount(scancode) & 1);
   if (bOddParity != (bool)(dw & PARITY_BIT)) {
      printf("PS/2 sync error, parity check failed\n");
      kbd_reset();
      ps2_clear_all_keys();   //Should maybe also dump all pressed keys?
      return 0;
   }

   return scancode;
}



uint8_t kbd_getc(void)
{
   uint8_t c;
   while (!(c = kbd_ready()))
      tight_loop_contents();  // RSW, not sure this is needed (?)
   return c;
}



//PS/2 set/reset LED command stuff
#define SET_LEDS_CMD       0xED
#define ECHO_CMD           0xEE

#define CAPS_LOCK_MASK     0x04
#define NUM_LOCK_MASK      0x02
#define SCROLL_LOCK_MASK   0x01

#define PS2_DAT_GPIO       (base_gpio)
#define PS2_CLK_GPIO       (base_gpio+1)

//PS/2 spec says for host to device, write data on the falling clk edge
//and kb will read on the rising edge
//Assume clock high on entry
static void kbd_write_bit(bool bit)
{
   //Wait for clock line high (sanity check)
   while (gpio_get(PS2_CLK_GPIO)!=1)
      busy_wait_us(1);

   //Wait for clock line falling edge
   while (gpio_get(PS2_CLK_GPIO)!=0)
      busy_wait_us(1);

   //Write out a bit on falling edge
   gpio_put(PS2_DAT_GPIO, bit ? 1 : 0);
}


//Assumes clock high on entry
//HACK, can make another PIO machine for this (?)
/*
   1) Bring the Clock line low for at least 100 microseconds.
   2) Bring the Data line low.
   3) Release the Clock line.
   4) Wait for the device to bring the Clock line low.
   5) Set/reset the Data line to send the first data bit
   6) Wait for the device to bring Clock high.
   7) Wait for the device to bring Clock low.
   8) Repeat steps 5-7 for the other seven data bits and the parity bit
   9) Release the Data line.
   10) Wait for the device to bring Data low.
   11) Wait for the device to bring Clock low.
   12) Wait for the device to release Data and Clock
*/
static void kbd_write_byte(uint8_t c)
{
   //Disable PIO reading PS/2 (?)
   pio_sm_set_enabled(kbd_pio, kbd_sm, false);


   // Setup GPIOs for output
   gpio_set_dir(PS2_CLK_GPIO, GPIO_OUT);

   //Pull CLK line low (for at least 100us) to inhibit communication from device
   gpio_put(PS2_CLK_GPIO, 0);
   busy_wait_us(100);

   //Set CLK line high and DATA low (start bit)
   //Host request to send, causes kb to start generating clock pulses
   gpio_set_dir(PS2_DAT_GPIO, GPIO_OUT);
   gpio_put(PS2_DAT_GPIO, 0);             //This will be the start bit
   gpio_set_dir(PS2_CLK_GPIO, GPIO_IN);   //(Sets CLK high, PS/2 lines are pulled up)
   gpio_pull_up(PS2_CLK_GPIO);


   //Clock should be high now, and start bit is on the data line
   //(already looping through the bits, so might as well count the high bits here
   // rather than using the __builtin_popcount(byte) function)
   int ctHigh=0;
   for (int x=0; x<8; x++) {
      kbd_write_bit(c & 1);
      ctHigh += c&1;
      c >>= 1;                   //Next bit
   }

   kbd_write_bit(ctHigh^1);      //Send odd parity (1 if even number of set bits, 0 if odd)
   kbd_write_bit(1);             //Stop bit

   //Wait for clock rising edge (?)
   while (gpio_get(PS2_CLK_GPIO)!=1)
      busy_wait_us(1);

   //Set DAT back to input, to read ACK bit
   gpio_set_dir(PS2_DAT_GPIO, GPIO_IN);
   gpio_pull_up(PS2_DAT_GPIO);

   //Device acknowledge bit is special, device puts it out on the rising edge
   while (gpio_get(PS2_CLK_GPIO)!=1)      //Rising
      busy_wait_us(1);
   while (gpio_get(PS2_CLK_GPIO)!=0)      //Falling
      busy_wait_us(1);
   if (0!=gpio_get(PS2_DAT_GPIO))
      printf("Bad device ACK bit\n");
   while (gpio_get(PS2_DAT_GPIO)!=1)      //Wait for ACK pulse to end
      busy_wait_us(1);


   //Reset PS/2 PIO ?
   pio_sm_set_enabled(kbd_pio, kbd_sm, true);
   kbd_reset();
// pio_sm_restart(kbd_pio, kbd_sm);   //Don't dump the fifo (?)
}



//Writes keyboard leds
//Note: PS/2 clock should be 30-50us low, 30-50us high
//General sequence is on p5-6 of The PS/@ Mount/Keyboard Protocol doc
static void write_leds(uint8_t led_mask)
{
   uint8_t scancode;

   kbd_write_byte(SET_LEDS_CMD);

   //Wait for 0xFA scancode response ?
   //HACK, should probably do this with a state machine, so that BLE task stuff still runs)
   while (!(scancode = kbd_ready()))
      ;
   if (scancode != 0xFA)
      goto clear_err;

   //Send LED mask
   kbd_write_byte(led_mask);

   //Wait for 0xFA scancode response ?
   //HACK, should probably do this with a state machine, so that BLE task stuff still runs)
   while (!(scancode = kbd_ready()))
      ;
   if (scancode == 0xFA)
      return;


// My MSFT kb doesn't like it when you try to light all three leds (i.e. led_mask = 7)
// It sends a 0xFE/resend after the mask byte ... then if I resend the command I get that
// again and again.   When I stop resending the command, the kb no longer sends scan codes.  
// Sending it an 0xFF reset fixes this, but also clears all three LEDS.
//
// This is kind of a hack.   If I get anything other than a good 0xFA response to the mask, 
// i.e. 0xFE, etc, assume the command is not supported.    In this case the MSFT
// speedbump kb seems to stop sending scancodes, but sending an echo command to it 
// appears to fix this problem.      

#define MAX_TRIES 3
clear_err:     
   for (int x=0; x<MAX_TRIES; x++) {
      kbd_write_byte(ECHO_CMD);  
      while (!(scancode = kbd_ready()))
         ;
      printf("Clr err w/ echo, got %02X\n", scancode);
      if (scancode == 0xEE)
         break;
   }
}



void kbd_write_leds_flags(bool bCapsLed, bool bNumLed, bool bScrollLed)
{
#ifndef NO_PS2_WRITES   
   uint8_t led_mask =   (bCapsLed ? CAPS_LOCK_MASK : 0)
                      | (bNumLed ? NUM_LOCK_MASK : 0)
                      | (bScrollLed ? SCROLL_LOCK_MASK : 0);

   write_leds(led_mask);
#endif   
}
