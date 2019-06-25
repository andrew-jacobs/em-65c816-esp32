;===============================================================================
; Free ROM Area for experimentation
 
		.65816

		.include "../w65c816.inc"
		.include "../signature.inc"
		
;===============================================================================
; Constants
;-------------------------------------------------------------------------------
		
; ASCII Control Characters

BEL		.equ	$07
BS		.equ	$08
LF		.equ	$0a
CR		.equ	$0d
DEL		.equ	$7f

;===============================================================================
; Data Areas
;-------------------------------------------------------------------------------

		.page0
		.org	0
		
		.space	10			; What ever you need on page 0

;-------------------------------------------------------------------------------		

		.bss
		.org	$020000
		
DATA_BANK:
		.space	10			; What ever you need in bank $02

;===============================================================================
; Startup Code
;-------------------------------------------------------------------------------
		
		.code	
		.org	$060000			; Makes listing easiler to read
		
		.longa	?			; Assume unknown state on entry
		.longi	?
EntryPoint:
		sei
		native				; Ensure we are in native mode
		short_a				; .. with 8-bit A
		long_i				; .. and 16-bit X/Y
		
		phk				; Make DBR same as PBR
		plb
		
		ldx	#BOOT_MESSAGE		; Print a message from ROM
		jsr	Print
		
		lda	#BANK DATA_BANK		; Set data bank
		pha
		plb
		
		bra	$			; Start real code here
		
BOOT_MESSAGE:	.byte	CR,LF,"Customize this ROM as needed",CR,LF,0
		
; Print the sting at 0,X in the current data bank.

		.longa	?
		.longi	on
Print:
		php				; Save MX bits
		short_a				; Ensure A/M 8-bits
		repeat
		 lda	0,x			; Load a byte of data
		 break eq			; Exit on zero byte
		 jsr	UartTx			; Other print and bump index
		 inx
		forever
		plp				; Restore MX before exit
		rts			

;===============================================================================
; Polled I/O Routines
;-------------------------------------------------------------------------------

; Transmit the character in A using the UART. Poll the UART to see if its
; busy before outputing the character.

		.longa	?
UartTx:
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
UartRx:
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