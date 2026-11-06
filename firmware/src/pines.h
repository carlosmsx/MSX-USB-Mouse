// Pines del adaptador (docs/placa-rp2350-usb-a.md).
//
// Todo lo que va al MSX esta en GPIO0-6: los GPIO26-29 (ADC) no toleran 5 V y
// el MSX tiene pull-ups a 5 V en el puerto de joystick.

#ifndef PINES_H
#define PINES_H

// Puerto de joystick del MSX
#define PIN_DATOS       0   // GPIO0-3 -> pines 1-4 (D0-D3), open-drain
#define PIN_BOTON_IZQ   4   // GPIO4   -> pin 6 (disparo A), open-drain
#define PIN_BOTON_DER   5   // GPIO5   -> pin 7 (disparo B), open-drain
#define PIN_STROBE      6   // GPIO6   <- pin 8 (con 1 kohm en serie)

// LED RGB WS2812B de la placa
#define PIN_LED         16

// USB-A (host por PIO). D- es D+ + 1. Hay que sacar R13.
#define PIN_USB_DP      12

// Frecuencia del sistema: Pico-PIO-USB necesita un multiplo de 12 MHz.
#define RELOJ_KHZ       120000

#endif
