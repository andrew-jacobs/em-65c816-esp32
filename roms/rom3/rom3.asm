;===============================================================================
; __     _______ _     ___  _  __
; \ \   / /_   _| |   ( _ )/ |/ /_
;  \ \ / /  | | | |   / _ \| | '_ \
;   \ V /   | | | |__| (_) | | (_) |
;    \_/    |_| |_____\___/|_|\___/
;
; VTL-2 for the WDC 65C816
;-------------------------------------------------------------------------------
; Notes:
;
; Based on the original VTL-2 programs by Frank McCoy and Gary Shannon (1977)
; and the adaption to the 6502 by Michael T. Barry (2012)
;
; During execution the processor is keep with A being 8-bits and X/Y 16-bits.
;
; The implementation of the text line reader is rather different from the
; original and allows lower case characters.
 
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
;-------------------------------------------------------------------------------

		.page0
		.org	0
		
DELIMITER:	.space	1
NUMBER:		.space 	2			; Number conversion workspace
		
		.org	2 * ' '
		
		.space	2			; ' '
		.space	2			; '!'
		.space	2			; '"'
HASH:		.space	2			; '#' - Number of executing line
		.space	2			; '$'
		.space	2			; '%'
AMPERSAND:	.space	2			; '&' - Free program space
		.space	2			; '''
		.space	2			; '('
		.space	2			; ')'
ASTERISK:	.space	2			; '*' - End of program space
		.space	2			; '+'
		.space	2			; ','
		.space	2			; '-'
		.space	2			; '.'
		.space	2			; '/'
		.space	2 * 10			; Digits 0-9
		.space	2			; ':'
		.space	2			; ';'
		.space	2			; '<'
		.space	2			; '='
		.space	2			; '>'
		.space	2			; '?'

		.space	2			; '@'
		.space	2 * 26			; Letters A-Z
		.space	2			; '['
		.space	2			; '\'
		.space	2			; ']'
		.space	2			; '^'
		.space	2			; '_'

		.space	2			; '`'
		.space	2 * 26			; Letters a-z
		.space	2			; '{'
		.space	2			; '|'
		.space	2			; '}'
		.space	2			; '~'
	
STACK_TOP	.equ	$01ff
		
		.bss
		.org	$020000
		
PROGRAM:	.space	$ff00
BUFFER:		.space	$0100
		
;===============================================================================
;-------------------------------------------------------------------------------
		
		.code	
		.org	$070000
		
		.longa	?
		.longi	?
VTL816:
		sei
		native				; Ensure we are in native mode
		short_a				; .. with 8-bit A
		long_i				; .. and 16-bit X/Y
		
		lda	#BANK PROGRAM		; Set data bank
		pha
		plb
		
		ldx	#PROGRAM		; Initialise memory areas
		stx	<AMPERSAND
		ldx	#BUFFER
		stx	<ASTERISK

;-------------------------------------------------------------------------------
	
.Interpret:
		ldx	#STACK_TOP		; Reset the stack
		txs
		
		jsr	.NewLine
		lda	#'O'
		jsr	.UartTx
		lda	#'K'
		jsr	.UartTx
		
		stz	HASH+0			; Clear line number
		stz	HASH+1
		jsr	.ConvertLine		; Read a command
		if cs				; Does it have a number number?
		 jsr	.Execute		; No, execute it
		 bra	.Interpret
		endif
		
		ldx	NUMBER			; Load the number
		if eq				; Is it zero?
		 ldy	#PROGRAM
		 repeat
		  cpy	AMPERSAND
		  break eq
		  ldx	!0,y
		  stx	NUMBER
		  jsr	.PrintNumber
		  iny
		  iny
		  jsr	.PrintMessage
		  jsr	.NewLine
		 forever
		 jmp	.Interpret
		endif
		
;==============================================================================
		
.Execute
		
		


;===============================================================================


.PrintMessage:
		lda	#0
		
.PrintString:
		sta	DELIMITER
		repeat
		 lda	!0,y
		 iny
		 cmp 	DELIMITER
		 break eq
		 jsr	.UartTx
		forever
		
	; TODO: Break test
		
		rts
		
.PrintNumber:

		rts


; If the character pointed to by X is digit then return with C = 0.

.IsDigit:
		lda	!0,y
		cmp	#'0'
		if cs
		 cmp 	#'9'+1
		 if cc
		  sec
		  rts
		 endif
		endif
		clc
		rts

; Read a line of text into the buffer area and then try parse a line number
; from it.

.ConvertLine:
		jsr	.ReadLine		; Read a line of text

; Try to parse a number from the buffer. Return with C = 1 if there current
; buffer location does not start a number. Return with C = 0 and 
		
.ConvertNumber:
		jsr	.IsDigit		; Found a digit?
		if cc
		 stz	NUMBER+0		; Yes
		 stz	NUMBER+1
		 repeat
		  and	#$0f			; Extract the digit
		  pha				; .. and save it
		  long_a
		  lda	NUMBER			; Copy current number
		  asl	NUMBER			; .. x2
		  asl	NUMBER			; .. x4
		  clc				; .. x5
		  adc	NUMBER
		  sta	NUMBER
		  asl	NUMBER			; .. x10
		  short_a
		  pla
		  clc
		  adc	NUMBER+0
		  sta	NUMBER+0
		  if cs
		   inc	NUMBER+1
		  endif
		  
		  iny				; Try the next character
		  jsr	.IsDigit
		 until cs
		 clc
		endif
		rts

; Read a line for text into the buffer area leaving two bytes at the start for a
; binary line number.

.ReadLine:
		ldy	#BUFFER+2		; Load buffer pointer
		phy
		repeat
		 jsr	.UartRx			; Read a character
		 sta	!0,y			; .. and save it
		 
		 cmp	#CR			; CR finished input
		 break eq
		 
		 cmp	#DEL			; Treat DEL like BS
		 if eq
		  lda	#BS
		 endif
		 
		 cmp 	#BS			; Delete last if possible
		 if eq
		  cpy	#BUFFER+3
		  if cs
		   dey
		   pha
		   jsr	.UartTx
		   lda	#' '
		   jsr	.UartTx
		   pla
		   jsr	.UartTx
		   continue
		  endif
		 endif
		 
		 cmp 	#' '			; Printable?
		 if cs
		  iny				; Yes, bump index
		 else
		  lda	#BEL			; No, ring the bell
		 endif
		 jsr	.UartTx
		forever
		
		lda	#0
		sta	!0,y			; Replace CR with NUL
		jsr	.NewLine
		ply
		rts
		
; Output a CR/LF sequence to move the terminal to the next line

.NewLine:	
		lda	#CR
		jsr	.UartTx
		lda	#LF
		jmp	.UartTx
		
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