// Adaptador mouse USB -> mouse MSX en una Waveshare RP2350-USB-A.
//
// Nucleo 0: host USB (USB-A, por PIO), consola (USB-C) y LED.
// Nucleo 1: contesta al pin 8 del MSX, sin interrupciones.

#include <stdio.h>

#include "hardware/clocks.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"

#include "led.h"
#include "msx.h"
#include "pines.h"
#include "usb.h"

#define VERSION "0.3"

int main(void)
{
    // Antes que nada: Pico-PIO-USB necesita un multiplo de 12 MHz.
    set_sys_clock_khz(RELOJ_KHZ, true);

    stdio_init_all();
    led_init();
    msx_init();
    multicore_launch_core1(msx_nucleo1);
    usb_init();

    printf("MSX-USB-Mouse %s\n", VERSION);

    uint32_t proximo = 0, ultimas = 0;
    for (;;) {
        usb_tarea();
        led_tarea();

        // Una linea por segundo, solo si el MSX leyo algo.
        uint32_t ahora = to_ms_since_boot(get_absolute_time());
        if (ahora >= proximo) {
            proximo = ahora + 1000;
            uint32_t n = msx_lecturas();
            if (n != ultimas)
                printf("MSX: %lu lecturas/s\n", (unsigned long)(n - ultimas));
            ultimas = n;
        }
    }
}
