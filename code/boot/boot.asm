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
; Boot ROM, Monitor & Operating System
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
; This source file builds both the boot ($00:f000-ffff) and rom0 ($04:0000-ffff)
; images.
;
; The following table describes the memory map of the target system.
;
; +---------+----+-------------------------------
; | 00:0000 | RW | OS Variables & Stack
; +---------+----+-------------------------------
; | 00:1000 | RW | Task Zero Pages & Stack
; |---------+----+-------------------------------
; | 00:2000 | RW | Other tasks areas
; |	    |	 |
; |	    |	 |
; +---------+----+-------------------------------
; | 00:ee00 | RW | Monitor Workspace - Can be overwritten
; +---------+----+-------------------------------
; | 00:ef00 | RW | I/O Workspace (Timer & UART Buffers)
; +---------+----+-------------------------------
; | 00:f000 | RO | OS Boot ROM & Interrupt Handlers
; | 00:ffe0 |	 | Native Mode Vectors
; | 00:fff0 |	 | Emulation Mode Vectors
; +---------+----+-------------------------------
; | 01:0000 | RW | Video
; +---------+----+-------------------------------
; | 02:0000 | RW | SRAM
; | 03:0000 |	 |
; +---------+----+-------------------------------
; | 04:0000 | RO | OS Code + Monitor
; | 05:0000 |	 | ROM1 (Spare)
; | 06:0000 |	 | ROM2 (Spare)
; | 07:0000 |	 | ROM3 (Spare)
; +---------+----+-------------------------------
;
;
;-------------------------------------------------------------------------------

		.65816

		.include "../w65c816.inc"
		.include "../signature.inc"

;===============================================================================
; Macros
;-------------------------------------------------------------------------------

MNEM		.macro
		.word	((\0-'@')<<10)|((\1-'@')<<5)|((\2-'@')<<0)
		.endm

;===============================================================================
; Constants
;-------------------------------------------------------------------------------

; ASCII Control characters

BEL		.equ	$07
BS		.equ	$08
LF		.equ	$0a
CR		.equ	$0d
ESC		.equ	$1b
DEL		.equ	$7f

;-------------------------------------------------------------------------------

MON_PAGE	.equ	$ee00			; Monitors private data page
IO_PAGE		.equ	$ef00			; I/O private data page

UART_BUFSIZ	.equ	64			; UART buffer size

		.page
;===============================================================================
; Private I/O Data Area
;-------------------------------------------------------------------------------

		.bss
		.org	IO_PAGE

TX_HEAD:	.space	1			; Transmit buffer head and tail
TX_TAIL:	.space	1			; .. indices
RX_HEAD:	.space	1			; Receive buffer head and tail
RX_TAIL:	.space	1			; .. indices

TICK:		.space	4			; Clock tick counter

TX_DATA:	.space	UART_BUFSIZ		; Uart transmit buffer
RX_DATA:	.space	UART_BUFSIZ		; Uart receive buffer

		.if	$ > $efff
		.error	"Exceeded I/O Page size"
		.endif

;===============================================================================
; Operating System Entry Points
;-------------------------------------------------------------------------------
		.code
		.org	$f000

		brl	Uart1Tx			; JSL $f000 - UART1 Transmit
		brl	Uart1Rx			; JSL $f003 - UART1 Receive
		brl	Uart1RxCount		; JSL $f006 - UART1 RX Count

;===============================================================================
; API Entry
;-------------------------------------------------------------------------------

COPE:
		rti
COPN:
		rti

;===============================================================================
; Power On Reset
;-------------------------------------------------------------------------------

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
		ldx	#$0fff			; Set O/S stack
		txs
		lda	#INT_CLK|INT_U1RX	; Enable clock and receive
		wdm	#WDM_IER_WR
		cli				; Allow interrupts

		short_a				; Display a boot message
		ldx	#BOOT_MESSAGE
		repeat
		 lda	!0,x
		 break eq
		 jsl	Uart1Tx
		 inx
		forever

		brk	#0			; Then enter the monitor
		stp

BOOT_MESSAGE:	.byte	CR,LF,"EM-65C816-ESP32 [19.06]"
		.byte	CR,LF,"(C)2018-2019 Andrew Jacobs"
		.byte	CR,LF,0

;===============================================================================
; Uart1 I/O
;-------------------------------------------------------------------------------

; Transmit the character in A via UART1 regardless of the state of the processor
; and preserve all the registers. If the buffer is full then wait for it to
; drain so there is at least one free space.

		.longa	?
		.longi	?
Uart1Tx:
		php				; Save MX bits
		phx				; .. and X
		short_a				; Make A/M 8-bits
		pha				; Sava A & B
		xba
		pha
		lda	>TX_TAIL		; Insert data at end of queue
		tax
		xba
		sta	>TX_DATA,x
		inx				; Bump tail index
		txa
		and	#UART_BUFSIZ-1		; .. and wrap
		repeat
		 cmp	>TX_HEAD		; If buffer is completely full
		until ne			; .. wait for it to drain
		sei				; Update the tail
		sta	>TX_TAIL
		lda	#>INT_U1TX		; Ensure TX interrupt enabled
		xba
		lda	#<INT_U1TX
		wdm	#WDM_IER_SET
		cli
		pla				; Restore B & A
		xba
		pla
		plx				; Restore X
		plp				; .. and MX flags
		rtl				; Done

; Receive a character from UART1 into A regardless of the state of rhe processor
; preserving all other registers. If the buffer is empty then wait for some data
; to arrive.

		.longa	?
		.longi	?
Uart1Rx:
		php				; Save MX bits & x
		phx
		short_a				; Make A/M 8-bit
		repeat
		 lda	>RX_HEAD		; Wait while buffer is empty
		 cmp	>RX_TAIL
		until ne
		tax
		lda	>RX_DATA,x
		pha
		inx				; Bump head index
		txa
		and	#UART_BUFSIZ-1		; .. and wrap
		sta	>RX_HEAD		; Then update head
		pla
		plx				; Restore X and flags
		plp
		rtl

; Return the number of characters in the RX buffer in A.

		.longa	?
		.longi	?
Uart1RxCount:
		php				; Save MX bits
		short_a				; Make A/M 8-bit
		sec
		lda	>RX_TAIL		; Work out index difference
		sbc	>RX_HEAD
		if mi
		 adc	#UART_BUFSIZ		; Correct if negative
		endif
		plp				; Restore callers state
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
BRKN:		 jml	Monitor			; Enter the monitor
		endif

		xba				; Save users B
		pha
		phx				; .. and X

		jsr	IRQHandler		; Do common processing

		plx				; Restore users X,
		pla				; .. B, and A
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
		short_a				; Then make A/M 8-bits

		jsr	IRQHandler		; Do common processing

		long_a				; Restore users X & C
		plx
		pla
		rti				; .. and continue

;-------------------------------------------------------------------------------

; This is the main IRQ handler used in both native and emulation mode. The size
; of A/M access is 8-bits but X/Y are undefined. X is used to index into buffer
; areas but is always loaded/stored via A.

		.longa	off
		.longi	?
