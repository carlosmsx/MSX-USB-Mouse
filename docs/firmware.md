# Firmware 0.3

Primer esqueleto (04-nov-2026). **Compila; no esta probado en la placa ni en
un MSX.**

## Compilar y cargar

```
cd firmware
.\build.ps1            # o .\build.ps1 -Clean
```

Sale `firmware/build/msx_usb_mouse.uf2`. Para cargarlo, con la placa enchufada
por el USB-C:

```
python grabar.py       # necesita pyserial (pip install pyserial)
```

Abre la consola a 1200 baudios, la placa se reinicia sola en BOOTSEL, copia el
`.uf2` a la unidad RP2350 y espera que vuelva la consola. Lo tiene que atender
el firmware que ya esta cargado: la primera vez (o si quedo colgado) hay que
hacerlo a mano, manteniendo BOOT, apretando RESET, soltando BOOT y copiando el
`.uf2` a la unidad que aparece (o corriendo `grabar.py`, que si ya ve la unidad
copia directo).

Al linkear `tinyusb_host` el SDK apaga ese reinicio por 1200 baudios; se prende
con `PICO_ENABLE_USB_RESET_VIA_BAUD_RATE=1` en `CMakeLists.txt`.

Usa el bundle de `~/.pico-sdk` (SDK 2.3.0, GCC 14.2), igual que `MSX-SDF-Pico`.
Pico-PIO-USB **0.7.3** esta copiado en `firmware/lib/Pico-PIO-USB` (sin `.git`
ni ejemplos) porque el SDK no lo trae.

## Como esta armado

| Archivo | Que hace |
|---|---|
| `src/pines.h` | GPIO del MSX y del USB-A, reloj de 120 MHz |
| `src/main.c` | Arranque: reloj, consola, PIO del MSX, nucleo 1, host USB |
| `src/usb.c` | Host USB por PIO; mouse en modo boot; consola del USB-C |
| `src/msx.c` | Acumulado del movimiento, botones, lazo del nucleo 1 |
| `src/led.c`, `src/led.pio` | LED WS2812B (GPIO16, PIO1): escala y actividad |
| `src/msx.pio` | Sigue el pin 8 y saca los 4 nibbles |
| `grabar.py` | Graba el `.uf2` por el USB-C sin tocar los botones |
| `src/tusb_config.h` | TinyUSB: device CDC en el rhport 0, host PIO en el rhport 1 |

- **Nucleo 0**: `tud_task()` (consola) y `tuh_task()` (mouse) en el lazo
  principal. Cada reporte del mouse se escala (ver abajo), suma a dos totales
  de 32 bits y actualiza los botones (GPIO4/5 en open-drain, directo, sin
  pasar por la PIO).
- **Nucleo 1**: espera activa, sin interrupciones, en RAM. Cuando la PIO
  avisa (IRQ 0) que llego el primer flanco de una lectura, calcula lo que falta
  entregar, lo satura a -127..127, le cambia el signo (convencion MSX) y manda
  los 4 nibbles por la FIFO. Lo que pasa de 127 **se descarta**: si quedara
  pendiente (asi era la 0.1), despues de un movimiento rapido el cursor sigue
  andando con la mano quieta.
- **PIO2, SM0**: cada flanco del pin 8 saca el nibble siguiente. Si el pin 8
  queda quieto mas de `TIEMPO_REINICIO_US` (1 ms, provisional; el corte real
  cae entre 1 y 1,5 ms), la proxima lectura vuelve a empezar por X alto.
  Pico-PIO-USB usa PIO0.
- **Open-drain**: los datos se manejan con `pindirs` y la OE invertida por el
  override del GPIO; el valor de salida queda siempre en 0. Nunca se sacan
  3,3 V hacia el MSX.
- Reloj a 120 MHz: Pico-PIO-USB necesita un multiplo de 12 MHz.

## Escala del movimiento (0.2)

Un mouse USB tiene muchas mas cuentas por centimetro que uno MSX de bolita, y
en la 0.1 el cursor era muy brusco. Ahora cada cuenta USB se multiplica por una
fraccion (tabla `escalas` en `msx.c`: 1/1, 1/2, 1/3, 1/4; arranca en 1/2). El
resto de la division se guarda por eje, asi los movimientos lentos no se
pierden. El **boton del medio (la rueda)** pasa a la escala siguiente y la
consola muestra `Escala n: a/b`. No se guarda: al reiniciar vuelve a 1/2.

## LED (0.3)

El WS2812B de la placa (GPIO16) va por PIO1: PIO0 es de Pico-PIO-USB y PIO2
del MSX. El color dice la escala; mientras llegan movimientos o botones del
mouse parpadea (60 ms prendido, 60 apagado) y sigue asi 150 ms despues del
ultimo.

| LED | Estado |
|---|---|
| Blanco tenue | Sin mouse |
| Rojo | Escala 1/1 |
| Verde | Escala 1/2 (al arrancar) |
| Azul | Escala 1/3 |
| Violeta | Escala 1/4 |

Brillo bajo a proposito (tabla `colores` en `led.c`).

## Consola (USB-C)

- Al conectar un dispositivo: `USB: HID vvvv:pppp ... mouse` (o lo ignora si es
  teclado o no tiene protocolo boot).
- Mientras el MSX lee el mouse: `MSX: N lecturas/s` una vez por segundo.

## Supuestos a confirmar en el hardware

- Que cada flanco, en cualquier sentido, avanza un nibble, y el orden
  X alto, X bajo, Y alto, Y bajo.
- El signo: X positivo = izquierda, Y positivo = arriba.
- El tiempo de reinicio de la secuencia.
- Que D0-D3 van en los pines 1-4 en ese orden.
- Que la consola por USB-C y el host por PIO convivan bien en el nucleo 0
  (no hay ejemplo del SDK con `pico_stdio_usb` + host PIO; esta armado leyendo
  `stdio_usb.c`).

## Lo que falta

- Mouse que solo hablan *report protocol* (hay que leer el descriptor).
- Protocolo extendido (rueda, botones 3-5).
- Modo joystick.
