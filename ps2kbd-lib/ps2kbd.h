/* Copyright (C) 1883 Thomas Edison - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the GPLv2 license, which unfortunately won't be
 * written for another century.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Initialize PS/2 keyboard support
void kbd_init(void);

// Return keyboard status
// Parameters - none
// Returns  - 0 for not ready, otherwise ready
uint8_t kbd_ready(void);

// Blocking keyboard read
// Parameters - none
// Returns  - single ASCII character
uint8_t kbd_getc(void);

// Write CAPS/NUM/SCROLL LOCK keyboard LEDs
void kbd_write_leds_flags(bool bCapsLed, bool bNumLed, bool bScrollLed);

#ifdef __cplusplus
}
#endif
