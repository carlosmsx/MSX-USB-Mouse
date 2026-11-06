cmake_minimum_required(VERSION 3.13)

# La placa ya esta en el SDK: USB-A en GPIO12/13, flash de 2 MB.
set(PICO_BOARD waveshare_rp2350_usb_a)

# Pico-PIO-USB 0.7.3, copia local (el SDK no la trae). Tiene que definirse antes
# de pico_sdk_init: ahi TinyUSB arma la biblioteca tinyusb_pico_pio_usb.
set(PICO_PIO_USB_PATH ${CMAKE_CURRENT_LIST_DIR}/lib/Pico-PIO-USB)

include(pico_sdk_import.cmake)

project(msx_usb_mouse C CXX ASM)
set(CMAKE_C_STANDARD 11)
pico_sdk_init()

add_compile_options(-Wall -O2)

add_executable(msx_usb_mouse
    src/main.c
    src/led.c
    src/msx.c
    src/usb.c
)

pico_generate_pio_header(msx_usb_mouse ${CMAKE_CURRENT_LIST_DIR}/src/msx.pio)
pico_generate_pio_header(msx_usb_mouse ${CMAKE_CURRENT_LIST_DIR}/src/led.pio)

# src/ primero: ahi esta el tusb_config.h con device (USB-C) y host (USB-A).
target_include_directories(msx_usb_mouse PRIVATE src)

# Consola por el USB-C. El USB-A es el host por PIO.
pico_enable_stdio_usb(msx_usb_mouse 1)
pico_enable_stdio_uart(msx_usb_mouse 0)

# Al linkear tinyusb_host, pico_stdio_usb supone que la aplicacion maneja todo
# TinyUSB y apaga sus descriptores y su tusb_init(). Se vuelven a prender. La
# tarea de fondo no: tud_task() va en el lazo principal junto a tuh_task(), para
# que el device y el host no se interrumpan entre si. Por lo mismo se apaga el
# reinicio en BOOTSEL al abrir la consola a 1200 baudios, que usa grabar.py.
target_compile_definitions(msx_usb_mouse PRIVATE
    PICO_STDIO_USB_USE_DEFAULT_DESCRIPTORS=1
    PICO_STDIO_USB_ENABLE_TINYUSB_INIT=1
    PICO_STDIO_USB_ENABLE_IRQ_BACKGROUND_TASK=0
    PICO_ENABLE_USB_RESET_VIA_BAUD_RATE=1
)

target_link_libraries(msx_usb_mouse
    pico_stdlib
    pico_multicore
    hardware_pio
    tinyusb_host
    tinyusb_pico_pio_usb
)

pico_add_extra_outputs(msx_usb_mouse)
