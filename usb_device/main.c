/*
 * PS/2 to USB HID Keyboard Bridge
 * 
 * Converts PS/2 keyboard input to USB HID keyboard output.
 * Designed for BMC64 compatibility using Boot Keyboard protocol.
 * 
 * Based on TinyUSB HID example by Ha Thach (tinyusb.org)
 */

#include <stdio.h>
#include "pico/stdlib.h"

#include "bsp/board_api.h"
#include "tusb.h"

#include "usb_descriptors.h"

#include "ps2kbd.h"
#include "ps2.h"


//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void) 
{ 
   printf("tud_mount\n"); 
}

// Invoked when device is unmounted
void tud_umount_cb(void) 
{ 
   printf("tud_umount\n"); 
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool /*remote_wakeup_en*/) 
{
   printf("tud_suspend\n");
}

// Invoked when usb bus is resumed
void tud_resume_cb(void) 
{ 
   printf("tud_suspend\n"); 
}



//--------------------------------------------------------------------+
// USB HID
//--------------------------------------------------------------------+

// Send keyboard HID report with current PS/2 state
static void send_hid_report(void) 
{
   // skip if hid is not ready yet
   if (!tud_hid_ready()) 
      return;

   // Send keyboard report using Boot Keyboard format (no report ID)
   // The tud_hid_keyboard_report function handles this correctly when
   // the descriptor doesn't include a report ID
   tud_hid_keyboard_report(0, ps2_get_modifiers(), (uint8_t*)ps2_get_keys());
}



// Send HID report periodically or when state changes
void hid_task(void) 
{
   // Poll every 10ms to send reports
   const uint32_t interval_ms = 10;
   static uint32_t start_ms = 0;

   if (board_millis() - start_ms < interval_ms) return;  // not enough time
   start_ms += interval_ms;

   // If suspended, don't send reports
   if (tud_suspended()) {
      // Could implement remote wakeup here if needed
      return;
   }

   // Send report if state changed, or periodically for reliability
   if (ps2_state_changed()) {
      send_hid_report();
      ps2_clear_changed();
   }
}



// Invoked when sent REPORT successfully to host
// For a single keyboard device, we don't need to chain reports
void tud_hid_report_complete_cb(uint8_t /*instance*/, uint8_t const* /*report*/, uint16_t /*len*/) 
{
   // Nothing to do - we only have one report type
}



// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t /*instance*/, uint8_t /*report_id*/,
                               hid_report_type_t report_type, uint8_t* buffer,
                               uint16_t reqlen) 
{
   // For Boot Keyboard, return current keyboard state
   if (report_type == HID_REPORT_TYPE_INPUT) {
      if (reqlen >= 8) {
         buffer[0] = ps2_get_modifiers();
         buffer[1] = 0;  // Reserved
         memcpy(&buffer[2], ps2_get_keys(), 6);
         return 8;
      }
   }

   return 0;
}



// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t /*instance*/, uint8_t /*report_id*/, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) 
{
   if (report_type == HID_REPORT_TYPE_OUTPUT) {
      // Set keyboard LED e.g Capslock, Numlock etc...
      // For Boot Keyboard without report ID, buffer[0] is the LED state
      if (bufsize < 1) return;

      uint8_t const kbd_leds = buffer[0];

      // RSW - Could set keyboard LEDs here, although BLE doesn't reliably send reports for doing this, so I decoded it off the keypresses
      // in the keyboard library: might was well just keep doing it there.
      if (kbd_leds & KEYBOARD_LED_CAPSLOCK)
         printf("CAPS on\n");
      else
         printf("CAPS off\n");
   }
}



// KBD data and clock inputs must be consecutive with
// data in the lower position.
#define DAT_GPIO 14  // PS/2 data
// #define CLK_GPIO 15 // PS/2 clock (RSW: This is not used, kbd_init uses DAT_GPIO+1 for the clock)

int main(void) 
{
   stdio_init_all();
   kbd_init(1, DAT_GPIO);

   tud_init(BOARD_TUD_RHPORT);  // init device stack on configured roothub port

   for (;;) {
      tud_task();  // tinyusb device task
      ps2_task();  // Poll PS/2 keyboard for incoming scancodes
      hid_task();  // Send HID reports when needed
   }
}





