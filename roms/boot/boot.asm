;===============================================================================
;  _____ __  __        __  ____   ____ ___  _  __   
; | ____|  \/  |      / /_| ___| / ___( _ )/ |/ /_  
; |  _| | |\/| |_____| '_ \___ \| |   / _ \| | '_ \ 
; | |___| |  | |_____| (_) |__) | |__| (_) | | (_) |
; |_____|_|__|_|___ __\___/____/ \____\___/|_|\___/ 
; | ____/ ___||  _ \___ /___ \                      
; |  _| \___ \| |_) ||_ \ __) |                     
; | |___ ___) |  __/___) / __/                      
; |_____|____/|_|  |____/_____|                     
;                                                   
; Boot ROM & Operating System
;-------------------------------------------------------------------------------
; Copyright (C),2019 Andrew John Jacobs
; All rights reserved.
;
; This work is made available under the terms of the Creative Commons
; Attribution-NonCommercial-ShareAlike 4.0 International license. Open the
; following URL to see the details.
;
; http://creativecommons.org/licenses/by-nc-sa/4.0/
;
;===============================================================================
; Notes:
;
;
;-------------------------------------------------------------------------------

		.65816
		
		.include "../w65c816.inc"

		.page0
		

		.bss


;===============================================================================
;-------------------------------------------------------------------------------

		.code
		.org	$f000
		
		
		
		
RESET:
		sei				; Ensure no interrupts
		cld
		native
		short_ai
		
		long_ai
		
		lda	#0			; Clear video RAM area
		sta	>VLINES
		tax
		tay
		iny
		dec	a
	lda	#$0010		
		mvn	bank(VLINES),bank(VLINES)
		
		ldx	#0
		lda	#VDATA
		repeat				
		 sta	!VLINES,x		; Setup video line offset
		 clc
		 adc	#BYTES_PER_LINE
		 inx
		 inx
		 cpx	#SVGA_HEIGHT * 2	; Repeat for entire screen
		until eq
		
		wdm	#$ff
		bra	$



COP:
IRQ:

	
;===============================================================================
; Unused Vector Trap
;-------------------------------------------------------------------------------

; If any undefined vectors are invoked execution will end up here doing in an
; infinite loop.

UnusedVector
		wdm	#$ff
		bra	$

;===============================================================================
; Vectors
;-------------------------------------------------------------------------------

; Native Mode Vectors

		.org	$ffe0

                .space  4                       ; Reserved
                .word   COP                     ; $FFE4 - COP(816)
                .word   UnusedVector            ; $FFE6 - BRK(816)
                .word	UnusedVector            ; $FFE8 - ABORT(816)
                .word	UnusedVector            ; $FFEA - NMI(816)
                .space  2                       ; Reserved
                .word	IRQ                     ; $FFEE - IRQ(816)

; Emulation Mode Vectors

		.org	$fff0
                .space  4
                .word   UnusedVector            ; $FFF4 - COP(C02)
                .space  2                       ; $Reserved
                .word   UnusedVector            ; $FFF8 - ABORT(C02)
                .word   UnusedVector            ; $FFFA - NMI(C02)
                .word   RESET                   ; $FFFC - RESET(C02)
                .word   UnusedVector            ; $FFFE - IRQBRK(C02)
		
;===============================================================================
; Video RAM
;-------------------------------------------------------------------------------

SVGA_WIDTH	.equ	800
SVGA_HEIGHT	.equ	600
PIXELS_PER_BYTE	.equ	8
BYTES_PER_LINE	.equ	SVGA_WIDTH / PIXELS_PER_BYTE

		.bss
		.org	$010000
		
VLINES		.space	SVGA_HEIGHT * 2		; Scan line pointers	

VDATA		.space 	SVGA_HEIGHT * BYTES_PER_LINE
VEND		.space	0

;-------------------------------------------------------------------------------




;===============================================================================
;-------------------------------------------------------------------------------

		.code
		.org	$040000

		.byte	"Main ROM Area",0
		
		.end