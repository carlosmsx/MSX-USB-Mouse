#ifndef MSX_H
#define MSX_H

#include <stdbool.h>
#include <stdint.h>

// Configura la PIO y los pines del puerto de joystick. Nucleo 0, antes de
// arrancar el nucleo 1.
void msx_init(void);

// Lazo del nucleo 1: contesta al pin 8. No vuelve.
void msx_nucleo1(void);

// Desde el nucleo 0 (reportes del mouse USB). Convencion USB: X positivo a la
// derecha, Y positivo hacia abajo.
void msx_mover(int dx, int dy);
void msx_botones(bool izquierdo, bool derecho);

// Pasa a la escala de movimiento siguiente (vuelve a la primera despues de la
// ultima). Devuelve su indice y en num/den la fraccion. Nucleo 0.
unsigned msx_escala_siguiente(uint8_t *num, uint8_t *den);
unsigned msx_escala(void);      // indice de la escala actual

// Cantidad de lecturas del MSX desde el arranque (para la consola).
uint32_t msx_lecturas(void);

#endif
