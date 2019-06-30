;===============================================================================
; In the recent 'Grand Digital Computer Race' at The National Museum of Computers
; the two 6502 based machines, a BBC and Apple II only managed 70 and 38 values
; in 15 seconds -- I assume this is because it was coded in BASIC (Yuck!).
;
; We can do better. We must do better.
;
; This program works out values with up to 512 digits. On my laptop running in an
; emulator it generates 2542 results in in 0.382394 Secs with an overall CPU
; Frequency = 79.5137 Mhz. Scaled to the BBC's 2Mhz 6502 that would take 15.20
; Secs (approximately). A real BBC would be a bit slower as it has interrupts to
; handle and the I/O  would take longer but even so a more respectable ~2000
; values in 15 Secs should be possible. The Apple II will be around 50% of that
; as its clocked at 1MHz.
;
; All the real code is 6502 but I've used the WDM opcode in the emulator to
; produce output text (see UartTx) so I compile it as 65816 to enable it.
;
; Andrew Jacobs
; 2018-02-21
;-------------------------------------------------------------------------------

		.65816
		
		.include "../w65c816.inc"
		
;===============================================================================
; Macros
;-------------------------------------------------------------------------------

COMPUTE		.macro	NA,NB,NC,LA,LB,LC
		ldx	#0
		clc
		repeat
		 txa			; Reached end of shorter number?
		 eor	<LA
		 if ne
		  lda	!NA,x		; No.
		 endif
		 adc	!NB,x		; Add digits (or zero + digit)
		 sta	!NC,x
		 inx
		 txa			; Reached end of longer number?
		 eor	<LB
		until eq
		if cs			; Has result extended the value?
		 lda	#1		; Yes, add a 1 to the start
		 sta	!NC,x
		 inx
		endif
		stx	<LC		; Save the new number length
		.endm
		
DISPLAY		.macro	NM,LN
		repeat
		 lda	!NM-1,x		; Fetch two digits
		 pha
		 lsr	a		; Extract MS digits
		 lsr	a
		 lsr	a
		 lsr	a
		 if eq			; Leading zero?
		  cpx	<LN
		  beq	Skip\?	; Yes, suppress it
		 endif
		 ora	#'0'
		 jsr	UartTx
Skip\?:		 pla			; Extract LS digit
		 and	#$0f
		 ora	#'0'
		 jsr	UartTx		; And print
		 dex
		until eq
		lda	#10		; Send CR/LF to output
		jsr	UartTx`
		.endm
		
;===============================================================================
; Workspace
;-------------------------------------------------------------------------------

		.page0
		
LEN_A		.space	1
LEN_B		.space	1
LEN_C		.space	1

;===============================================================================
; Number workspaces (for 512 digits)
;-------------------------------------------------------------------------------

		.bss
		
NUM_A		.space	256
NUM_B		.space	256
NUM_C		.space	256

;===============================================================================
; The emulator expects the S28 to contain a ROM image so this is kicked off via
; the dummy reset vector.
;-------------------------------------------------------------------------------

		.code

Fibonacci:
		phk			; Ensure DBR == PBR
		plb
		short_ai
		
		ldx	#1		; Initialise number lengths
		stx	<LEN_A
		stx	<LEN_B
		stx	<LEN_C
		
		lda	#0
		sta	!NUM_A		; Start with A = 0
		stx	!NUM_B		; .. and B = 1
		
		sed			; Work in decimal mode
		
ComputeLoop:		
		; C = A + B
		
		COMPUTE NUM_A,NUM_B,NUM_C,LEN_A,LEN_B,LEN_C
		cpx	#0
		if eq
		 jmp	AllDone		; Result is over 512 digits
		endif		
		DISPLAY NUM_C,LEN_C
		
		; A = B + C

		COMPUTE NUM_B,NUM_C,NUM_A,LEN_B,LEN_C,LEN_A
		cpx	#0
		if eq
		 jmp	AllDone		; Result is over 512 digits
		endif		
		DISPLAY NUM_A,LEN_A
		
		; B = C + A

		COMPUTE NUM_C,NUM_A,NUM_B,LEN_C,LEN_A,LEN_B
		cpx	#0
		if eq
		 jmp	AllDone		; Result is over 512 digits
		endif		
		DISPLAY NUM_B,LEN_B

		jmp	ComputeLoop
		
UartTx:
		jsl	>$00f000	; Use emulator I/O
		rts
		
AllDone:
		brk	#$ff		; Re-enter the monitor
		jmp	AllDone
				
		.end