IRQHandler:
		phb				; Save users data bank
		phk				; And switch to bank $00
		plb

		wdm	#WDM_IFLAGS		; Fetch interrupt flags
		pha				; .. and save some copies
		pha

		and	#INT_CLK		; Is this a timer interrupt?
		if ne
		 wdm	#WDM_IFR_CLR		; Yes, clear it

		 inc	TICK+0			; Bump the tick counter
		 if eq
		  inc	TICK+1
		  if eq
		   inc	TICK+2
		   if eq
		    inc	TICK+3
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

		plb
		rts				; Done

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

		.page
;===============================================================================
; Video RAM
;-------------------------------------------------------------------------------

; Bank $01 is reserved for video data. The intended display will be 800x600
; monochrome.

SVGA_WIDTH	.equ	800
SVGA_HEIGHT	.equ	600
PIXELS_PER_BYTE	.equ	8
BYTES_PER_LINE	.equ	SVGA_WIDTH / PIXELS_PER_BYTE

		.bss
		.org	$010000

VLINES		.space	SVGA_HEIGHT * 2		; Scan line pointers

VDATA		.space	SVGA_HEIGHT * BYTES_PER_LINE
VEND		.space	0

		.page
;===============================================================================
; Operating System
;-------------------------------------------------------------------------------

		.code
		.org	$040000

; This is the target area for my operating system ROM.

		.page
;===============================================================================
; Monitor
;-------------------------------------------------------------------------------
; This is a simple monitor based on my SXB-Hacker code. It allows access to the
; emulated address space and the ability to download, inspect, change and run
; machine language programs. It uses the interrupt driven I/O routines in the
; boot ROM accessed by JSLs to $F000 and $F003.
;
; If the monitor is not in use workspace page is never accessed and could be
; used by another application.

;===============================================================================
; Opcodes & Addressing Modes
;-------------------------------------------------------------------------------

OP_ADC		.equ	0<<1
OP_AND		.equ	1<<1
OP_ASL		.equ	2<<1
OP_BCC		.equ	3<<1
OP_BCS		.equ	4<<1
OP_BEQ		.equ	5<<1
OP_BIT		.equ	6<<1
OP_BMI		.equ	7<<1
OP_BNE		.equ	8<<1
OP_BPL		.equ	9<<1
OP_BRA		.equ	10<<1
OP_BRK		.equ	11<<1
OP_BRL		.equ	12<<1
OP_BVC		.equ	13<<1
OP_BVS		.equ	14<<1
OP_CLC		.equ	15<<1
OP_CLD		.equ	16<<1
OP_CLI		.equ	17<<1
OP_CLV		.equ	18<<1
OP_CMP		.equ	19<<1
OP_COP		.equ	20<<1
OP_CPX		.equ	21<<1
OP_CPY		.equ	22<<1
OP_DEC		.equ	23<<1
OP_DEX		.equ	24<<1
OP_DEY		.equ	25<<1
OP_EOR		.equ	26<<1
OP_INC		.equ	27<<1
OP_INX		.equ	28<<1
OP_INY		.equ	29<<1
OP_JML		.equ	30<<1
OP_JMP		.equ	31<<1
OP_JSL		.equ	32<<1
OP_JSR		.equ	33<<1
OP_LDA		.equ	34<<1
OP_LDX		.equ	35<<1
OP_LDY		.equ	36<<1
OP_LSR		.equ	37<<1
OP_MVN		.equ	38<<1
OP_MVP		.equ	39<<1
OP_NOP		.equ	40<<1
OP_ORA		.equ	41<<1
OP_PEA		.equ	42<<1
OP_PEI		.equ	43<<1
OP_PER		.equ	44<<1
OP_PHA		.equ	45<<1
OP_PHB		.equ	46<<1
OP_PHD		.equ	47<<1
OP_PHK		.equ	48<<1
OP_PHP		.equ	49<<1
OP_PHX		.equ	50<<1
OP_PHY		.equ	51<<1
OP_PLA		.equ	52<<1
OP_PLB		.equ	53<<1
OP_PLD		.equ	54<<1
OP_PLP		.equ	55<<1
OP_PLX		.equ	56<<1
OP_PLY		.equ	57<<1
OP_REP		.equ	58<<1
OP_ROL		.equ	59<<1
OP_ROR		.equ	60<<1
OP_RTI		.equ	61<<1
OP_RTL		.equ	62<<1
OP_RTS		.equ	63<<1
OP_SBC		.equ	64<<1
OP_SEC		.equ	65<<1
OP_SED		.equ	66<<1
OP_SEI		.equ	67<<1
OP_SEP		.equ	68<<1
OP_STA		.equ	69<<1
OP_STP		.equ	70<<1
OP_STX		.equ	71<<1
OP_STY		.equ	72<<1
OP_STZ		.equ	73<<1
OP_TAX		.equ	74<<1
OP_TAY		.equ	75<<1
OP_TCD		.equ	76<<1
OP_TCS		.equ	77<<1
OP_TDC		.equ	78<<1
OP_TRB		.equ	79<<1
OP_TSB		.equ	80<<1
OP_TSC		.equ	81<<1
OP_TSX		.equ	82<<1
OP_TXA		.equ	83<<1
OP_TXS		.equ	84<<1
OP_TXY		.equ	85<<1
OP_TYA		.equ	86<<1
OP_TYX		.equ	87<<1
OP_WAI		.equ	88<<1
OP_WDM		.equ	89<<1
OP_XBA		.equ	90<<1
OP_XCE		.equ	91<<1

MD_ABS		.equ	0<<1			; a
MD_ACC		.equ	1<<1			; A
MD_ABX		.equ	2<<1			; a,x
MD_ABY		.equ	3<<1			; a,y
MD_ALG		.equ	4<<1			; al
MD_ALX		.equ	5<<1			; al,x
MD_AIN		.equ	6<<1			; (a)
MD_AIX		.equ	7<<1			; (a,x)
MD_DPG		.equ	8<<1			; d
MD_STK		.equ	9<<1			; d,s
MD_DPX		.equ	10<<1			; d,x
MD_DPY		.equ	11<<1			; d,x
MD_DIN		.equ	12<<1			; (d)
MD_DLI		.equ	13<<1			; [d]
MD_SKY		.equ	14<<1			; (d,s),y
MD_DIX		.equ	15<<1			; (d,x)
MD_DIY		.equ	16<<1			; (d),y
MD_DLY		.equ	17<<1			; [d],y
MD_IMP		.equ	18<<1			;
MD_REL		.equ	19<<1			; r
MD_RLG		.equ	20<<1			; rl
MD_MOV		.equ	21<<1			; xyc
MD_IMM		.equ	22<<1			; # (A or M)
MD_INT		.equ	23<<1			; # (BRK/COP/WDM)
MD_IMX		.equ	24<<1			; # (X or Y)

;===============================================================================
; Private Data Area
;-------------------------------------------------------------------------------

		.bss
		.org	MON_PAGE

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
VALUE		.space	3			; Value parsing area
DEFAULT		.space	1			; Default bank (DBR on entry)
FLAGS		.space	1			; Flag bits during disassembly

