;===============================================================================
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
		
		
		.end