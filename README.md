# MSX-USB-Mouse

Adaptador para usar un mouse USB en el puerto de joystick de un MSX, con una
**Waveshare RP2350-USB-A** (RP2350A).

Estado: **firmware 0.3**. La version 0.1 se probo en un MSX con el programa de
prueba y funciona; la escala del movimiento y el LED (0.2 y 0.3) estan grabados
en la placa y falta probarlos a fondo ([docs/firmware.md](docs/firmware.md)).

## Uso

- **Boton del medio (rueda):** cambia la escala del movimiento (1/1, 1/2, 1/3,
  1/4; arranca en 1/2). Un mouse USB tiene mucha mas resolucion que uno MSX, y
  con la escala se recupera la precision.
- **LED de la placa:** el color indica la escala (rojo 1/1, verde 1/2, azul
  1/3, violeta 1/4) y parpadea mientras el mouse se usa. En blanco tenue, no
  hay mouse conectado.

## Idea

```
mouse USB ──> USB-A (PIO-USB host, GP12/GP13) ──> RP2350 ──> puerto joystick MSX (DE-9)
                                                     │        GPIO0-5 → pines 1-4, 6, 7 (open-drain)
                                                     └──────  GPIO6  ← pin 8 (strobe)

Antes de usarla como host hay que sacar R13 (pull-up de 1,5 k en D+).
```

1. El RP2350 hace de **host USB** en el conector USB-A usando
   [Pico-PIO-USB](https://github.com/sekigon-gonnoc/Pico-PIO-USB) + TinyUSB
   (es lo que trae la demo de Waveshare).
2. Acumula los desplazamientos de los reportes HID del mouse (boot protocol).
3. Del lado MSX emula un **mouse MSX** (protocolo de nibbles con pin 8 como
   reloj): una maquina PIO o una interrupcion de GPIO sigue los flancos del
   pin 8 y pone el nibble que corresponde en los pines 1-4.

## Documentos

- [docs/placa-rp2350-usb-a.md](docs/placa-rp2350-usb-a.md) — lo que se sabe de la placa y lo que falta verificar
- [docs/protocolo-mouse-msx.md](docs/protocolo-mouse-msx.md) — puerto de joystick y protocolo del mouse MSX
- [docs/plan.md](docs/plan.md) — pasos de la investigacion y preguntas abiertas
- [docs/firmware.md](docs/firmware.md) — como compilar y como esta armado el firmware

## Compilar y grabar

```
cd firmware
.\build.ps1
python grabar.py
```

pico-sdk 2.3.0 en C (mismo esquema que `MSX-SDF-Pico`), TinyUSB host sobre
Pico-PIO-USB 0.7.3 (copia en `firmware/lib`). `grabar.py` (requiere pyserial)
reinicia la placa en BOOTSEL por el USB-C y copia el `.uf2`, sin tocar los
botones. La primera vez, con un firmware anterior a la 0.2, se graba a mano:
BOOT apretado, RESET, y copiar el `.uf2` a la unidad RP2350.

## Programa de prueba para el MSX

[msx/mousetst.asm](msx/mousetst.asm) → `MOUSETST.COM`, para correr desde MSX-DOS
(MSX1 o MSX2). Entra en SCREEN 2, lee el mouse directo por el PSG, mueve un
cursor y dibuja (boton izquierdo) o borra (derecho). Abajo muestra el puerto,
los bytes crudos X/Y en hex (FF FF = no hay mouse), los botones y la posicion.
Teclas: ESC sale, C borra, P cambia de puerto, I invierte los ejes, 1-9 color.

```
cd msx
C:\workspace\bin\sjasmplus\sjasmplus.exe mousetst.asm
```