ADDR_S		.space	3			; Memory start address
ADDR_E		.space	3			; Memory end adddress

		.align	128			; Gap used for stack
CMD_BUF		.space	128			; Command buffer

;-------------------------------------------------------------------------------

; The entry point is called from the boot ROM when a BRK instruction is executed
; in either emulation or native mode.

		.code
		.longa	?
		.longi	?
		.dpage	REG_E
Monitor:
		short_a				; Ensure A/M 8-bits
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
		sta	DEFAULT
		long_i
		stx	REG_X			; Save X
		sty	REG_Y			; Save Y
		tsx
		stx	REG_SP			; Save SP
		ldx	#CMD_BUF-1		; .. then load ours
		txs

		phk				; Set DBR to this bank (to
		plb				; .. access data and strings)
		cli				; And allow interrupts

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
.OldCommand:
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
		 break	eq

		 cmp	#' '			; Printable?
		 if cs
		  cmp	#DEL			; Delete?
		  if cs
.BackSpace:	   txa				; Is buffer empty?
		   beq	.Beep			; Yes, make a noise

		   lda	#BS			; Erase the last character
		   jsr	.UartTx
		   jsr	.Space
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

		ldx	#0			; Set character index
		jsr	.SkipSpaces		; And get command

		cmp	#CR
		beq	.NewCommand

;-------------------------------------------------------------------------------
; Print Help

		cmp	#'?'
		if eq
		 ldx	#.StrHelp
		 jsr	.Print
		 bra	.NewCommand
		endif

;-------------------------------------------------------------------------------
; Set Default Bank

		cmp	#'B'
		if eq
		 jsr	.GetValue
		 if cc
		  lda	<VALUE+0
		  sta	<DEFAULT
		  jmp	.NewCommand
		 endif
		 jmp	.ShowError
		endif

;-------------------------------------------------------------------------------
; Disassemble

		cmp	#'D'
		if eq
		 lda	<REG_PC+0		; Default to current PC
		 sta	<ADDR_S+0
		 lda	<REG_PC+1
		 sta	<ADDR_S+1
		 lda	<REG_PBR
		 sta	<ADDR_S+2
		 jsr	.GetValue
		 if cc
		  jsr	.CopyToStart
		  jsr	.GetValue
		  if cc
		   jsr	.CopyToEnd
		  endif
		 endif

		 lda	<REG_P
		 sta	<FLAGS

		 repeat
		  jsr	.ShowAddress
		  jsr	.ShowBytes
		  jsr	.ShowSymbolic

		  lda	[ADDR_S]
		  pha
		  ldy	#1

		  cmp	#$18			; CLC?
		  if eq
		   lda	#C_FLAG
		   bra	.DoREP
		  endif

		  cmp	#$38			; SEC?
		  if eq
		   lda	#C_FLAG
		   bra	.DoSEP
		  endif

		  cmp	#$c2			; REP?
		  if eq
		   lda	[ADDR_S],Y
.DoREP:		   trb	<FLAGS
		   bra	.NextOpcode
		  endif

		  cmp	#$e2			; SEP?
		  if eq
		   lda	[ADDR_S],Y
.DoSEP:		   tsb	<FLAGS
		  endif

.NextOpcode:
		  pla
		  jsr	.OpcodeSize
		  jsr	.BumpAddress
		  break cs
		  jsr	.Escape
		  break	cs
		  jsr	.CompareAddr
		 until cc

		 jmp	.NewCommand
		endif

;-------------------------------------------------------------------------------
; Fill

		cmp	#'F'
		if eq
		 jsr	.GetValue
		 if cc
		  jsr	.CopyToStart
		  jsr	.GetValue
		  if cc
		   jsr	.CopyToEnd
		   jsr	.GetValue
		   if cc
		    repeat
		     lda <VALUE+0
		     sta [ADDR_S]
		     lda #1
		     jsr .BumpAddress
		     break cs
		     jsr .CompareAddr
		    until cc
		    jmp .NewCommand
		   endif
		  endif
		 endif
		 jmp	.ShowError
		endif

;-------------------------------------------------------------------------------
; Go

		cmp	#'G'
		if eq
		 jsr	.GetValue		; Try to get address
		 if cs
		  ror	<REG_E			; None, perform reset
		  lda	>$00fffc
		  sta	<VALUE+0
		  lda	>$00fffd
		  sta	<VALUE+1
		  stz	<VALUE+2
		 endif

		 sei
		 ldx	<REG_SP			; Restore user stack
		 txs

		 bit	<REG_E			; Push PBR if native mode
		 if pl
		  lda	<VALUE+2
		  pha
		 endif
		 lda	<VALUE+1		; Then PC
		 pha
		 lda	<VALUE+0
		 pha
		 lda	<REG_P			; And flags
		 pha
		 lda	<REG_DP+1		; Push DP
		 pha
		 lda	<REG_DP+0
		 pha
		 lda	<REG_DBR
		 pha
		 ldx	<REG_X			; Restore X, Y and C
		 ldy	<REG_Y
		 lda	<REG_C+1
		 xba
		 lda	<REG_C+0
		 rol	<REG_E			; Restore CPU mode
		 xce
		 plb				; Pull DBR
		 pld				; Pull DP
		 rti				; And start execution
		endif

;-------------------------------------------------------------------------------
; Memory

		cmp	#'M'
		if eq
		 jsr	.GetValue		; Get start address
		 if cc
		  jsr	.CopyToStart
		  jsr	.GetValue
		  jsr	.CopyToEnd

		  repeat
		   jsr	.ShowAddress		; Display address
		   ldy	#0
		   repeat			; Then 16 bytes of data
		    jsr	.Space
		    lda	[ADDR_S],y
		    jsr	.Hex2
		    iny
		    cpy	#16
		   until eq

		   jsr	.Space			; Show as ASCII
		   lda	#'|'
		   jsr	.UartTx

		   ldy	#0
		   repeat
		    lda	[ADDR_S],y
		    cmp #' '
		    if cc
		     lda #'.'
		    endif
		    cmp #DEL
		    if cs
		     lda #'.'
		    endif
		    jsr	.UartTx
		    iny
		    cpy #16
		   until eq
		   lda	#'|'
		   jsr	.UartTx

		   tya				; Update the address
		   jsr	.BumpAddress
		   break cs
		   jsr	.Escape
		   break cs
		   jsr	.CompareAddr
		  until cc

		  jmp	.NewCommand
		 endif
		 jmp	.ShowError
		endif

;-------------------------------------------------------------------------------
; Quit

		cmp	#'Q'
		if eq
		 stp
		endif

;-------------------------------------------------------------------------------
; Registers

		cmp	#'R'
		if eq
