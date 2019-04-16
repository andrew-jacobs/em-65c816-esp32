;==============================================================================
; __        ____  ____   ____ ___  _  __   
; \ \      / / /_| ___| / ___( _ )/ |/ /_  
;  \ \ /\ / / '_ \___ \| |   / _ \| | '_ \ 
;   \ V  V /| (_) |__) | |__| (_) | | (_) |
;    \_/\_/  \___/____/ \____\___/|_|\___/ 
;
; Western Design Center W65C816 device definitions                                          
;------------------------------------------------------------------------------
; Copyright (C)2015-2019 HandCoded Software Ltd.
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
; Various macros and definitions for the W65C816 microprocessor.
;
;===============================================================================
; Revision History:
;
; 2015-12-18 AJ Initial version
;-------------------------------------------------------------------------------

;==============================================================================
; Status Register Bits
;------------------------------------------------------------------------------

N_FLAG          .equ    1<<7
V_FLAG          .equ    1<<6
M_FLAG          .equ    1<<5
X_FLAG          .equ    1<<4
B_FLAG          .equ    1<<4
D_FLAG          .equ    1<<3
I_FLAG          .equ    1<<2
Z_FLAG          .equ    1<<1
C_FLAG          .equ    1<<0

;==============================================================================
; Macros
;------------------------------------------------------------------------------

; Puts the processor in emulation mode. A, X and Y become 8-bits and the stack
; is fixed at $0100-$01ff. 

emulate         .macro
                sec
                xce
                .endm

; Puts the processor in native mode. The size of the memory and index register
; operations is not controlled by the M & X bits in the status register.

native          .macro
                clc
                xce
                .endm

; Resets the M bit making the accumulator and memory accesses 16-bits wide.
 
long_a          .macro
                rep     #M_FLAG
                .longa  on
                .endm

; Resets the X bit making the index registers 16-bits wide

long_i          .macro
                rep     #X_FLAG
                .longi  on
                .endm

; Resets the M and X bits making the accumulator, memory accesses and index
; registers 16-bits wide.
                
long_ai         .macro
                rep     #M_FLAG|X_FLAG
                .longa  on
                .longi  on
                .endm

; Sets the M bit making the accumulator and memory accesses 8-bits wide.

short_a         .macro
                sep     #M_FLAG
                .longa  off
                .endm

; Sets the X bit making the index registers 8-bits wide.

short_i         .macro
                sep     #X_FLAG
                .longi  off
                .endm
                
; Sets the M & X bits making the accumulator, memory accesses and index
; registers 8-bits wide.

short_ai        .macro
                sep     #M_FLAG|X_FLAG
                .longa  off
                .longi  off
                .endm
		
;===============================================================================
; WDM Signature Bytes
;-------------------------------------------------------------------------------

WDM_IER_RD	.equ	$00
WDM_IER_WR	.equ	$01
WDM_IER_SET	.equ	$02
WDM_IER_CLR	.equ	$03

WDM_IFR_RD	.equ	$04
WDM_IFR_WR	.equ	$05
WDM_IFR_SET	.equ	$06
WDM_IFR_CLR	.equ	$07

WDM_U1RX	.equ	$08
WDM_U1TX	.equ	$09

WDM_HALT	.equ	$ff

;===============================================================================
; IER/IFR Bits
;-------------------------------------------------------------------------------

INT_CLK		.equ	$0001
INT_U1RX	.equ	$0002
INT_U1TX	.equ	$0004
