# Waveshare RP2350-USB-A

Datos de la wiki de Waveshare, de CNX Software y del esquematico oficial
([RP2350-USB-A.pdf](https://files.waveshare.com/wiki/RP2350-USB-A/RP2350-USB-A.pdf),
leido el 04-nov-2026). La publicacion de MercadoLibre no se pudo leer (403).

## Resumen

| Item | Dato |
|---|---|
| Chip | RP2350A (QFN-60), revision A4, hasta 150 MHz |
| Memoria | 520 KB SRAM, flash externa W25Q16JV (**2 MB**) |
| Cristal | 12 MHz |
| Regulador | RT9013-33 (LDO de 500 mA), entrada VSYS |
| USB-C | USB nativo del RP2350 (27 Ω en serie), CC1/CC2 con 5,1 kΩ a GND |
| USB-A | **D+ = GPIO12, D- = GPIO13** (27 Ω en serie), VBUS = VSYS |
| LED | WS2812B en **GPIO16**, alimentado de 3V3 |
| Botones | Key1 = BOOT (QSPI_SS), Key2 = RESET (RUN) |
| Tamano | 33 x 17,5 mm |

## Tira P1 (18 pines)

| Pin | Senal | | Pin | Senal |
|---|---|---|---|---|
| 1 | GPIO0 | | 10 | GPIO9 |
| 2 | GPIO1 | | 11 | GPIO10 |
| 3 | GPIO2 | | 12 | GPIO26 / ADC0 |
| 4 | GPIO3 | | 13 | GPIO27 / ADC1 |
| 5 | GPIO4 | | 14 | GPIO28 / ADC2 |
| 6 | GPIO5 | | 15 | GPIO29 / ADC3 |
| 7 | GPIO6 | | 16 | 3V3 |
| 8 | GPIO7 | | 17 | GND |
| 9 | GPIO8 | | 18 | **VSYS** |

Los 15 GPIO son **GPIO0-10** (consecutivos) y **GPIO26-29** (con ADC). GPIO11,
14, 15 y 17-25 no salen.

## El puerto USB-A (J1)

```
3V3 ──[R10 NC/1.5k]──┬── GPIO13 ──[R11 27Ω]── DATA-
                     
3V3 ──[R13 1.5k]─────┬── GPIO12 ──[R12 27Ω]── DATA+
VSYS ─────────────────────────────────────── VBUS
```

- **R13 es un pull-up de 1,5 kΩ en D+**: hace que el puerto se presente como
  *device full-speed*. Para usarlo como **host hay que sacarla**: con R13 puesta
  el host siempre ve una linea D+ en alto, no detecta conexion/desconexion
  (hot-swap) y un mouse *low-speed* (que pone su pull-up en D-) queda en
  conflicto. Es lo que dice la FAQ de la wiki, y el esquematico lo explica.
- R10 (en D-, sin montar) es la opcion para device low-speed. Dejarla sin montar.
- Los pull-down de 15 kΩ que necesita un host no estan en la placa: Pico-PIO-USB
  usa los pull-down internos del RP2350. La revision A4 ya no tiene el
  problema E9 de las pull-down internas.
- D- = D+ + 1, que es lo que pide Pico-PIO-USB (`pin_dp = 12`).

## Alimentacion: VSYS es todo un mismo nodo

**VBUS del USB-C = VSYS = pin 18 de la tira = VBUS del USB-A**, sin diodo, sin
fusible y sin switch. Consecuencias:

- El mouse toma los 5 V de lo que alimente la placa.
- Si se alimenta desde el **pin 5 del joystick del MSX** a VSYS (pin 18) y
  ademas se enchufa el USB-C a una PC, **se unen los 5 V del MSX y los de la PC**.
  Para desarrollo hace falta un diodo Schottky (o un ideal diode) entre el MSX y
  VSYS, o directamente no conectar el pin 5 mientras se programa.
- El regulador de 3,3 V es un LDO de 500 mA desde VSYS: si el MSX da algo menos
  de 5 V, igual sobra margen.
- Consumo a medir: RP2350 a 150 MHz con PIO-USB (~25-40 mA) + mouse
  (~20-100 mA). El limite real es lo que entregue el pin 5 del MSX.

## Pines para el MSX

Usar **GPIO0-10** y dejar fuera GPIO26-29: los pines con ADC del RP2350 **no**
son tolerantes a 5 V, y los pull-ups del MSX estan a 5 V.

Propuesta (todos en la tira, en orden):

| GPIO | Pin P1 | DE-9 MSX | Uso |
|---|---|---|---|
| GPIO0 | 1 | 1 | D0 (open-drain) |
| GPIO1 | 2 | 2 | D1 (open-drain) |
| GPIO2 | 3 | 3 | D2 (open-drain) |
| GPIO3 | 4 | 4 | D3 (open-drain) |
| GPIO4 | 5 | 6 | Boton izquierdo (open-drain) |
| GPIO5 | 6 | 7 | Boton derecho (open-drain) |
| GPIO6 | 7 | 8 | Strobe del MSX (entrada, sin pull) |
| — | 17 | 9 | GND |
| — | 18 | 5 | +5 V (con diodo, ver arriba) |

- GPIO0-3 consecutivos: la PIO saca el nibble entero con `out pindirs, 4`
  (open-drain = valor fijo en 0 y se maneja la direccion: 1 → salida en 0,
  0 → entrada y el pull-up del MSX da el 1). Hay que invertir el nibble antes.
- GPIO6 queda pegado para que la misma SM lo use con `wait pin` / `jmp pin`.
- Quedan libres GPIO7-10 (por ejemplo para un segundo puerto o para elegir modo
  mouse/joystick con un jumper).
- En el pin 8 conviene una resistencia en serie (1 kΩ) por las dudas: la
  tolerancia a 5 V del RP2350 vale solo con IOVDD alimentado, y en desarrollo la
  placa puede estar apagada con el MSX prendido.
- Desactivar los pull internos en GPIO0-6.