.SetRegister:
		 jsr	.SkipSpaces		; Search for next code
		 cmp	#CR			; End of line
		 if eq
		  jmp	.ShowRegisters		; Yes, show new state
		 endif

		 cmp	#'A'			; Set A?
		 if eq
		  ldy	#REG_C-MON_PAGE
		  bra	.SetByte
		 endif
		 cmp	#'B'			; Set DBR?
		 if eq
		  ldy	#REG_DBR-MON_PAGE
		  bra	.SetByte
		 endif
		 cmp	#'C'			; Set C?
		 if eq
		  ldy	#REG_C-MON_PAGE
		  bra	.SetWord
		 endif
		 cmp	#'D'			; Set DP?
		 if eq
		  ldy	#REG_DP-MON_PAGE
		  bra	.SetWord
		 endif
		 cmp	#'E'			; Set E?
		 if eq
		  jsr	.GetValue
		  if cc
		   ror	<VALUE
		   if cs
		    stz <REG_SP+1
		    inc <REG_SP+1
		   endif
		   ror	<REG_E
		   bra	.SetRegister
		  endif
		  jmp	.ShowError
		 endif
		 cmp	#'P'			; Set flags?
		 if eq
		  ldy	#REG_P-MON_PAGE+1
.SetByte:
		  jsr	.GetValue
		  if cc
		   phx
		   tyx
		   lda	<VALUE
		   sta	<0,x
		   plx
		   bra	.SetRegister
		  endif#
		  jmp	.ShowError
		 endif
		 cmp	#'S'			; Set SP?
		 if eq
		  ldy	#REG_SP-MON_PAGE
		  bra	.SetWord
		 endif
		 cmp	#'X'			; Set X?
		 if eq
		  ldy	#REG_X-MON_PAGE
		  bra	.SetWord
		 endif
		 cmp	#'Y'			; Set Y?
		 if eq
		  ldy	#REG_Y-MON_PAGE
.SetWord:
		  jsr	.GetValue
		  if cc
		   phx
		   tyx
		   lda	<VALUE+0
		   sta	<0,x
		   lda	<VALUE+1
		   sta	<1,x
		   plx
		   brl	.SetRegister
		  endif
		  jmp	.ShowError
		 endif

		 jmp	.ShowError		; Anything else is wrong
		endif

;-------------------------------------------------------------------------------
; S28 SRECORD loader

		cmp	#'S'
		if eq
		 jsr	.NextChar
		 cmp	#'2'			; Only process type '2'
		 if eq
		  jsr	.GetByte		; Get the byte count
		  bcs	.InvalidRecord
		  dec	a			; Ignore address and checksum
		  dec	a
		  dec	a
		  dec	a
		  beq	.InvalidRecord
		  bmi	.InvalidRecord
		  sta	<VALUE+2
		  jsr	.GetByte		; Get target address
		  bcs	.InvalidRecord
		  sta	<ADDR_S+2
		  jsr	.GetByte
		  bcs	.InvalidRecord
		  sta	<ADDR_S+1
		  jsr	.GetByte
		  bcs	.InvalidRecord
		  sta	<ADDR_S+0

		  repeat
		   jsr	.GetByte		; Get a data byte
		   bcs	.InvalidRecord
		   sta	[ADDR_S]		; .. and write to memory
		   lda	#1
		   jsr	.BumpAddress
		   dec	<VALUE+2
		  until eq
		 endif
		 jmp	.NewCommand
.InvalidRecord:
		 ldx	#.StrInvalid		; Print the error string
		 jsr	.Print
		 jmp	.NewCommand
		endif

;-------------------------------------------------------------------------------

		cmp	#'W'
		if eq
		 jsr	.GetValue		; Get start address
		 if cc
		  jsr	.CopyToStart
		  jsr	.GetValue
		  if cc
		   lda	<VALUE+0
		   sta	[ADDR_S]

		   lda	#1
		   jsr	.BumpAddress

		   lda	#'W'
		   jmp	.BuildCommand
		  endif
		  jmp	.NewCommand
		 endif
		 jmp	.ShowError
		endif

;-------------------------------------------------------------------------------

.ShowError
		ldx	#.StrError		; Print the error string
		jsr	.Print
		jmp	.NewCommand

;-------------------------------------------------------------------------------

; Parse a value in for [x[x]:]x[x][x][x] from the command line. if no bank is
; given use the current default bank.

		.longa	off
		.longi	on
.GetValue:
		stz	<VALUE+0		; Clear result area
		stz	<VALUE+1
		lda	<DEFAULT		; Assume default bank
		sta	<VALUE+2

		jsr	.SkipSpaces		; Find first digit
		jsr	.AddHexDigit
		if cs
		 rts				; None, syntax error
		endif
		jsr	.NextChar		; Try for a second
		cmp	#':'
		beq	.FoundBank		; End of bank
		jsr	.AddHexDigit
		bcs	.ReturnValue
		jsr	.NextChar		; Try for a third
		cmp	#':'
		if eq
.FoundBank:
		 lda	<VALUE+0		; Set the bank
		 sta	<VALUE+2
		 stz	<VALUE+0
		 jsr	.NextChar		; Must be followed by digit
		 jsr	.AddHexDigit
		 if cs
		  rts
		 endif
		 jsr	.NextChar
		 jsr	.AddHexDigit
		 bcs	.ReturnValue
		 jsr	.NextChar
		endif
		jsr	.AddHexDigit		; Must be offset within bank
		bcs	.ReturnValue
		jsr	.NextChar
		jsr	.AddHexDigit
.ReturnValue:	clc
		rts

; Parse a byte	from the command line and return it in A. Set the carry if a
; non-hex digit is found.

.GetByte:
		stz	<VALUE+0
		jsr	.NextChar
		jsr	.HexDigit
		if cc
		 asl	a
		 asl	a
		 asl	a
		 asl	a
		 sta	<VALUE
		 jsr	.NextChar
		 jsr	.HexDigit
		 if cc
		  ora	<VALUE+0
		 endif
		endif
		rts

; If the character in A is a hex digit then work it into the value being
; parsed from the line.

		.longa	off
.AddHexDigit:
		jsr	.HexDigit
		if cc
		 asl	<VALUE+0		; Shift up the value
		 rol	<VALUE+1
		 asl	<VALUE+0
		 rol	<VALUE+1
		 asl	<VALUE+0
		 rol	<VALUE+1
		 asl	<VALUE+0
		 rol	<VALUE+1
		 clc
		 adc	<VALUE+0
		 sta	<VALUE+0
		 if cs
		  inc	<VALUE+1
		 endif
		 clc
		endif
		rts

; Returns with the carry clear if the character in A on entry was a hexidecimal
; digit and replaces it with its value.

		.longa	off
.HexDigit:
		cmp	#'0'			; Numeric digit?
		if cs
		 cmp	#'9'+1
		 if cc
		  and	#$0f			; Yes, strip out low nybble
		  rts
		 endif

		 cmp	#'A'			; Letter A thru F?
		 if cs
		  cmp	#'F'+1
		  if cc
		   sbc	#'A'-11			; Yes.
		   clc
		   rts
		  endif
		 endif
		endif
		sec				; No.
		rts

;-------------------------------------------------------------------------------

; Copy the last parsed value into the start address.

		.longa off
.CopyToStart:
		lda	<VALUE+0
		sta	<ADDR_S+0
		lda	<VALUE+1
		sta	<ADDR_S+1
		lda	<VALUE+2
		sta	<ADDR_S+2
		rts

; Copy the last parsed value into the end address.

		.longa off
