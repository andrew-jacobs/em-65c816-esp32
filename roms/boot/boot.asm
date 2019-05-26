;===============================================================================
;  _____ __  __	       __  ____	  ____ ___  _  __
; | ____|  \/  |      / /_| ___| / ___( _ )/ |/ /_
; |  _| | |\/| |_____| '_ \___ \| |   / _ \| | '_ \
; | |___| |  | |_____| (_) |__) | |__| (_) | | (_) |
; |_____|_|__|_|___ __\___/____/ \____\___/|_|\___/
; | ____/ ___||	 _ \___ /___ \
; |  _| \___ \| |_) ||_ \ __) |
; | |___ ___) |	 __/___) / __/
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
; +---------+----+-------------------------------
; | 00:0000 | WR | OS Variables & Stack
; +---------+----+-------------------------------
; | 00:1000 | WR | Task Zero Pages & Stack
; +---------+----+------------------------------
; | 00:ee00 | WR | Monitor Workspace - Can be overwritten
; +---------+----+------------------------------
; | 00:ef00 | WR | I/O Workspace (Timer & UART Buffers)
; +---------+----+------------------------------
; | 00:f000 | RO | OS Boot ROM & Interrupt Handlers
; | 00:ffe0 |    | Native Mode Vectors
; | 00:fff0 |    | Emulation Mode Vectors
; +---------+----+------------------------------
; | 01:0000 | WR | Video
; +---------+----+------------------------------
; | 02:0000 |    | 
; | 03:0000 |    |
; +---------+----+------------------------------
; | 04:0000 | RO | OS Code + Monitor
; | 05:0000 |    |
; | 06:0000 |    |
; | 07:0000 |    |
; +---------+----+-------------------------------
;
;
;-------------------------------------------------------------------------------

		.65816

		.include "../w65c816.inc"
		.include "../signature.inc"
		
;===============================================================================
;-------------------------------------------------------------------------------

MON_PAGE	.equ	$ee00
IO_PAGE		.equ	$ef00


UART_BUFSIZ	.equ	64

;===============================================================================
; Constants
;-------------------------------------------------------------------------------

; ASCII Control characters

BEL		.equ	$07
BS		.equ	$08
LF		.equ	$0a
CR		.equ	$0d
DEL		.equ	$7f

		.page0




;===============================================================================
;-------------------------------------------------------------------------------

		.bss
		.org	IO_PAGE
		
TX_HEAD:	.space	1
TX_TAIL:	.space	1
RX_HEAD:	.space	1
RX_TAIL:	.space	1

MSEC:		.space	4

TX_DATA:	.space	UART_BUFSIZ
RX_DATA:	.space	UART_BUFSIZ

		.if	$ > $efff
		.error	"Exceeded I/O Page size"
		.endif

		.code
		.org	$f000


		.longa	off
		.longi	off
RESET:
		sei				; Ensure no interrupts
		cld

		ldx	#8			; Clear FIFO indexes and timer
		repeat
		 dex
		 stz	IO_PAGE,x
		until eq
		
		clc				; Switch to native mode
		xce

		long_ai
		ldx	#$0fff
		txs
		lda	#INT_CLK|INT_U1RX	; Enable clock and receive
		wdm	#WDM_IER_WR
		cli				; Allow interrupts
		
		repeat
;		 jsl	Uart1Rx

	.if 0
		 lda	#'0'
		 jsl	Uart1Tx
		 lda	#'1'
		 jsl	Uart1Tx
		 lda	#'2'
		 jsl	Uart1Tx
		 lda	#'3'
		 jsl	Uart1Tx
		 lda	#'4'
		 jsl	Uart1Tx
		 lda	#'5'
		 jsl	Uart1Tx
		 lda	#'6'
		 jsl	Uart1Tx
		 lda	#'7'
		 jsl	Uart1Tx
		 lda	#'8'
		 jsl	Uart1Tx
		 lda	#'9'
		 jsl	Uart1Tx
	.endif
	
		 lda	#'.'
		 jsl	Uart1Tx
		 lda	MSEC+0
		 repeat
		  cmp	MSEC+0
		 until ne
		forever

		brk	#0
		stp
		
	.if 0
		long_ai

		lda	#0			; Clear video RAM area
		sta	>VLINES
		tax
		tay
		iny
		dec	a
;	lda	#$0010
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
	.endif


COPN:
		jmp	($f000,x)

