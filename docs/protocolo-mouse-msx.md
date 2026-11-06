# Protocolo del mouse MSX

Fuentes: [MSX wiki, Mouse/Trackball](https://www.msx.org/wiki/Mouse/Trackball),
hilos de msx.org ("Use ~10€ to connect modern mouse to MSX", "MSX mouse
details"). Lo que no tiene fuente esta marcado como *a verificar*.

## Puerto de joystick (DE-9 macho en el MSX)

| Pin | Joystick | Mouse | Sentido (visto desde el MSX) |
|---|---|---|---|
| 1 | Arriba | D0 del nibble | entrada (pull-up) |
| 2 | Abajo | D1 | entrada (pull-up) |
| 3 | Izquierda | D2 | entrada (pull-up) |
| 4 | Derecha | D3 | entrada (pull-up) |
| 5 | +5 V | +5 V | alimentacion |
| 6 | Disparo A | Boton izquierdo | entrada (pull-up), activo en 0 |
| 7 | Disparo B | Boton derecho | entrada (pull-up), activo en 0 |
| 8 | Salida | **Strobe / reloj** | salida del MSX (PSG R15) |
| 9 | GND | GND | |

Los pines 1-4, 6, 7 se leen en el registro 14 del PSG; el pin 8 se maneja con el
registro 15 (bit 4 para el puerto 1, bit 5 para el puerto 2). Asignacion de D0-D3
a pines 1-4: *a verificar* contra una rutina de lectura real.

Advertencia (wiki): **no conectar un mouse Amiga**, usa el pin 5 para un boton y
cortocircuita los 5 V del MSX.

## Lectura (protocolo original)

Cada cambio de nivel del pin 8 avanza un nibble. El mouse mantiene el nibble
hasta el proximo cambio. Secuencia de una lectura (lo que hace GTPAD):

1. Flanco 1 → nibble alto de **X**
2. Flanco 2 → nibble bajo de X
3. Flanco 3 → nibble alto de **Y**
4. Flanco 4 → nibble bajo de Y

- X e Y son **desplazamientos con signo de 8 bits** desde la lectura anterior.
- Convencion de signo (wiki): **X positivo = izquierda, Y positivo = arriba**.
  Un mouse USB da X positivo a la derecha e Y positivo abajo, asi que hay que
  **invertir los dos**. *A verificar en una maquina real.*
- Si el mouse esta quieto devuelve 0. Si no hay nada conectado, los pull-ups dan
  todos unos (-1, -1) y el puntero se va abajo a la derecha: asi lo "detecta"
  el software.
- El mouse **vuelve a empezar la secuencia** (desde X alto) si el pin 8 queda
  quieto un tiempo. *A verificar el valor exacto*; los adaptadores existentes
  (codigo Arduino de NYYRIKKI) son la mejor referencia. Hay que contemplar
  tambien la fase inicial del pin 8 (si la secuencia empieza en flanco de
  bajada o de subida).

## Protocolo extendido (opcional, para mas adelante)

Despues de X e Y, dos bytes mas:

- Byte 3: nibble alto = ID (bits 7-6 = `00` para identificacion de mouse BIOS,
  bits 5-4 = `01` para protocolo extendido); nibble bajo = botones
  (bit 0 = boton 3/rueda, bit 1 = boton 4, bit 2 = boton 5).
- Byte 4: delta de la **rueda** (Z), con signo, positivo = arriba.
- Minimo entre flancos: **0,18 ms**.
- Si los bytes 3 y 4 no se leen dentro de **3 ms** desde el byte 1, en la
  siguiente lectura se vuelve a mandar el byte 1 (compatibilidad con software
  viejo que solo lee 4 nibbles).

## Trackball (para diferenciar, no lo emulamos)

El trackball usa el pin 8 para elegir eje y entrega un nibble de "angulo"; sus
contadores se reinician en 8 en vez de 0, y la BIOS usa eso para distinguirlo
del mouse.

## Que tiene que hacer el firmware

- Acumular dx, dy de los reportes HID (pueden llegar varios entre lecturas del
  MSX) y **saturar** a -128..127.
- En el flanco 1 de una secuencia nueva: congelar `X = -dx`, `Y = -dy` y
  restarlos del acumulador (lo que no entro en 8 bits queda para la proxima).
- Botones: izquierdo → pin 6 en 0, derecho → pin 7 en 0, en todo momento (no
  dependen del strobe).
- Responder rapido: el MSX lee el nibble pocos microsegundos despues de mover el
  pin 8 (la rutina de la BIOS espera muy poco entre `OUT` y `IN`). Por eso la
  idea es resolverlo con **PIO** (o IRQ de GPIO en el nucleo 1), no en el lazo
  de USB.