.CopyToEnd:
		lda	<VALUE+0
		sta	<ADDR_E+0
		lda	<VALUE+1
		sta	<ADDR_E+1
		lda	<VALUE+2
		sta	<ADDR_E+2
		rts

; Print the current starting address value

.ShowAddress:
		jsr	.NewLine		; Print the bank
		lda	<ADDR_S+2
		jsr	.Hex2
		lda	#':'
		jsr	.UartTx
		lda	<ADDR_S+1		; And address within it.
		xba
		lda	<ADDR_S+0
		jmp	.Hex4

; Add the value in A to the current start address. On return if the carry
; is set then the address wrapped around.

.BumpAddress:
		clc
		adc	<ADDR_S+0
		sta	<ADDR_S+0
		lda	#0
		adc	<ADDR_S+1
		sta	<ADDR_S+1
		lda	#0
		adc	<ADDR_S+2
		sta	<ADDR_S+2
		rts

; Compare the start and end addresses. Return with carry clear when the start
; is bigger than the end.

.CompareAddr:
		sec
		lda	<ADDR_E+0
		sbc	<ADDR_S+0
		lda	<ADDR_E+1
		sbc	<ADDR_S+1
		lda	<ADDR_E+2
		sbc	<ADDR_S+2
		rts

;-------------------------------------------------------------------------------

; Return the next character on the command line line that is not a space.

		.longa	off
		.longi	on
.SkipSpaces:
		repeat
		 jsr	.NextChar
		 cmp #' '			; .. until a non-space
		until ne
		rts				; Done

; Return the next character from the command line converting it to UPPER case.

		.longa	off
.NextChar:
		lda	<CMD_BUF,x		; Fetch a character
		cmp	#CR
		if ne
		 inx
		endif
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

		.longa	off
		.longi	on
.BuildCommand:
		ldx	#0
		jsr	.AddChar
		lda	#' '
		jsr	.AddChar
		lda	<VALUE+2
		jsr	.AddHex2
		lda	#':'
		jsr	.AddChar
		lda	<VALUE+1
		jsr	.AddHex2
		lda	<VALUE+0
		jsr	.AddHex2
		lda	#' '
		jsr	.AddChar
		txa
		sta	CMD_LEN
		jmp	.OldCommand

		.longa	off
		.longi	on
.AddHex2:
		pha
		lsr	a
		lsr	a
		lsr	a
		lsr	a
		jsr	.AddHex
		pla
.AddHex:
		jsr	.ToHex
.AddChar:
		sta	<CMD_BUF,x
		inx
		rts

;-------------------------------------------------------------------------------

		.longa	off
		.longi	on
.ShowBytes:
		lda	[ADDR_S]
		jsr	.OpcodeSize		; Get byte count
		tax
		ldy	#0
		repeat
		 jsr	.Space			; Show a byte
		 lda	[ADDR_S],y
		 jsr	.Hex2
		 iny
		 dex
		until eq

		repeat
		 cpy	#4
		 break eq
		 iny
		 jsr	.Space
		 jsr	.Space
		 jsr	.Space
		forever
		jmp	.Space

		.longa	off
		.longi	on
.ShowSymbolic:
		lda	[ADDR_S]		; Fetch the instruction
		tax
		lda	OPCODES,x		; Get opcode
		tay

		long_a
		lda	MNEMONICS,y		; Get the mnemonic
		pha
		lsr	a
		lsr	a
		lsr	a
		lsr	a
		lsr	a
		pha
		lsr	a
		lsr	a
		lsr	a
		lsr	a
		lsr	a
		jsr	.ExpandMnem		; And print it
		pla
		jsr	.ExpandMnem
		pla
		jsr	.ExpandMnem
		short_a
		jsr	.Space

		lda	MODES,x			; Get the addressing mode
		tax
		jmp	(.MODE_SHOW,x)

.MODE_SHOW:
		.word	.Absolute		; a
		.word	.Accumulator		; A
		.word	.AbsoluteX		; a,x
		.word	.AbsoluteY		; a,y
		.word	.Long			; al
		.word	.LongX			; al,x
		.word	.AbsoluteIndirect	; (a)
		.word	.AbsoluteXIndirect	; (a,x)
		.word	.Direct			; d
		.word	.Stack			; d,s
		.word	.DirectX		; d,x
		.word	.DirectY		; d,y
		.word	.DirectIndirect		; (d)
		.word	.DirectIndirectLong	; [d]
		.word	.StackIndirectY		; (d,s),y
		.word	.DirectXIndirect	; (d,x)
		.word	.DirectIndirectY	; (d),y
		.word	.DirectIndirectLongY	; [d],y
		.word	.Implied		;
		.word	.Relative		; r
		.word	.RelativeLong		; rl
		.word	.Move			; xyc
		.word	.ImmediateM		; # (A & M)
		.word	.ImmediateByte		; # (BRK/COP/WDM)
		.word	.ImmediateX		; # (X or Y)

.Accumulator:
		lda	#'A'
		jmp	.UartTx

.ImmediateM:
		lda	#M_FLAG
		bit	<FLAGS
		beq	.ImmediateWord
		bra	.ImmediateByte

.ImmediateX:
		lda	#X_FLAG
		bit	<FLAGS
		beq	.ImmediateWord
		bra	.ImmediateByte

.Implied:
		rts

.Move:
		lda	#'$'
		jsr	.UartTx
		ldy	#1
		lda	[ADDR_S],Y
		jsr	.Hex2
		lda	#','
		jsr	.UartTx
		lda	#'$'
		jsr	.UartTx
		iny
		lda	[ADDR_S],Y
		jmp	.Hex2

.ImmediateByte:
		lda	#'#'
		jsr	.UartTx
		bra	.Direct

.ImmediateWord:
		lda	#'#'
		jsr	.UartTx
		bra	.Absolute

.Stack:
		jsr	.Direct
		lda	#','
		jsr	.UartTx
		lda	#'S'
		jmp	.UartTx

.Direct:
		lda	#'$'
		jsr	.UartTx
		ldy	#1
		lda	[ADDR_S],Y
		jmp	.Hex2

.DirectX:
		jsr	.Direct
.X:		lda	#','
		jsr	.UartTx
		lda	#'X'
		jmp	.UartTx

.DirectY:
		jsr	.Direct
.Y:		lda	#','
		jsr	.UartTx
		lda	#'Y'
		jmp	.UartTx

.Absolute:
		lda	#'$'
		jsr	.UartTx
		ldy	#2
		lda	[ADDR_S],Y
		jsr	.Hex2
		dey
		lda	[ADDR_S],Y
		jmp	.Hex2

.AbsoluteX:
		jsr	.Absolute
		bra	.X

.AbsoluteY:
		jsr	.Absolute
		bra	.Y

.Long:
		lda	#'$'
		jsr	.UartTx
		ldy	#3
		lda	[ADDR_S],Y
		jsr	.Hex2
		lda	#':'
		jsr	.UartTx
		dey
		lda	[ADDR_S],Y
		jsr	.Hex2
		dey
		lda	[ADDR_S],Y
		jmp	.Hex2

.LongX:
		jsr	.Long
		bra	.X

