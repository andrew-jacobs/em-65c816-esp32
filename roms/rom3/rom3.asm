;===============================================================================
; __     _______ _     ___  _  __
; \ \   / /_   _| |   ( _ )/ |/ /_
;  \ \ / /  | | | |   / _ \| | '_ \
;   \ V /   | | | |__| (_) | | (_) |
;    \_/    |_| |_____\___/|_|\___/
;
; VTL-2 for the WDC 65C816
;-------------------------------------------------------------------------------
;
; Based on the original VTL-2 programs by Frank McCoy and Gary Shannon (1977)
; and the adaption to the 6502 by Michael T. Barry (2012)

		.65816

		.include "../w65c816.inc"
		.include "../signature.inc"
		
BS		.equ	$08
LF		.equ	$0a
CR		.equ	$0d
DEL		.equ	$7f

;===============================================================================

		.page0
		.org	$80
AT:		.space	2			; {@} Interpreter text pointer
		.space	2 * 30			; User variables
UNDER:		.space	2			; {_} Interpreter temp storage
		.space	2
BANG:		.space	2			; {!} Return line number
QUOTE:		.space	2			; {"} User ML subroutine vector
POUND:		.space	2
DOLLAR:		.space	2
REMAINDER:	.space	2
AMPERSAND:	.space	2
TICK:		.space	2
LPAREN:		.space	2
RPAREN:		.space	2
STAR:		.space	2
		.space	2 * 5
ARG:		.space	2 * 11
		.space 	2 * 3
GTHAN:		.space	2
QUES:		.space	2

		.org	$0100
		.space	255
NULLSTACK:	.space	1


		.bss
		.org	$020000
PRGM:		.space	63 * 1024
HIMEM:

LINEBUFFER:	.space	256

;===============================================================================
;-------------------------------------------------------------------------------

		.code
		.org	$070000
		
		.longa	?
		.longi	?
VTL2:
		sei			;
		native
		short_a
		long_i

		ldx	#PRGM			; {&} -> empty program
		stx	AMPERSAND
		ldx	#HIMEM			; {*} -> tio of user RAM
		stx	STAR

.StartOk:	sec

.Start:		ldx	#NULLSTACK		; Reset system stack pointer
		txs
		if cs
		 lda	#'O'			; Print prompt
		 jsr	.UartTx
		 lda	#'K'
		 jsr	.UartTx
		endif
		jsr	.NewLn
		ldx	#POUND

.ELoop:


.Stmnt:

.Listing:

.FindLn:


.PrMsg:


.OutCR:
		php
		short_a
		lda	#CR
		jsr	.UartTx
		plp
		rts


		.longa	on
		.longi	on
.Oper:
		cmp	#'+'			; Addition operator?
		if eq
.Add:		 clc
		 lda	<0,x			; Var[x] += Var[x+2]
		 adc	<2,x
		 sta	<0,x
		 rts
		endif
		cmp	#'-'			; Subtraction operator?
		if eq
.Sub:		 sec
		 lda	<0,x			; Var[x] -= Var[x+2]
		 sbc	<2,x
		 sta	<0,x
		 rts
		endif

		cmp	#'*'
		if eq
.Mul:		 lda	<0,x			; {_} = Var[x]
		 sta	UNDER
		 stz	<0,x			; Var[x] = 0
		 repeat
		  lsr	UNDER			; {_} /= 2
		  if cs
		   jsr	.Add
		  endif
		  asl	<2,x			; Var[x+2] <<= 1
		 until eq			; Until zero
		 rts
		endif

		cmp	#'/'
		if eq
.Div:		 stz	REMAINDER		; {%} = 0
		 lda	#16
		 sta	UNDER
		 repeat
		  asl	<0,x
		  rol	REMAINDER
		  lda	REMAINDER
		  cmp	<2,x
		  if cs
		   sbc	<2,x
		   sta	REMAINDER
		   inc	<0,x
		  endif
		  dec	UNDER
		 until eq
		 rts
		endif

		pha
		jsr	.Sub
		pla
		sec
		sbc	#'='
		if eq
		 lda	<0,x
		 if ne
		  clc
		 endif
		else
		 eor	<0,x
		 eor	#$8000
		 asl	a
		endif
		lda	#0
		rol	a
		sta	<0,x
		rts
		

		.longa	?
		.longi	on
.NewLn:
		jsr	.OutCR
		ldy	#LINEBUFFER
		sty	AT
		short_ai
		ldy	#0
		repeat
		 jsr	.UartRx
		 cmp	#DEL
		 if eq
		  lda	#BS
		 endif
		 cmp 	#BS
		 if eq
		  cpy	#1
		 endif
		 
		forever



;===============================================================================
; I/O Routines for EM-65C816-ESP32
;-------------------------------------------------------------------------------

; Transmit the character in A using the UART. Poll the UART to see if its
; busy before outputing the character.

		.longa	?
.UartTx:
		php				; Save MX bits
		long_a
		pha				; Save user data
		repeat
		 wdm	#WDM_IFR_RD		; Ready to transmit?
		 and	#INT_U1TX
		until ne
		pla				; Yes, recover data
		wdm	#WDM_U1TX		; .. and send
		plp				; Restore MX
		rts				; Done

; Receive a character from UART performing a polled wait for data to arrive.

		.longa	?
.UartRx:
		php				; Save MX bits
		long_a
		repeat
		 wdm	#WDM_IFR_RD		; Any data to read?
		 and	#INT_U1RX
		until ne
		wdm	#WDM_U1RX		; Yes, fetch a byte
		plp				; Restore MX
		rts				; Done

		.end