;===============================================================================
; Uart1 I/O
;-------------------------------------------------------------------------------
		
		.longa	?
		.longi	?
Uart1Tx:
		php				; Save MX bits
		phx
		short_a
		pha
		xba
		pha
		lda	#0
		xba
		pha				; Insert data at end of queue
		lda	TX_TAIL	
		tax
		pla
		sta	TX_DATA,x
		inx				; Bump tail index
		txa
		and	#UART_BUFSIZ-1		; .. and wrap
		repeat
		 cmp	TX_HEAD			; If buffer is completely full
		 break ne
		 nop;wai				; .. wait for it to drain
		forever
		sei				; Update the tail
		sta	TX_TAIL
		lda	#INT_U1TX		; Ensure TX interrupt enabled
		wdm	#WDM_IER_SET
		cli
		pla				; Restore registers and flags
		xba
		pla
		xba
		plx
		plp
		rtl				; Done
		
		.longa	?
		.longi	?
Uart1Rx:
		php				; Save MX bits
		phx
		short_a
		repeat
		 lda	RX_HEAD			; Wait while buffer is empty
		 cmp	RX_TAIL
		 break ne
		 wai
		forever
		tax
		lda	RX_DATA,x
		pha
		inx				; Bump head index
		txa
		and	#UART_BUFSIZ-1		; .. and wrap
		sta	RX_HEAD			; Then update head
		pla
		plx				; Restore X and flags
		plp
		rtl

;===============================================================================
; Interrupt Handlers
;-------------------------------------------------------------------------------

; In emulation mode the interrupt handler must differentiate between IRQ and
; BRK.

		.longa	off
		.longi	off
IRQBRK:
		pha				; Save users A
		lda	2,s			; Recover P
		and	#$10
		if ne
		 pla				; Restores users A
BRKN:		 sep	#M_FLAG			; Ensure 8-bit A
		 jml	Monitor			; Enter the monitor
		endif

		xba				; Save users B				
		pha
		
		wdm	#WDM_IFR_RD		; Fetch interrupt flags
		pha				; .. and save some copies
		pha
		
		and	#INT_CLK		; Is this a timer interrupt?
		if ne
		 wdm	#WDM_IFR_CLR		; Yes, clear it
		 
		 inc	MSEC+0			; Bump the timer
		 if eq
		  inc	MSEC+1
		  if eq
		   inc	MSEC+2
		   if eq
		    inc	MSEC+3
		   endif
		  endif
		 endif
		endif
		
		pla				; Check for received data
		and	#INT_U1RX
		if ne
		 lda	RX_TAIL			; Save at tail of RX buffer
		 tax
		 wdm	#WDM_U1RX
		 sta	RX_DATA,x
		 inx				; Bump the index
		 txa
		 and	#UART_BUFSIZ-1		; .. and wrap
		 cmp	RX_HEAD			; Is RX buffer complete full?
		 if ne
		  sta	RX_TAIL			; No, save new tail
		 endif		
		endif
		
		pla				; Ready to transmit?
		and	#INT_U1TX
		if ne
		 wdm	#WDM_IER_RD		; And enabled?
		 and	#INT_U1TX
		 if ne
		  lda	TX_HEAD			; Fetch next character to send
		  tax
		  lda	TX_DATA,x
		  wdm	#WDM_U1TX		; .. and transmit it
		  inx				; Bump the index
		  txa
		  and	#UART_BUFSIZ-1		; .. and wrap
		  sta	TX_HEAD			; Save updated head
		  cmp	TX_TAIL			; Is the buffer now empty?
		  if eq
		   lda	#INT_U1TX
		   wdm	#WDM_IER_CLR		; Yes disable TX interrupt
		  endif
		 endif		 
		endif

		pla				; Restore users B & A
		xba
		pla
		rti				; .. and continue
	
;-------------------------------------------------------------------------------

		.longa	?
		.longi	?