.AbsoluteIndirect:
		lda	#'('
		jsr	.UartTx
		jsr	.Absolute
		lda	#')'
		jmp	.UartTx

.AbsoluteXIndirect:
		lda	#'('
		jsr	.UartTx
		jsr	.AbsoluteX
		lda	#')'
		jmp	.UartTx

.DirectIndirect:
		lda	#'('
		jsr	.UartTx
		jsr	.Direct
		lda	#')'
		jmp	.UartTx

.DirectXIndirect:
		lda	#'('
		jsr	.UartTx
		jsr	.DirectX
		lda	#')'
		jmp	.UartTx

.DirectIndirectY:
		lda	#'('
		jsr	.UartTx
		jsr	.Direct
		lda	#')'
		jsr	.UartTx
		jmp	.Y

.DirectIndirectLong:
		lda	#'['
		jsr	.UartTx
		jsr	.Direct
		lda	#']'
		jmp	.UartTx

.DirectIndirectLongY:
		jsr	.DirectIndirectLong
		jmp	.Y

.StackIndirectY:
		lda	#'('
		jsr	.UartTx
		jsr	.Stack
		lda	#')'
		jsr	.UartTx
		jmp	.Y

.Relative:
		ldx	<ADDR_S+1		; Work out next PC
		lda	<ADDR_S+0
		clc
		adc	#2
		bcc	$+3
		inx

		pha				; Add relative offset
		ldy	#1
		lda	[ADDR_S],y
		bpl	$+3
		dex
		clc
		adc	1,s
		sta	1,s
		bcc	$+3
		inx
		bra	.Addr

.RelativeLong:
		ldx	ADDR_S+1		; Work out next PC
		lda	ADDR_S+0
		clc
		adc	#3
		bcc	$+3
		inx

		clc				; Add relative offset
		ldy	#1
		adc	[ADDR_S],y
		pha
		iny
		txa
		adc	[ADDR_S],Y
		tax

.Addr:
		lda	#'$'			; Print address
		jsr	.UartTx
		txa
		jsr	.Hex2
		pla
		jmp	.Hex2



		rts


		.longa	on
		.longi	on
.ExpandMnem:
		and	#$1f
		clc
		adc	#'@'
		jmp	.UartTx

		.longa	off
		.longi	on
.OpcodeSize:
		lda	[ADDR_S]
		tax
		lda	MODES,x
		tax
		jmp	(.MODE_SIZE,x)

.MODE_SIZE:
		.word	.Size3			; a
		.word	.Size1			; A
		.word	.Size3			; a,x
		.word	.Size3			; a,y
		.word	.Size4			; al
		.word	.Size4			; al,x
		.word	.Size3			; (a)
		.word	.Size3			; (a,x)
		.word	.Size2			; d
		.word	.Size2			; d,s
		.word	.Size2			; d,x
		.word	.Size2			; d,y
		.word	.Size2			; (d)
		.word	.Size2			; [d]
		.word	.Size2			; (d,s),y
		.word	.Size2			; (d,x)
		.word	.Size2			; (d),y
		.word	.Size2			; [d],y
		.word	.Size1			;
		.word	.Size2			; r
		.word	.Size3			; rl
		.word	.Size3			; xyc
		.word	.TestM			; # (A & M)
		.word	.Size2			; # (BRK/COP/WDM)
		.word	.TestX			; # (X or Y)

.TestM:
		lda	#M_FLAG			; Is M bit set?
		and	<FLAGS
		beq	.Size3			; No, word
		bra	.Size2			; else byte

.TestX:
		lda	#X_FLAG			; Is X bit set?
		and	<FLAGS
		beq	.Size3			; No, word
		bra	.Size2			; else byte

.Size1:
		lda	#1
		rts
.Size2:
		lda	#2
		rts
.Size3
		lda	#3
		rts
.Size4:
		lda	#4
		rts

;-------------------------------------------------------------------------------

; Print the null terminated string pointed to the address in the X register to
; the UART.

		.longa	off
		.longi	on
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

		.longa	off
.NewLine:
		lda	#CR
		jsr	.UartTx
		lda	#LF
		bra	.UartTx

;-------------------------------------------------------------------------------

; Output the high byte of the C register in hex.

		.longa	off
.HexCHi:
		lda	<REG_C+1
		bra	.Hex2

; Output the high byte of the X register in hex.

		.longa	off
.HexXHi:
		lda	<REG_X+1
		bra	.Hex2

; Output the high byte of the Y register in hex.

		.longa	off
.HexYHi:
		lda	<REG_Y+1
		bra	.Hex2

; Output the high byte of the SP register in hex.

		.longa	off
.HexSPHi:
		lda	<REG_SP+1
		bra	.Hex2

; Print the value in the C register in hex.

		.longa	off
.Hex4:
		xba				; Swap the high and low bytes
		jsr	.Hex2			; Print the high byte
		xba				; Swap back then ..

; Print the value in the A registers in hex.

		.longa	off
.Hex2:
		pha				; Save the byte
		lsr	a			; Shift down the high nybble
		lsr	a
		lsr	a
		lsr	a
		jsr	.Hex			; Print it
		pla				; Recover the byte then ..

; Print the value in the low nybble of A in hex.

		.longa	off
.Hex:
		jsr	.ToHex
		bra	.UartTx


.ToHex
		and	#$0f			; Strip out the low nybble
		sed
		clc				; And convert using BCD
		adc	#$90
		adc	#$40
		cld
		rts

;-------------------------------------------------------------------------------

; Transmit the character in A using the UART. Poll the UART to see if its
; busy before outputing the character.

		.longa	?
.UartTx:
		jsl	$00f000
		rts

; Receive a character from UART performing a polled wait for data to arrive.

		.longa	?
.UartRx:
		jsl	>$00f003
		rts

; If there are any characters in the UART RX buffer then check for an ESC and
; return with carry set if it is,

		.longa	off
.Escape:
		repeat
		 jsl	>$00f006		; Is the buffer empty?
		 cmp	#0
		 break eq
		 jsl	>$00f003		; .. No, fetch a character
		 cmp	#ESC			; Test for escape
		 if eq
		  sec				; Signal stop.
		  rts
		 endif
		forever
		clc				; Continue output
		rts

; Output an openning bracket character.

		.longa	off
.Space:
		lda	#' '
		bra	.UartTx

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

.StrError:	.byte	CR,LF,"Error - Type ? for help",0
.StrInvalid:	.byte	CR,LF,"Invalid S28 record",0

.StrHelp:
		.byte	CR,LF,"Commands:"
		.byte	CR,LF,"B xx\t\t\t\t- Set default bank"
		.byte	CR,LF,"D [ss:]ssss [[ee:]eeee]\t\t- Disassemble memory"
		.byte	CR,LF,"F [ss:]ssss [ee:]eeee xx\t- Fill memory"
		.byte	CR,LF,"G [[xx:]xxxx]\t\t\t- Execute or RESET"
		.byte	CR,LF,"M [ss:]ssss [[ee:]eeee]\t\t- Display memory"
		.byte	CR,LF,"Q\t\t\t\t- Stop emulator"
		.byte	CR,LF,"R reg xx ..\t\t\t- Change/Show registers"
		.byte	CR,LF,"S...\t\t\t\t- Load S28 record"
		.byte	CR,LF,"W [ss:]ssss bb\t\t\t- Write memory byte"
		.byte	0

