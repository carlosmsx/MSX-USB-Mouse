# Plan de investigacion

## Pasos

1. **Placa sola.** Compilar y correr la demo `Host_hid_to_device_cdc` de
   Waveshare y ver los reportes del mouse por el CDC del USB-C. Probar con
   varios mouse (incluido alguno low-speed) y ver si hace falta sacar R13.
2. ~~**Esquematico.**~~ Hecho el 04-nov-2026: tira P1, R13 = pull-up de D+
   (sacarla), VSYS = VBUS del USB-C y del USB-A sin proteccion. Ver
   [placa-rp2350-usb-a.md](placa-rp2350-usb-a.md).
3. ~~**Elegir pines**~~ Propuesta: GPIO0-3 = pines 1-4, GPIO4/5 = botones,
   GPIO6 = pin 8 (en [placa-rp2350-usb-a.md](placa-rp2350-usb-a.md)). Falta
   confirmar la tolerancia a 5 V en el datasheet del RP2350.
4. **Medir el MSX.** Con el analizador (o el OWON) capturar el pin 8 durante
   GTPAD y con un mouse MSX real si hay uno: tiempo entre `OUT` y `IN`, fase
   inicial del pin 8, cada cuanto lee la BIOS y despues de cuanto tiempo de
   reposo el mouse reinicia la secuencia.
5. **Firmware v0.1**: escrito y compilado el 04-nov-2026
   ([firmware.md](firmware.md)); falta probarlo. Primero en la placa sola (que
   aparezca el mouse en la consola), despues en el MSX con un programa BASIC
   que llame a `PAD(12)`, `PAD(13)`, `PAD(14)`.
6. **Software real**: MSX-DOS 2 con un programa que use mouse, Graph Saurus,
   juegos con soporte de mouse (Arkanoid no: usa el paddle).
7. Opcional: protocolo extendido (rueda, botones 3-5).

## Preguntas abiertas

- Alimentar todo desde el pin 5 del joystick: cuanta corriente da el MSX por ese
  pin y cuanto consume la placa + un mouse USB (los opticos modernos ~20-100 mA).
  Si no alcanza, alimentar por el USB-C. VSYS esta unido directo al VBUS del
  USB-C: hace falta un diodo entre el pin 5 y VSYS para poder programar con el
  MSX conectado.
- Si se alimenta por USB-C y el MSX esta apagado, que no se alimente el MSX por
  los GPIO (open-drain lo evita en las salidas; revisar el pin 8).
- Tolerancia a 5 V real del RP2350 en el pin 8 (datasheet, seccion GPIO).
- Que hacer con mouse con reporte no-boot (report protocol): TinyUSB pide boot
  protocol por defecto; si algun mouse no lo soporta, parsear el descriptor.
- Si conviene ademas un modo **joystick** (mouse → direcciones) para juegos sin
  soporte de mouse.

## Referencias

- Waveshare wiki: https://www.waveshare.com/wiki/RP2350-USB-A
- CNX Software: https://www.cnx-software.com/2025/02/03/rp2350-usb-a-raspberry-pi-rp2350-board-usb-type-a-port-pio-implementation/
- Pico-PIO-USB: https://github.com/sekigon-gonnoc/Pico-PIO-USB
- HID Remapper: https://github.com/jfedor2/hid-remapper
- MSX wiki, mouse: https://www.msx.org/wiki/Mouse/Trackball
- msx.org, "Use ~10€ to connect modern mouse to MSX": https://msx.org/node/43569
- msx.org, "MSX mouse details": https://msx.org/node/66787
