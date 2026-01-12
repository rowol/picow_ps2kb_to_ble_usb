<br><br><br>

# Description

This Raspberry Pi Pico W C SDK  project reads a PS/2 keyboard stream and converts the output to
a BLE keyboard.   It uses a PIO state machine to read the PS/2 stream.

For development/testing I used:
* Raspberry Pi Pico W
* Microsoft Natural Keyboard Elite keyboard (KU-0045, "white speedbump")


# <br>Build 

Standard cmdline CMake build using the Pico C SDK.   I used [this Docker imager](https://github.com/lukstep/raspberry-pi-pico-docker-sdk)
to make a container.  Later I manually installed the current/newer version of the C SDK.


# <br>Connections

![image](td_libs_PS2Keyboard_pins.jpg)

| PS/2 &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; | PICO W Pin &nbsp; &nbsp; &nbsp; | Pico Signal |
| :----------- | :---------- | :-------|
| Data | 19 | GPIO14 |
| CLK  | 20 | GPIO15 |
| 5V   | 40 | VBUS   |
| GND  | 18 | GND, many other choices | 

<br>I ran my keyboard off the 5V VBUS output from the Pico W, which is powered by the USB port.   It could
be more robust to use an external 5V supply to run the Pico and keyboard, and possibly not all keyboards 
will work off VBUS.  Current limit is determined by the USB port, standard USB 2.0 is 500mA. 


## Notes:

* Project built with [Raspberry Pi Pico C SDK 2.2.1](https://github.com/raspberrypi/pico-sdk).  It may work with other 
versions of the SDK 

* Define MSFT_SPEEDBUMP_KB in CMakeLists.txt for the Microsoft Natural Keyboard Elite: this reverses the scancodes for NUM 
and SCROLL locks.   You may or may not need this for other PS/2 keyboards.   This also prevents all three KB leds from 
turning on at the same time as this appears to cause an error with my keyboard.  (Modes still function properly, but KB 
leds retain last setting.)




# <br>Links

This project uses modified code from several repositories, as well as original code:

* [PS/2 to USB HID Keyboard Bridge for Raspberry Pi Pico 2](https://github.com/CCappsDevelopment/pico-ps2-usb-kbd-bridge) 
  Used  key scancode to USB HID code translation.  
  (BLE GATT uses the same HID codes as USB)
  
* [BTStack HID Keyboard example](https://github.com/bluekitchen/btstack/blob/master/example/hid_keyboard_demo.c) 
  Used HID over GATT BLE code  
  (included in the [Pico W Bluetooth examples](https://github.com/raspberrypi/pico-examples) )

* [ps2kbd-lib](https://github.com/lurk101/ps2kbd-lib/tree/7409b5572734b0dd7577b63319d93c66914f2141) Used structure of 
  PIO PS/2 code, improved PIO assembler and associated C input processing, added PS/2 write code.


<br><br><br><br><br>