;-------------------------------------------------------------------------------

OPCODES:
		.byte	OP_BRK,OP_ORA,OP_COP,OP_ORA	; 00
		.byte	OP_TSB,OP_ORA,OP_ASL,OP_ORA
		.byte	OP_PHP,OP_ORA,OP_ASL,OP_PHD
		.byte	OP_TSB,OP_ORA,OP_ASL,OP_ORA
		.byte	OP_BPL,OP_ORA,OP_ORA,OP_ORA	; 10
		.byte	OP_TRB,OP_ORA,OP_ASL,OP_ORA
		.byte	OP_CLC,OP_ORA,OP_INC,OP_TCS
		.byte	OP_TRB,OP_ORA,OP_ASL,OP_ORA
		.byte	OP_JSR,OP_AND,OP_JSL,OP_AND	; 20
		.byte	OP_BIT,OP_AND,OP_ROL,OP_AND
		.byte	OP_PLP,OP_AND,OP_ROL,OP_PLD
		.byte	OP_BIT,OP_AND,OP_ROL,OP_AND
		.byte	OP_BMI,OP_AND,OP_AND,OP_AND	; 30
		.byte	OP_BIT,OP_AND,OP_ROL,OP_AND
		.byte	OP_SEC,OP_AND,OP_DEC,OP_TSC
		.byte	OP_BIT,OP_AND,OP_ROL,OP_AND
		.byte	OP_RTI,OP_EOR,OP_WDM,OP_EOR	; 40
		.byte	OP_MVP,OP_EOR,OP_LSR,OP_EOR
		.byte	OP_PHA,OP_EOR,OP_LSR,OP_PHK
		.byte	OP_JMP,OP_EOR,OP_LSR,OP_EOR
		.byte	OP_BVC,OP_EOR,OP_EOR,OP_EOR	; 50
		.byte	OP_MVN,OP_EOR,OP_LSR,OP_EOR
		.byte	OP_CLI,OP_EOR,OP_PHY,OP_TCD
		.byte	OP_JMP,OP_EOR,OP_LSR,OP_EOR
		.byte	OP_RTS,OP_ADC,OP_PER,OP_ADC	; 60
		.byte	OP_STZ,OP_ADC,OP_ROR,OP_ADC
		.byte	OP_PLA,OP_ADC,OP_ROR,OP_RTL
		.byte	OP_JMP,OP_ADC,OP_ROR,OP_ADC
		.byte	OP_BVS,OP_ADC,OP_ADC,OP_ADC	; 70
		.byte	OP_STZ,OP_ADC,OP_ROR,OP_ADC
		.byte	OP_SEI,OP_ADC,OP_PLY,OP_TDC
		.byte	OP_JMP,OP_ADC,OP_ROR,OP_ADC
		.byte	OP_BRA,OP_STA,OP_BRL,OP_STA	; 80
		.byte	OP_STY,OP_STA,OP_STX,OP_STA
		.byte	OP_DEY,OP_BIT,OP_TXA,OP_PHB
		.byte	OP_STY,OP_STA,OP_STX,OP_STA
		.byte	OP_BCC,OP_STA,OP_STA,OP_STA	; 90
		.byte	OP_STY,OP_STA,OP_STX,OP_STA
		.byte	OP_TYA,OP_STA,OP_TXS,OP_TXY
		.byte	OP_STZ,OP_STA,OP_STZ,OP_STA
		.byte	OP_LDY,OP_LDA,OP_LDX,OP_LDA	; A0
		.byte	OP_LDY,OP_LDA,OP_LDX,OP_LDA
		.byte	OP_TAY,OP_LDA,OP_TAX,OP_PLB
		.byte	OP_LDY,OP_LDA,OP_LDX,OP_LDA
		.byte	OP_BCS,OP_LDA,OP_LDA,OP_LDA	; B0
		.byte	OP_LDA,OP_LDY,OP_LDX,OP_LDA
		.byte	OP_CLV,OP_LDA,OP_TSX,OP_TYX
		.byte	OP_LDY,OP_LDA,OP_LDX,OP_LDA
		.byte	OP_CPY,OP_CMP,OP_REP,OP_CMP	; C0
		.byte	OP_CPY,OP_CMP,OP_DEC,OP_CMP
		.byte	OP_INY,OP_CMP,OP_DEX,OP_WAI
		.byte	OP_CPY,OP_CMP,OP_DEC,OP_CMP
		.byte	OP_BNE,OP_CMP,OP_CMP,OP_CMP	; D0
		.byte	OP_PEI,OP_CMP,OP_DEC,OP_CMP
		.byte	OP_CLD,OP_CMP,OP_PHX,OP_STP
		.byte	OP_JML,OP_CMP,OP_DEC,OP_CMP
		.byte	OP_CPX,OP_SBC,OP_SEP,OP_SBC	; E0
		.byte	OP_CPX,OP_SBC,OP_INC,OP_SBC
		.byte	OP_INX,OP_SBC,OP_NOP,OP_XBA
		.byte	OP_CPX,OP_SBC,OP_INC,OP_SBC
		.byte	OP_BEQ,OP_SBC,OP_SBC,OP_SBC	; F0
		.byte	OP_PEA,OP_SBC,OP_INC,OP_SBC
		.byte	OP_SED,OP_SBC,OP_PLX,OP_XCE
		.byte	OP_JSR,OP_SBC,OP_INC,OP_SBC

