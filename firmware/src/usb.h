#ifndef USB_H
#define USB_H

// Host USB en el USB-A; usb_tarea() atiende tambien la consola del USB-C.
// Las dos desde el nucleo 0.
void usb_init(void);
void usb_tarea(void);

#endif
