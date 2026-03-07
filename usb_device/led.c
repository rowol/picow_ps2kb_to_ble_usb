//
// LED.C
// LED module
// For PicoW, be sure to call cyw43_arch_init() during hw initialization,
// because LED is on the BLE/Wifi chip, not a GPIO
//
// Limitations:
// * LED pin is hardcoded to only LED pin on Pioc W.  For multiple LEDs,
//   make this an object instead and pass in pin, etc...
//


#include "pico/cyw43_arch.h"

#include "led.h"


void ledOn(void)
{
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
}

void ledOff(void)
{
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
}

void ledToggle(void)
{
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN,
                        cyw43_arch_gpio_get(CYW43_WL_GPIO_LED_PIN) ? 0 : 1);  //Invert value
}

