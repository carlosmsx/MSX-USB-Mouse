// Configuracion de TinyUSB: device en el USB-C (consola) y host en el USB-A
// (mouse, por PIO).
//
// El host NO se declara con CFG_TUSB_RHPORT1_MODE a proposito: asi tusb_init(),
// que llama pico_stdio_usb, solo arranca el device, y el host lo arranca usb.c
// con tuh_init(1) despues de configurar los pines de Pico-PIO-USB.

#ifndef TUSB_CONFIG_H_
#define TUSB_CONFIG_H_

#ifndef CFG_TUSB_MCU
#error CFG_TUSB_MCU tiene que venir del SDK
#endif

#ifndef CFG_TUSB_OS
#define CFG_TUSB_OS             OPT_OS_PICO
#endif
#ifndef CFG_TUSB_DEBUG
#define CFG_TUSB_DEBUG          0
#endif

// Device: USB nativo (rhport 0), solo CDC para la consola de pico_stdio_usb.
#define CFG_TUSB_RHPORT0_MODE   OPT_MODE_DEVICE
#define CFG_TUD_ENDPOINT0_SIZE  64
#define CFG_TUD_CDC             1
#define CFG_TUD_CDC_RX_BUFSIZE  64
#define CFG_TUD_CDC_TX_BUFSIZE  256
#define CFG_TUD_CDC_EP_BUFSIZE  64
#define CFG_TUD_VENDOR          0

// Host: Pico-PIO-USB en el rhport 1. Con hub, por si el mouse es un receptor
// inalambrico que se presenta detras de uno.
#define CFG_TUH_ENABLED             1
#define CFG_TUH_RPI_PIO_USB         1
#define CFG_TUH_ENUMERATION_BUFSIZE 256
#define CFG_TUH_HUB                 1
#define CFG_TUH_DEVICE_MAX          4
#define CFG_TUH_HID                 (3 * CFG_TUH_DEVICE_MAX)
#define CFG_TUH_HID_EPIN_BUFSIZE    64
#define CFG_TUH_HID_EPOUT_BUFSIZE   64

#endif
