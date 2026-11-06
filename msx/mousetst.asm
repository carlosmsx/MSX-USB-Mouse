; MOUSETST.COM - prueba del mouse MSX desde MSX-DOS
;
; Entra en SCREEN 2, lee el mouse directo por el PSG (sirve en MSX1 y MSX2),
; mueve un cursor (sprite 0) y dibuja con los botones:
;   boton izquierdo = dibuja, boton derecho = borra
; Teclas: ESC sale, C borra, P cambia de puerto, I invierte los ejes,
;         1-9 color de la pluma.
; Abajo muestra el puerto, los bytes crudos X/Y leidos (en hex), los botones
; y la posicion. Sin mouse conectado se leen FF FF (pull-ups).
;
; Ensamblar con sjasmplus: sjasmplus mousetst.asm

	output	"MOUSETST.COM"
	org	100h

; --- BIOS (por CALSLT) y variables del sistema
RDSLT	equ	000Ch
CALSLT	equ	001Ch
CGTABL	equ	0004h
FILVRM	equ	0056h
CHGMOD	equ	005Fh
CHGCLR	equ	0062h
EXPTBL	equ	0FCC1h
GETPNT	equ	0F3FAh
PUTPNT	equ	0F3F8h
KEYBUF	equ	0FBF0h

; --- VRAM de SCREEN 2
PATTBL	equ	0000h
COLTBL	equ	2000h
SPRATR	equ	1B00h
SPRPAT	equ	3800h

MAXY	equ	175		; dibujo en las filas de caracteres 0-21
HELPROW	equ	22
STATROW	equ	23

; --- espera despues de mover el pin 8 (vueltas de DJNZ, ~3,6 us c/u)
WAIT1	equ	10
WAIT2	equ	30