IRQN:
		long_ai				; Then go full 16-bit
		pha				; .. and save C & X
		phx
		short_a				; Than make A 8-bits
		
		wdm	#WDM_IFR_RD		; Fetch interrupt flags
		pha				; .. and save some copies
		pha
		
		and	#INT_CLK		; Is this a timer interrupt?
		if ne
		 wdm	#WDM_IFR_CLR		; Yes, clear it
		 
		 inc	MSEC+0			; Bump the timer
		 if eq
		  inc	MSEC+1
		  if eq
		   inc	MSEC+2
		   if eq
		    inc	MSEC+3
		   endif
		  endif
		 endif
		endif
		
		pla				; Check for received data
		and	#INT_U1RX
		if ne
		 lda	RX_TAIL			; Save at tail of RX buffer
		 tax
		 wdm	#WDM_U1RX
		 sta	RX_DATA,x
		 inx				; Bump the index
		 txa
		 and	#UART_BUFSIZ-1		; .. and wrap
		 cmp	RX_HEAD			; Is RX buffer complete full?
		 if ne
		  sta	RX_TAIL			; No, save new tail
		 endif		
		endif
		
		pla				; Ready to transmit?
		and	#INT_U1TX		
		if ne
		 wdm	#WDM_IER_RD		; And enabled?
		 and	#INT_U1TX
		 if ne
		  lda	TX_HEAD			; Fetch next character to send
		  tax
		  lda	TX_DATA,x
		  wdm	#WDM_U1TX		; .. and transmit it
		  inx				; Bump the index
		  txa
		  and	#UART_BUFSIZ-1		; .. and wrap
		  sta	TX_HEAD			; Save updated head
		  cmp	TX_TAIL			; Is the buffer now empty?
		  if eq
		   lda	#INT_U1TX
		   wdm	#WDM_IER_CLR		; Yes disable TX interrupt
		  endif
		 endif	
		endif
		
		long_a
		plx
		pla
		rti

;===============================================================================
; Unused Vector Trap
;-------------------------------------------------------------------------------

; If any undefined vectors are invoked execution will end up here doing in an
; infinite loop.

UnusedVector
		stp
		bra	$

;===============================================================================
; Vectors
;-------------------------------------------------------------------------------

; Native Mode Vectors

		.org	$ffe0

		.space	4			; Reserved
		.word	COPN			; $FFE4 - COP(816)
		.word	BRKN			; $FFE6 - BRK(816)
		.word	UnusedVector		; $FFE8 - ABORT(816)
		.word	UnusedVector		; $FFEA - NMI(816)
		.space	2			; Reserved
		.word	IRQN			; $FFEE - IRQ(816)

; Emulation Mode Vectors

		.org	$fff0
		.space	4
		.word	UnusedVector		; $FFF4 - COP(C02)
		.space	2			; $Reserved
		.word	UnusedVector		; $FFF8 - ABORT(C02)
		.word	UnusedVector		; $FFFA - NMI(C02)
		.word	RESET			; $FFFC - RESET(C02)
		.word	IRQBRK			; $FFFE - IRQBRK(C02)

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

VDATA		.space	SVGA_HEIGHT * BYTES_PER_LINE
VEND		.space	0

;-------------------------------------------------------------------------------




;===============================================================================
;-------------------------------------------------------------------------------

		.code
		.org	$040000

		.byte	"Main ROM Area",0



		.page
;===============================================================================
; Monitor
;-------------------------------------------------------------------------------
;
; The monitor runs with interrupts disabled and performs polled I/O. If it is
; not entered then its workspace page is never accessed and could be used by
; another application.

		.bss
		.org	$00ee00

; User Registers

REG_E		.space	1			; In bit 7
REG_P		.space	1
REG_C		.space	2
REG_X		.space	2
REG_Y		.space	2
REG_SP		.space	2
REG_DP		.space	2
REG_PC		.space	2
REG_PBR		.space	1
REG_DBR		.space	1

CMD_LEN		.space	1			; Command buffer length

		.align	128			; Used for stack
CMD_BUF		.space	128			; Command buffer

;-------------------------------------------------------------------------------

; The entry point is called from the boot ROM when a BRK instruction is executed
; in either emulation or native mode.

		.code
		.longa	off
		.longi	off
		.dpage	REG_E
Monitor:
		phd				; Push users DP
		pea	#REG_E			; Move to monitor's direct page
		pld
		sta	REG_C+0			; Save C
		xba
		sta	REG_C+1
		pla				; Save DP
		sta	REG_DP+0
		pla
		sta	REG_DP+1
		pla				; Save P
		sta	REG_P
		sec				; Save PC (adjusting for BRK)
		pla
		sbc	#2
		sta	REG_PC+0
		pla
		sbc	#0
		sta	REG_PC+1
		clc				; Switch to native mode
		xce
		stz	REG_PBR
		if cc
		 pla
		 sta	REG_PBR			; Save PBR
		endif
		ror	REG_E			; Save E
		phb				; Save DBR
		pla
		sta	REG_DBR
		long_i
		stx	REG_X			; Save X
		sty	REG_Y			; Save Y
		tsx
		stx	REG_SP			; Save SP
		ldx	#CMD_BUF-1		; .. then load ours
		txs

		phk				; Set DBR to this bank (to
		plb				; .. access data and strings)

