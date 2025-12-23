/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "btstack_run_loop.h"
#include "pico/stdlib.h"
#include "picow_bt_example_common.h"

#include "ps2kbd.h"


int btstack_main(int argc, const char * argv[]);



// KBD data and clock inputs must be consecutive with
// data in the lower position.
#define DAT_GPIO 14 // PS/2 data
//#define CLK_GPIO 15 // PS/2 clock (RSW: This is not used, kbd_init uses DAT_GPIO+1 for the clock)


int main() 
{
    stdio_init_all();

    kbd_init(1, DAT_GPIO);

    int res = picow_bt_example_init();
    if (res){
        return -1;
    }


    btstack_main(0, NULL);
    btstack_run_loop_execute();    //Does not return
}
