// Lado MSX: protocolo del mouse en el puerto de joystick.
//
// El nucleo 0 (USB) escala el movimiento y lo acumula en dos totales que solo
// crecen o decrecen; el nucleo 1 recuerda cuanto ya entrego y en cada lectura
// del MSX manda la diferencia, saturada a 8 bits. Lo que no entra se descarta:
// si quedara para la proxima lectura, despues de un movimiento rapido el
// cursor seguiria andando con la mano quieta. Cada total lo escribe un solo
// nucleo y es una palabra de 32 bits, asi que no hace falta cerrojo.

#include "msx.h"
#include "pines.h"
#include "msx.pio.h"

#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"

#define PIO_MSX     pio2        // pio0 es de Pico-PIO-USB
#define SM_MSX      0

// Si el pin 8 queda quieto mas que esto, la proxima lectura empieza por X alto.
// Valor provisional: falta medirlo en un MSX con un mouse original
// (docs/plan.md, paso 4). Por como esta hecha la espera en la PIO, el corte
// real cae entre 1 y 1,5 veces este valor.
#define TIEMPO_REINICIO_US  1000

// Escalas del movimiento (fraccion de cada cuenta USB que llega al MSX). Un
// mouse USB tiene muchas mas cuentas por centimetro que uno MSX de bolita.
static const struct { uint8_t num, den; } escalas[] = {
    {1, 1}, {1, 2}, {1, 3}, {1, 4},
};
#define ESCALAS         (sizeof escalas / sizeof escalas[0])
#define ESCALA_INICIAL  1       // 1/2

static volatile int32_t total_x, total_y;      // escribe el nucleo 0
static volatile uint32_t lecturas;             // escribe el nucleo 1

// Solo nucleo 0: escala elegida y lo que sobro de la division por eje.
static unsigned escala = ESCALA_INICIAL;
static int32_t resto_x, resto_y;

static void pin_open_drain(uint pin)
{
    gpio_init(pin);
    gpio_disable_pulls(pin);
    gpio_put(pin, 0);
    gpio_set_dir(pin, GPIO_IN);                 // suelto = 1 para el MSX
}

void msx_init(void)
{
    pin_open_drain(PIN_BOTON_IZQ);
    pin_open_drain(PIN_BOTON_DER);

    uint offset = pio_add_program(PIO_MSX, &msx_mouse_program);
    pio_sm_config c = msx_mouse_program_get_default_config(offset);
    sm_config_set_out_pins(&c, PIN_DATOS, 4);
    sm_config_set_in_pins(&c, PIN_STROBE);
    sm_config_set_jmp_pin(&c, PIN_STROBE);
    sm_config_set_out_shift(&c, true, false, 32);  // a la derecha: X alto primero
    sm_config_set_clkdiv(&c, 1.0f);

    for (uint i = 0; i < 4; i++) {
        uint pin = PIN_DATOS + i;
        pio_gpio_init(PIO_MSX, pin);
        gpio_disable_pulls(pin);
        // OE invertida: pindirs 1 = suelto (1), pindirs 0 = a GND (0).
        gpio_set_oeover(pin, GPIO_OVERRIDE_INVERT);
    }
    gpio_init(PIN_STROBE);
    gpio_disable_pulls(PIN_STROBE);
    gpio_set_dir(PIN_STROBE, GPIO_IN);

    pio_sm_init(PIO_MSX, SM_MSX, offset, &c);
    pio_sm_set_pins_with_mask(PIO_MSX, SM_MSX, 0, 0xFu << PIN_DATOS);
    // Arranca mostrando 0 en los cuatro pines (sin movimiento).
    pio_sm_set_pindirs_with_mask(PIO_MSX, SM_MSX, 0, 0xFu << PIN_DATOS);

    // Limite de espera en el ISR, contado con la vuelta de 2 ciclos (con el
    // pin 8 en 1 la vuelta es de 3 y el corte cae 1,5 veces mas tarde).
    uint32_t mhz = clock_get_hz(clk_sys) / 1000000;
    pio_sm_put_blocking(PIO_MSX, SM_MSX, TIEMPO_REINICIO_US * mhz / 2);
    pio_sm_exec(PIO_MSX, SM_MSX, pio_encode_pull(false, true));
    pio_sm_exec(PIO_MSX, SM_MSX, pio_encode_mov(pio_isr, pio_osr));

    pio_interrupt_clear(PIO_MSX, 0);
    pio_sm_set_enabled(PIO_MSX, SM_MSX, true);
}

// Movimiento pendiente en un eje, saturado a lo que entra en un byte con
// signo. Se deja en -127..127 para poder cambiarle el signo sin desbordar.
// El exceso se descarta (se da por entregado todo).
static inline int8_t tomar(int32_t total, int32_t *entregado)
{
    int32_t d = total - *entregado;
    if (d > 127) d = 127;
    if (d < -127) d = -127;
    *entregado = total;
    return (int8_t)d;
}

void __not_in_flash_func(msx_nucleo1)(void)
{
    int32_t entregado_x = 0, entregado_y = 0;

    for (;;) {
        // Espera activa, sin interrupciones en este nucleo: es lo que deja
        // contestar en unos cientos de ns.
        while (!pio_interrupt_get(PIO_MSX, 0))
            tight_loop_contents();
        pio_interrupt_clear(PIO_MSX, 0);

        // El MSX cuenta X positivo a la izquierda e Y positivo hacia arriba:
        // al reves que USB.
        uint8_t x = (uint8_t)(-tomar(total_x, &entregado_x));
        uint8_t y = (uint8_t)(-tomar(total_y, &entregado_y));

        uint32_t palabra = (x >> 4) | (x & 0xF) << 4
                         | (y >> 4) << 8 | (y & 0xF) << 12;
        pio_sm_put(PIO_MSX, SM_MSX, palabra);
        lecturas++;
    }
}

// Escala un eje guardando el resto, para que los movimientos lentos no se
// pierdan por redondeo. La division de C trunca hacia 0: el resto conserva el
// signo y se compensa solo al volver.
static inline int32_t escalar(int d, int32_t *resto)
{
    int32_t v = *resto + d * escalas[escala].num;
    int32_t q = v / escalas[escala].den;
    *resto = v - q * escalas[escala].den;
    return q;
}

void msx_mover(int dx, int dy)
{
    total_x += escalar(dx, &resto_x);
    total_y += escalar(dy, &resto_y);
}

unsigned msx_escala(void)
{
    return escala;
}

unsigned msx_escala_siguiente(uint8_t *num, uint8_t *den)
{
    escala = (escala + 1) % ESCALAS;
    resto_x = resto_y = 0;
    *num = escalas[escala].num;
    *den = escalas[escala].den;
    return escala;
}

void msx_botones(bool izquierdo, bool derecho)
{
    // Apretado = pin a GND.
    gpio_set_dir(PIN_BOTON_IZQ, izquierdo ? GPIO_OUT : GPIO_IN);
    gpio_set_dir(PIN_BOTON_DER, derecho ? GPIO_OUT : GPIO_IN);
}

uint32_t msx_lecturas(void)
{
    return lecturas;
}