;-------------------------------------------------------------------------------

; Show the state of the users registers when the BRK was executed.

.ShowRegisters:
		jsr	.NewLine

		ldx	#.StrPC
		jsr	.Print
		lda	REG_PBR			; Show PBR and PC
		jsr	.Hex2
		lda	#':'
		jsr	.UartTx
		lda	REG_PC+1
		xba
		lda	REG_PC+0
		jsr	.Hex4

		ldx	#.StrE			; Show E bit
		jsr	.Print
		lda	#'0'
		bit	REG_E
		if mi
		 inc	a
		endif
		jsr	.UartTx

		ldx	#.StrP			; Show P
		jsr	.Print
		ldx	#7
		repeat
		 lda	.Mask,x			; .. as individual flags
		 and	REG_P
		 php
		 lda	.Flag,x
		 plp
		 if eq
		  lda	#'.'
		 endif
		 jsr	.UartTx
		 dex
		until mi

		ldx	#.StrC			; Show C
		jsr	.Print
		bit	REG_E
		bmi	.ShortA
		lda	#M_FLAG
		bit	REG_P
		if eq
		 jsr	.OpenBracket
		 jsr	.HexCHi
		else
.ShortA:	 jsr	.HexCHi
		 jsr	.OpenBracket
		endif
		lda	REG_C+0
		jsr	.Hex2
		jsr	.CloseBracket

		ldx	#.StrX			; Show X
		jsr	.Print
		bit	REG_E
		bmi	.ShortX
		lda	#X_FLAG
		bit	REG_P
		if eq
		 jsr	.OpenBracket
		 jsr	.HexXHi
		else
.ShortX:	 jsr	.HexXHi
		 jsr	.OpenBracket
		endif
		lda	REG_X+0
		jsr	.Hex2
		jsr	.CloseBracket

		ldx	#.StrY			; Show Y
		jsr	.Print
		bit	REG_E
		bmi	.ShortY
		lda	#X_FLAG
		bit	REG_P
		if eq
		 jsr	.OpenBracket
		 jsr	.HexYHi
		else
.ShortY:	 jsr	.HexYHi
		 jsr	.OpenBracket
		endif
		lda	REG_Y+0
		jsr	.Hex2
		jsr	.CloseBracket

		ldx	#.StrDP			; Show DP
		jsr	.Print
		lda	REG_DP+1
		xba
		lda	REG_DP+0
		jsr	.Hex4

		ldx	#.StrSP			; Show SP
		jsr	.Print
		bit	REG_E
		if mi
		 jsr	.HexSPHi
		 jsr	.OpenBracket
		else
		 jsr	.OpenBracket
		 jsr	.HexSPHi
		endif
		lda	REG_SP+0
		jsr	.Hex2
		jsr	.CloseBracket

		ldx	#.StrDBR		; Show DBR
		jsr	.Print
		lda	REG_DBR
		jsr	.Hex2

;-------------------------------------------------------------------------------

; Read a command from the user into the buffer area. Pressing either BS or DEL
; erases the last character.

.NewCommand:
		stz	CMD_LEN			; Clear command buffer
		jsr	.NewLine		; Print the entry prompt
		lda	#'.'
		jsr	.UartTx
		
		ldx	#0
		repeat
		 txa
		 cmp	CMD_LEN			; Any forced characters?
		 break	eq
		 lda	CMD_BUF,x		; Yes, print one
		 jsr	.UartTx
		 inx
		forever
		
		repeat
		 jsr	.UartRx			; Read a real character
		 sta	CMD_BUF,x		; .. and save it
		 
		 cmp	#BS			; Map BS to DEL
		 beq	.BackSpace
		 cmp	#CR			; End of input?
		 break 	eq
		 
		 cmp 	#' '			; Printable?
		 if cs
		  cmp	#DEL			; Delete?	
		  if cs
.BackSpace:	   txa				; Is buffer empty?
		   beq	.Beep			; Yes, make a noise 

		   lda	#BS			; Erase the last character
		   jsr	.UartTx
		   lda	#' '
		   jsr	.UartTx
		   lda	#BS
		   dex
		  else
		   inx				; Keep the last character
		  endif
		 else