MODES:
		.byte	MD_INT,MD_DIX,MD_INT,MD_STK	; 00
		.byte	MD_DPG,MD_DPG,MD_DPG,MD_DLI
		.byte	MD_IMP,MD_IMM,MD_ACC,MD_IMP
		.byte	MD_ABS,MD_ABS,MD_ABS,MD_ALG
		.byte	MD_REL,MD_DIY,MD_DIN,MD_SKY	; 10
		.byte	MD_DPG,MD_DPX,MD_DPX,MD_DLY
		.byte	MD_IMP,MD_ABY,MD_ACC,MD_IMP
		.byte	MD_ABS,MD_ABX,MD_ABX,MD_ALX
		.byte	MD_ABS,MD_DIX,MD_ALG,MD_STK	; 20
		.byte	MD_DPG,MD_DPG,MD_DPG,MD_DLI
		.byte	MD_IMP,MD_IMM,MD_ACC,MD_IMP
		.byte	MD_ABS,MD_ABS,MD_ABS,MD_ALG
		.byte	MD_REL,MD_DIY,MD_DIN,MD_SKY	; 30
		.byte	MD_DPX,MD_DPX,MD_DPX,MD_DLY
		.byte	MD_IMP,MD_ABY,MD_ACC,MD_IMP
		.byte	MD_ABX,MD_ABX,MD_ABX,MD_ALX
		.byte	MD_IMP,MD_DIX,MD_INT,MD_STK	; 40
		.byte	MD_MOV,MD_DPG,MD_DPG,MD_DLI
		.byte	MD_IMP,MD_IMM,MD_ACC,MD_IMP
		.byte	MD_ABS,MD_ABS,MD_ABS,MD_ALG
		.byte	MD_REL,MD_DIY,MD_DIN,MD_SKY	; 50
		.byte	MD_MOV,MD_DPX,MD_DPX,MD_DLY
		.byte	MD_IMP,MD_ABY,MD_IMP,MD_IMP
		.byte	MD_ALG,MD_ABX,MD_ABX,MD_ALX
		.byte	MD_IMP,MD_DIX,MD_IMP,MD_STK	; 60
		.byte	MD_DPG,MD_DPG,MD_DPG,MD_DLI
		.byte	MD_IMP,MD_IMM,MD_ACC,MD_IMP
		.byte	MD_AIN,MD_ABS,MD_ABS,MD_ALG
		.byte	MD_REL,MD_DIY,MD_DIN,MD_SKY	; 70
		.byte	MD_DPX,MD_DPX,MD_DPX,MD_DLY
		.byte	MD_IMP,MD_ABY,MD_IMP,MD_IMP
		.byte	MD_AIX,MD_ABX,MD_ABX,MD_ALX
		.byte	MD_REL,MD_DIX,MD_RLG,MD_STK	; 80
		.byte	MD_DPG,MD_DPG,MD_DPG,MD_DLI
		.byte	MD_IMP,MD_IMM,MD_IMP,MD_IMP
		.byte	MD_ABS,MD_ABS,MD_ABS,MD_ALG
		.byte	MD_REL,MD_DIY,MD_DIN,MD_SKY	; 90
		.byte	MD_DPX,MD_DPX,MD_DPY,MD_DLY
		.byte	MD_IMP,MD_ABY,MD_IMP,MD_IMP
		.byte	MD_ABS,MD_ABX,MD_ABX,MD_ALX
		.byte	MD_IMX,MD_DIX,MD_IMX,MD_STK	; A0
		.byte	MD_DPG,MD_DPG,MD_DPG,MD_DLI
		.byte	MD_IMP,MD_IMM,MD_IMP,MD_IMP
		.byte	MD_ABS,MD_ABS,MD_ABS,MD_ALG
		.byte	MD_REL,MD_DIY,MD_DIN,MD_SKY	; B0
		.byte	MD_DPX,MD_DPX,MD_DPY,MD_DLY
		.byte	MD_IMP,MD_ABY,MD_IMP,MD_IMP
		.byte	MD_ABX,MD_ABX,MD_ABY,MD_ALX
		.byte	MD_IMX,MD_DIX,MD_INT,MD_STK	; C0
		.byte	MD_DPG,MD_DPG,MD_DPG,MD_DLI
		.byte	MD_IMP,MD_IMM,MD_IMP,MD_IMP
		.byte	MD_ABS,MD_ABS,MD_ABS,MD_ALG
		.byte	MD_REL,MD_DIY,MD_DIN,MD_SKY	; D0
		.byte	MD_IMP,MD_DPX,MD_DPX,MD_DLY
		.byte	MD_IMP,MD_ABY,MD_IMP,MD_IMP
		.byte	MD_AIN,MD_ABX,MD_ABX,MD_ALX
		.byte	MD_IMX,MD_DIX,MD_INT,MD_STK	; E0
		.byte	MD_DPG,MD_DPG,MD_DPG,MD_DLI
		.byte	MD_IMP,MD_IMM,MD_IMP,MD_IMP
		.byte	MD_ABS,MD_ABS,MD_ABS,MD_ALG
		.byte	MD_REL,MD_DIY,MD_DIN,MD_SKY	; F0
		.byte	MD_ABS,MD_DPX,MD_DPX,MD_DLY
		.byte	MD_IMP,MD_ABY,MD_IMP,MD_IMP
		.byte	MD_AIX,MD_ABX,MD_ABX,MD_ALX

MNEMONICS:
		MNEM	'A','D','C'
		MNEM	'A','N','D'
		MNEM	'A','S','L'
		MNEM	'B','C','C'
		MNEM	'B','C','S'
		MNEM	'B','E','Q'
		MNEM	'B','I','T'
		MNEM	'B','M','I'
		MNEM	'B','N','E'
		MNEM	'B','P','L'
		MNEM	'B','R','A'
		MNEM	'B','R','K'
		MNEM	'B','R','L'
		MNEM	'B','V','C'
		MNEM	'B','V','S'
		MNEM	'C','L','C'
		MNEM	'C','L','D'
		MNEM	'C','L','I'
		MNEM	'C','L','V'
		MNEM	'C','M','P'
		MNEM	'C','O','P'
		MNEM	'C','P','X'
		MNEM	'C','P','Y'
		MNEM	'D','E','C'
		MNEM	'D','E','X'
		MNEM	'D','E','Y'
		MNEM	'E','O','R'
		MNEM	'I','N','C'
		MNEM	'I','N','X'
		MNEM	'I','N','Y'
		MNEM	'J','M','L'
		MNEM	'J','M','P'
		MNEM	'J','S','L'
		MNEM	'J','S','R'
		MNEM	'L','D','A'
		MNEM	'L','D','X'
		MNEM	'L','D','Y'
		MNEM	'L','S','R'
		MNEM	'M','V','N'
		MNEM	'M','V','P'
		MNEM	'N','O','P'
		MNEM	'O','R','A'
		MNEM	'P','E','A'
		MNEM	'P','E','I'
		MNEM	'P','E','R'
		MNEM	'P','H','A'
		MNEM	'P','H','B'
		MNEM	'P','H','D'
		MNEM	'P','H','K'
		MNEM	'P','H','P'
		MNEM	'P','H','X'
		MNEM	'P','H','Y'
		MNEM	'P','L','A'
		MNEM	'P','L','B'
		MNEM	'P','L','D'
		MNEM	'P','L','P'
		MNEM	'P','L','X'
		MNEM	'P','L','Y'
		MNEM	'R','E','P'
		MNEM	'R','O','L'
		MNEM	'R','O','R'
		MNEM	'R','T','I'
		MNEM	'R','T','L'
		MNEM	'R','T','S'
		MNEM	'S','B','C'
		MNEM	'S','E','C'
		MNEM	'S','E','D'
		MNEM	'S','E','I'
		MNEM	'S','E','P'
		MNEM	'S','T','A'
		MNEM	'S','T','P'
		MNEM	'S','T','X'
		MNEM	'S','T','Y'
		MNEM	'S','T','Z'
		MNEM	'T','A','X'
		MNEM	'T','A','Y'
		MNEM	'T','C','D'
		MNEM	'T','C','S'
		MNEM	'T','D','C'
		MNEM	'T','R','B'
		MNEM	'T','S','B'
		MNEM	'T','S','C'
		MNEM	'T','S','X'
		MNEM	'T','X','A'
		MNEM	'T','X','S'
		MNEM	'T','X','Y'
		MNEM	'T','Y','A'
		MNEM	'T','Y','X'
		MNEM	'W','A','I'
		MNEM	'W','D','M'
		MNEM	'X','B','A'
		MNEM	'X','C','E'

		.end