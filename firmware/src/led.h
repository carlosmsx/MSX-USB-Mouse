#ifndef LED_H
#define LED_H

#include <stdbool.h>

// LED RGB de la placa: el color dice la escala del movimiento y parpadea
// mientras el mouse se usa. Sin mouse conectado queda en blanco tenue.
// Todo desde el nucleo 0.
void led_init(void);
void led_tarea(void);           // en el lazo principal

void led_mouse(bool conectado);
void led_actividad(void);       // en cada movimiento o cambio de botones

#endif