.Beep:		  lda	#BEL
		 endif
		 jsr	.UartTx			; And echo char, BEL or BS
		forever
		
		txa				; Save the buffer length
		sta	CMD_LEN
		
		lda	#0			; Start processing at the	
		tax				; .. start of the line
		jsr	.SkipSpaces
		jsr	.ToUpper
		
		cmp	#CR
		beq	.NewCommand
		
;-------------------------------------------------------------------------------
		
		cmp 	#'?'
		if eq
		 ldx	#.StrHelp
		 jsr	.Print
		 bra	.NewCommand
		endif
		
;-------------------------------------------------------------------------------
		
		cmp 	#'D'
		if eq
		
		 jmp	.NewCommand
		endif

;-------------------------------------------------------------------------------
		
		cmp 	#'M'
		if eq
		
		 jmp	.NewCommand
		endif
		
;-------------------------------------------------------------------------------

		cmp	#'Q'
		if eq
		 jsr	.NewLine
		 stp
		endif
		
;-------------------------------------------------------------------------------
		
		cmp 	#'R'
		if eq
		
		 jmp	.ShowRegisters
		endif

;-------------------------------------------------------------------------------
		
		ldx	#.StrError
		jsr	.Print		
		jmp	.NewCommand
		
;-------------------------------------------------------------------------------

.SkipSpaces:
		repeat
		 lda	CMD_BUF,x		; Fetch characters
		 inx
		 cmp #' '			; .. until a non-space
		until ne
		rts				; Done
		
.ToUpper:
		cmp	#'a'			; If A is 'a'..'z'
		if cs
		 cmp	#'z'+1
		 if cc
		  sbc	#31			; .. then capitalise
		 endif
		endif
		rts
		
;-------------------------------------------------------------------------------

; Print the null terminated string pointed to the address in the X register to
; the UART.

.Print:
		repeat
		 lda	!0,x
		 if eq
		  rts
		 endif
		 jsr	.UartTx
		 inx
		forever

; Output a CR+LF character sequence to move the cursor to the next line.

.NewLine:
		lda	#CR
		jsr	.UartTx
		lda	#LF
		bra	.UartTx

;-------------------------------------------------------------------------------

; Output the high byte of the C register in hex.

		.longa	off
.HexCHi:
		lda	REG_C+1
		bra	.Hex2

; Output the high byte of the X register in hex.

		.longa	off
.HexXHi:
		lda	REG_X+1
		bra	.Hex2

; Output the high byte of the Y register in hex.

		.longa	off
.HexYHi:
		lda	REG_Y+1
		bra	.Hex2

; Output the high byte of the SP register in hex.

		.longa	off
.HexSPHi:
		lda	REG_SP+1
		bra	.Hex2

; Print the value in the C register in hex.

		.longa	off
.Hex4:
		xba				; Swap the high and low bytes
		jsr	.Hex2			; Print the high byte
		xba				; Swap back then ..

; Print the value in the A registers in hex.

.Hex2:
		pha				; Save the byte
		lsr	a			; Shift down the high nybble
		lsr	a
		lsr	a
		lsr	a
		jsr	.Hex			; Print it
		pla				; Recover the byte then ..

; Print the value in the low nybble of A in hex.

.Hex:
		and	#$0f			; Strip out the low nybble
		sed				; And make printable
		clc
		adc	#$90
		adc	#$40
		cld				; Then drop into ..

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

; Output an openning bracket character.

		.longa	off
.OpenBracket:
		lda	#'['
		bra	.UartTx

; Output a closing bracket character.

		.longa	off
.CloseBracket:
		lda	#']'
		bra	.UartTx

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

;-------------------------------------------------------------------------------

; 65xx Flags Bits

.Flag:		.byte	'C','Z','I','D','X','M','V','N'
.Mask:		.byte	$01,$02,$04,$08,$10,$20,$40,$80

; Various Strings

.StrPC:		.byte	"PC=",0
.StrE:		.byte	" E=",0
.StrP:		.byte	" P=",0
.StrC:		.byte	" C=",0
.StrX:		.byte	" X=",0
.StrY:		.byte	" Y=",0
.StrDP:		.byte	" DP=",0
.StrSP:		.byte	" SP=",0
.StrDBR:	.byte	" DBR=",0

.StrError:	.byte	CR,LF,"Error",0

.StrHelp:	
		.byte	CR,LF,"? - Display this help"
		.byte	0

		.end