start:
	ld	a,2
	ld	ix,CHGMOD
	call	bios
	ld	hl,PATTBL		; patrones en 0
	ld	bc,1800h
	xor	a
	ld	ix,FILVRM
	call	bios
	ld	hl,COLTBL		; blanco sobre negro
	ld	bc,1600h
	ld	a,0F1h
	ld	ix,FILVRM
	call	bios
	ld	hl,COLTBL+HELPROW*256	; dos filas de texto: blanco sobre azul
	ld	bc,0200h
	ld	a,0F4h
	ld	ix,FILVRM
	call	bios
	; LDIRVM no sirve: con CALSLT la pagina 0 es la ROM y leeria de ahi
	ld	hl,SPRPAT		; patron del cursor (32 bytes: sirve en 8x8 y 16x16)
	ld	de,sprite
	ld	b,32
	call	vcopy
	ld	hl,SPRATR
	ld	de,sprattr
	ld	b,5
	call	vcopy
	di
	ld	a,1			; borde negro (R#7)
	out	(99h),a
	ld	a,87h
	out	(99h),a
	ei
	call	loadfont
	ld	hl,help
	ld	de,HELPROW*256
	call	print
	ld	hl,(PUTPNT)		; vaciar el buffer de teclado
	ld	(GETPNT),hl

mainloop:
	ei
	halt
	call	moveptr
	call	drawbtn
	call	setspr
	call	status
	call	getkey
	or	a
	jr	z,mainloop
	cp	'a'
	jr	c,.upper
	sub	20h
.upper:
	cp	1Bh
	jr	z,quit
	cp	'C'
	jr	z,.clear
	cp	'P'
	jr	z,.port
	cp	'I'
	jr	z,.inv
	cp	'1'
	jr	c,mainloop
	cp	'9'+1
	jr	nc,mainloop
	sub	'1'
	ld	e,a
	ld	d,0
	ld	hl,colors
	add	hl,de
	ld	a,(hl)
	ld	(pencol),a
	jp	mainloop
.clear:
	ld	hl,PATTBL
	ld	bc,HELPROW*256
	xor	a
	ld	ix,FILVRM
	call	bios
	ld	hl,COLTBL
	ld	bc,HELPROW*256
	ld	a,0F1h
	ld	ix,FILVRM
	call	bios
	jp	mainloop
.port:
	ld	a,(port)
	xor	3			; 1 <-> 2
	ld	(port),a
	ld	de,1310h
	cp	1
	jr	z,.setp
	ld	de,6C20h
.setp:
	ld	(psgval),de
	jp	mainloop
.inv:
	ld	a,(invert)
	cpl
	ld	(invert),a
	jp	mainloop

quit:
	xor	a
	ld	ix,CHGMOD
	call	bios
	ld	ix,CHGCLR
	call	bios
	ld	hl,(PUTPNT)
	ld	(GETPNT),hl
	ld	c,0
	jp	0005h

; Llama a la rutina de BIOS en IX (slot principal)
bios:
	ld	iy,(EXPTBL-1)
	call	CALSLT
	ei
	ret

; ---------------------------------------------------------------------------
; Mouse

; Lee el mouse del puerto elegido: rawx, rawy, buttons (bit 4 = izq,
; bit 5 = der, activos en 0)
readmouse:
	ld	de,(psgval)		; D = valor de R#15, E = bit del pin 8
	ld	b,WAIT2
	call	.nib
	add	a,a
	add	a,a
	add	a,a
	add	a,a
	ld	c,a
	ld	b,WAIT1
	call	.nib
	and	0Fh
	or	c
	ld	(rawx),a
	ld	b,WAIT1
	call	.nib
	add	a,a
	add	a,a
	add	a,a
	add	a,a
	ld	c,a
	ld	b,WAIT1
	call	.nib
	ld	(buttons),a
	and	0Fh
	or	c
	ld	(rawy),a
	ret
; Escribe D en R#15 (con el LED de kana como estaba), alterna el pin 8 para
; la proxima, espera B vueltas y lee R#14
.nib:
	di
	ld	a,15
	out	(0A0h),a
	in	a,(0A2h)
	and	80h
	or	d
	out	(0A1h),a
	ld	a,d
	xor	e
	ld	d,a
.wait:
	djnz	.wait
	ld	a,14
	out	(0A0h),a
	in	a,(0A2h)
	ei
	ret

; Lee el mouse y mueve el cursor. Sin invertir: X positivo = izquierda,
; Y positivo = arriba (convencion del mouse MSX)
moveptr:
	ld	a,(curx)
	ld	(oldx),a
	ld	a,(cury)
	ld	(oldy),a
	call	readmouse
	ld	a,(rawx)
	ld	hl,curx
	call	.axis
	jr	nc,.xok
	ld	a,255
.xok:
	ld	(curx),a
	ld	a,(rawy)
	ld	hl,cury
	call	.axis
	jr	c,.ymax
	cp	MAXY+1
	jr	c,.yok
.ymax:
	ld	a,MAXY
.yok:
	ld	(cury),a
	ret
; A = desplazamiento, HL -> coordenada. Devuelve A = coordenada nueva
; (0 si quedo negativa) y carry si paso de 255
.axis:
	ld	e,a			; DE = desplazamiento con signo
	add	a,a
	sbc	a,a
	ld	d,a
	ld	a,(hl)
	ld	l,a
	ld	h,0
	ld	a,(invert)
	or	a
	jr	nz,.add
	sbc	hl,de			; carry en 0 por el OR
	jr	.clamp
.add:
	add	hl,de
.clamp:
	bit	7,h
	jr	nz,.neg
	ld	a,h
	or	a
	ld	a,l
	ret	z
	scf
	ret
.neg:
	xor	a
	ret

; Dibuja (izq) o borra (der) una linea desde la posicion anterior
drawbtn:
	ld	a,(buttons)
	cpl
	and	30h			; 1 = apretado
	ld	b,a
	ld	a,(prevbtn)
	ld	c,a
	ld	a,b
	ld	(prevbtn),a
	or	a
	ret	z
	and	c			; ya estaba apretado: linea desde la anterior
	ld	a,(oldx)
	ld	h,a
	ld	a,(oldy)
	ld	l,a
	jr	nz,.from
	ld	a,(curx)
	ld	h,a
	ld	a,(cury)
	ld	l,a
.from:
	ld	a,h
	ld	(lx0),a
	ld	a,l
	ld	(ly0),a
	ld	a,(curx)
	ld	(lx1),a
	ld	a,(cury)
	ld	(ly1),a
	ld	a,b
	and	10h			; izquierdo tiene prioridad
	ld	a,0
	jr	nz,.mode
	inc	a
.mode:
	ld	(erase),a
	jp	line

; Sprite 0 en el cursor (el sprite se ve una linea mas abajo de su Y)
setspr:
	ld	hl,SPRATR
	di
	call	vsetw
	ld	a,(cury)
	dec	a
	out	(98h),a
	ld	a,(curx)
	nop
	out	(98h),a
	ei
	ret

; ---------------------------------------------------------------------------
; Graficos

; Linea de (lx0,ly0) a (lx1,ly1), Bresenham
line:
	ld	a,(lx0)
	ld	b,a
	ld	a,(lx1)
	ld	c,1
	sub	b
	jr	nc,.dxp
	neg
	ld	c,-1
.dxp:
	ld	l,a
	ld	h,0
	ld	(ldx),hl
	ld	a,c
	ld	(lsx),a
	ld	a,(ly0)
	ld	b,a
	ld	a,(ly1)
	ld	c,1
	sub	b
	jr	nc,.dyp
	neg
	ld	c,-1
.dyp:
	ld	e,a
	ld	d,0
	ld	hl,0
	or	a
	sbc	hl,de
	ld	(ldy),hl		; ldy = -|dy|
	ld	a,c
	ld	(lsy),a
	ld	de,(ldx)
	add	hl,de
	ld	(lerr),hl
.loop:
	ld	a,(lx0)
	ld	d,a
	ld	a,(ly0)
	ld	e,a
	call	plot
	ld	a,(lx0)
	ld	b,a
	ld	a,(lx1)
	cp	b
	jr	nz,.go
	ld	a,(ly0)
	ld	b,a
	ld	a,(ly1)
	cp	b
	ret	z
.go:
	ld	hl,(lerr)
	add	hl,hl
	push	hl			; e2 = 2*err
	ld	de,(ldy)
	or	a
	sbc	hl,de
	jp	m,.skipx		; e2 < dy
	ld	hl,(lerr)
	ld	de,(ldy)
	add	hl,de
	ld	(lerr),hl
	ld	a,(lsx)
	ld	b,a
	ld	a,(lx0)
	add	a,b
	ld	(lx0),a
.skipx:
	pop	de
	ld	hl,(ldx)
	or	a
	sbc	hl,de
	jp	m,.loop			; e2 > dx
	ld	hl,(lerr)
	ld	de,(ldx)
	add	hl,de
	ld	(lerr),hl
	ld	a,(lsy)
	ld	b,a
	ld	a,(ly0)
	add	a,b
	ld	(ly0),a
	jr	.loop

; Pixel en D = x, E = y. erase = 0 lo prende con pencol, si no lo apaga
plot:
	ld	a,e
	cp	MAXY+1
	ret	nc
	and	0F8h			; HL = (y/8)*256 + (x and F8h) + (y and 7)
	rrca
	rrca
	rrca
	ld	h,a
	ld	a,d
	and	0F8h
	ld	l,a
	ld	a,e
	and	7
	or	l
	ld	l,a
	ld	a,d
	and	7
	ld	b,a
	ld	a,80h
	jr	z,.mask
.shift:
	rrca
	djnz	.shift
.mask:
	ld	c,a
	di
	call	vread
	ld	b,a
	ld	a,(erase)
	or	a
	jr	nz,.off
	ld	a,c
	or	b
	call	vwrite
	set	5,h			; + 2000h: tabla de colores
	ld	a,(pencol)
	call	vwrite
	ei
	ret
.off:
	ld	a,c
	cpl
	and	b
	call	vwrite
	ei
	ret

; VRAM (HL) -> A. Con interrupciones cortadas
vread:
	ld	a,l
	out	(99h),a
	ld	a,h
	and	3Fh
	out	(99h),a
	ex	(sp),hl			; tiempo para que el VDP traiga el dato
	ex	(sp),hl
	in	a,(98h)
	ret

; A -> VRAM (HL). Con interrupciones cortadas
vwrite:
	push	af
	call	vsetw
	pop	af
	out	(98h),a
	ret

; Direccion de escritura HL
vsetw:
	ld	a,l
	out	(99h),a
	ld	a,h
	and	3Fh
	or	40h
	out	(99h),a
	ret

; Copia B bytes de RAM (DE) a VRAM (HL)
vcopy:
	di
	call	vsetw
.byte:
	ld	a,(de)
	out	(98h),a
	inc	de
	djnz	.byte
	ei
	ret

; Copia los caracteres 32-95 de la ROM (CGTABL) a font
loadfont:
	ld	a,(EXPTBL)
	ld	hl,CGTABL
	call	RDSLT
	push	af
	ld	a,(EXPTBL)
	ld	hl,CGTABL+1
	call	RDSLT
	ld	h,a
	pop	af
	ld	l,a
	ld	de,32*8
	add	hl,de
	ld	de,font
	ld	bc,64*8
.copy:
	push	bc
	push	de
	push	hl
	ld	a,(EXPTBL)
	call	RDSLT
	pop	hl
	pop	de
	pop	bc
	ld	(de),a
	inc	hl
	inc	de
	dec	bc
	ld	a,b
	or	c
	jr	nz,.copy
	ei
	ret

; Texto en HL (termina en 0) desde la direccion de patrones DE
print:
	ld	a,(hl)
	or	a
	ret	z
	inc	hl
	push	hl
	push	de
	sub	32
	cp	64
	jr	c,.ok
	xor	a
.ok:
	ld	l,a
	ld	h,0
	add	hl,hl
	add	hl,hl
	add	hl,hl
	ld	bc,font
	add	hl,bc
	ex	de,hl
	ld	b,8
	call	vcopy
	pop	de
	ld	hl,8
	add	hl,de
	ex	de,hl
	pop	hl
	jr	print

; Linea de estado: "P1 X=FB Y=03 LR INV 128,091"
status:
	ld	hl,stat+1
	ld	a,(port)
	add	a,'0'
	ld	(hl),a
	ld	hl,stat+5
	ld	a,(rawx)
	call	hex
	ld	hl,stat+10
	ld	a,(rawy)
	call	hex
	ld	a,(buttons)
	ld	bc,'L'*256+'-'
	bit	4,a
	jr	nz,.l
	ld	c,b
.l:
	ld	hl,stat+13
	ld	(hl),c
	ld	bc,'R'*256+'-'
	bit	5,a
	jr	nz,.r
	ld	c,b
.r:
	inc	hl
	ld	(hl),c
	ld	hl,stat+16
	ld	de,txtinv
	ld	a,(invert)
	or	a
	jr	nz,.i
	ld	de,txtnor
.i:
	ex	de,hl
	ld	bc,3
	ldir
	ld	hl,stat+20
	ld	a,(curx)
	call	dec3
	ld	hl,stat+24
	ld	a,(cury)
	call	dec3
	ld	hl,stat
	ld	de,STATROW*256
	jp	print

; A en hex en (HL), (HL+1)
hex:
	push	af
	rrca
	rrca
	rrca
	rrca
	call	.dig
	pop	af
.dig:
	and	0Fh
	add	a,'0'
	cp	'9'+1
	jr	c,.st
	add	a,7
.st:
	ld	(hl),a
	inc	hl
	ret

; A en decimal de 3 cifras en (HL)
dec3:
	ld	b,'0'-1
.c:
	inc	b
	sub	100
	jr	nc,.c
	add	a,100
	ld	(hl),b
	inc	hl
	ld	b,'0'-1
.d:
	inc	b
	sub	10
	jr	nc,.d
	add	a,10+'0'
	ld	(hl),b
	inc	hl
	ld	(hl),a
	ret

; Tecla del buffer de la BIOS en A, 0 si no hay
getkey:
	di
	ld	hl,(GETPNT)
	ld	de,(PUTPNT)
	or	a
	sbc	hl,de
	jr	z,.none
	add	hl,de
	ld	a,(hl)
	inc	hl
	ld	de,KEYBUF+40
	ex	de,hl
	or	a
	sbc	hl,de
	ex	de,hl
	jr	nz,.st
	ld	hl,KEYBUF
.st:
	ld	(GETPNT),hl
	ei
	ret
.none:
	xor	a
	ei
	ret

; ---------------------------------------------------------------------------
; Datos

sprite:
	db	10000000b
	db	11000000b
	db	11100000b
	db	11110000b
	db	11111000b
	db	11100000b
	db	10110000b
	db	00011000b
	ds	24,0

sprattr:
	db	95,128,0,9		; Y, X, patron, color (rojo claro)
	db	208			; fin de la lista de sprites

; colores de pluma para 1-9 (nibble alto = color, bajo = fondo negro)
colors:
	db	0F1h,081h,021h,071h,0B1h,0D1h,051h,091h,031h

help:	db	"ESC C:BORRA P:PUERTO I:INV 1-9",0
stat:	db	"P1 X=00 Y=00 -- NOR 000,000",0
txtinv:	db	"INV"
txtnor:	db	"NOR"

port:	db	1
psgval:	dw	1310h			; puerto 1: R#15 = 00010011b, pin 8 = bit 4
				; puerto 2: R#15 = 01101100b, pin 8 = bit 5
invert:	db	0
pencol:	db	0F1h
curx:	db	128
cury:	db	96
oldx:	db	0
oldy:	db	0
rawx:	db	0
rawy:	db	0
buttons: db	0FFh
prevbtn: db	0
erase:	db	0
lx0:	db	0
ly0:	db	0
lx1:	db	0
ly1:	db	0
lsx:	db	0
lsy:	db	0
ldx:	dw	0
ldy:	dw	0
lerr:	dw	0

font:	ds	64*8
