// Lado USB: host por PIO en el USB-A, mouse en modo boot.
//
// En modo boot el reporte del mouse es fijo (botones, X, Y, rueda), sin
// descriptor que interpretar. Los mouse que no lo soportan quedan para despues
// (docs/plan.md).

#include <stdio.h>

#include "pines.h"
#include "led.h"
#include "msx.h"
#include "usb.h"

#include "pio_usb.h"
#include "tusb.h"

void usb_init(void)
{
    pio_usb_configuration_t cfg = PIO_USB_DEFAULT_CONFIG;
    cfg.pin_dp = PIN_USB_DP;
    tuh_configure(1, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &cfg);
    tuh_init(1);
}

void usb_tarea(void)
{
    tud_task();     // consola (USB-C)
    tuh_task();     // mouse (USB-A)
}

void tuh_hid_mount_cb(uint8_t dev, uint8_t inst, uint8_t const *desc, uint16_t largo)
{
    (void)desc;
    (void)largo;
    uint16_t vid, pid;
    tuh_vid_pid_get(dev, &vid, &pid);
    uint8_t proto = tuh_hid_interface_protocol(dev, inst);
    printf("USB: HID %04x:%04x dir %u inst %u: %s\n", vid, pid, dev, inst,
           proto == HID_ITF_PROTOCOL_MOUSE ? "mouse" :
           proto == HID_ITF_PROTOCOL_KEYBOARD ? "teclado (ignorado)" :
           "sin protocolo boot (ignorado)");

    if (proto == HID_ITF_PROTOCOL_MOUSE) {
        led_mouse(true);
        if (!tuh_hid_receive_report(dev, inst))
            printf("USB: no se pudo pedir el reporte\n");
    }
}

void tuh_hid_umount_cb(uint8_t dev, uint8_t inst)
{
    printf("USB: desconectado dir %u inst %u\n", dev, inst);
    msx_botones(false, false);
    // TinyUSB llama antes de borrar la interfaz: el protocolo todavia esta.
    if (tuh_hid_interface_protocol(dev, inst) == HID_ITF_PROTOCOL_MOUSE)
        led_mouse(false);
}

void tuh_hid_report_received_cb(uint8_t dev, uint8_t inst, uint8_t const *rep, uint16_t largo)
{
    static bool medio_antes;
    static uint8_t botones_antes;

    if (largo >= 3) {
        hid_mouse_report_t const *m = (hid_mouse_report_t const *)rep;

        // Boton del medio (la rueda): cambia la escala al apretarlo. El MSX
        // no lo ve.
        bool medio = m->buttons & MOUSE_BUTTON_MIDDLE;
        if (medio && !medio_antes) {
            uint8_t num, den;
            unsigned i = msx_escala_siguiente(&num, &den);
            printf("Escala %u: %u/%u\n", i + 1, num, den);
        }
        medio_antes = medio;

        if (m->x || m->y || m->buttons != botones_antes)
            led_actividad();
        botones_antes = m->buttons;

        msx_mover(m->x, m->y);
        msx_botones(m->buttons & MOUSE_BUTTON_LEFT, m->buttons & MOUSE_BUTTON_RIGHT);
    }
    tuh_hid_receive_report(dev, inst);
}
