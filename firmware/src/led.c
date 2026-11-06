// LED WS2812B por PIO.
//
// Fijo con el color de la escala; mientras llegan movimientos o botones del
// mouse se apaga y prende, y sigue asi un rato despues del ultimo.

#include "led.h"
#include "msx.h"
#include "pines.h"
#include "led.pio.h"

#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "pico/time.h"

#define PIO_LED     pio1        // pio0 es de Pico-PIO-USB, pio2 del MSX
#define SM_LED      0

#define PARPADEO_MS     60      // medio periodo del parpadeo
#define ACTIVIDAD_MS    150     // cuanto sigue parpadeando despues del ultimo

// Colores por escala (R, G, B), en el orden de la tabla de msx.c. Bajos a
// proposito: a pleno el LED encandila.
static const uint8_t colores[][3] = {
    {32,  0,  0},   // 1/1 rojo
    { 0, 32,  0},   // 1/2 verde
    { 0,  0, 32},   // 1/3 azul
    {24,  0, 24},   // 1/4 violeta
};
#define COLORES     (sizeof colores / sizeof colores[0])

static const uint8_t sin_mouse[3] = {6, 6, 6};

static bool conectado;
static uint32_t ultima_actividad;
static bool hubo_actividad;
static uint32_t mostrado = 0xFFFFFFFF;  // fuerza la primera escritura

static uint32_t grb(uint8_t const c[3])
{
    return (uint32_t)c[1] << 24 | (uint32_t)c[0] << 16 | (uint32_t)c[2] << 8;
}

void led_init(void)
{
    uint offset = pio_add_program(PIO_LED, &led_ws2812_program);
    pio_sm_config c = led_ws2812_program_get_default_config(offset);
    sm_config_set_sideset_pins(&c, PIN_LED);
    sm_config_set_out_shift(&c, false, true, 24);  // a la izquierda, autopull
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);
    sm_config_set_clkdiv(&c, clock_get_hz(clk_sys) / (800000.0f * 10));

    pio_gpio_init(PIO_LED, PIN_LED);
    pio_sm_set_consecutive_pindirs(PIO_LED, SM_LED, PIN_LED, 1, true);
    pio_sm_init(PIO_LED, SM_LED, offset, &c);
    pio_sm_set_enabled(PIO_LED, SM_LED, true);
}

void led_tarea(void)
{
    uint32_t ahora = to_ms_since_boot(get_absolute_time());
    uint32_t color;

    if (!conectado)
        color = grb(sin_mouse);
    else if (hubo_actividad && ahora - ultima_actividad < ACTIVIDAD_MS
             && (ahora / PARPADEO_MS) & 1)
        color = 0;
    else
        color = grb(colores[msx_escala() % COLORES]);

    // Solo cuando cambia: el LED lo guarda, y asi la FIFO nunca se llena.
    if (color != mostrado && !pio_sm_is_tx_fifo_full(PIO_LED, SM_LED)) {
        pio_sm_put(PIO_LED, SM_LED, color);
        mostrado = color;
    }
}

void led_mouse(bool si)
{
    conectado = si;
}

void led_actividad(void)
{
    ultima_actividad = to_ms_since_boot(get_absolute_time());
    hubo_actividad = true;
}
