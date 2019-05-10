//==============================================================================
//  _____ __  __        __  ____   ____ ___  _  __   
// | ____|  \/  |      / /_| ___| / ___( _ )/ |/ /_  
// |  _| | |\/| |_____| '_ \___ \| |   / _ \| | '_ \ 
// | |___| |  | |_____| (_) |__) | |__| (_) | | (_) |
// |_____|_|__|_|___ __\___/____/ \____\___/|_|\___/ 
// | ____/ ___||  _ \___ /___ \                      
// |  _| \___ \| |_) ||_ \ __) |                     
// | |___ ___) |  __/___) / __/                      
// |_____|____/|_|  |____/_____|                     
//
//------------------------------------------------------------------------------                                                   
// Copyright (C),2019 Andrew John Jacobs
// All rights reserved.
//
// This work is made available under the terms of the Creative Commons
// Attribution-NonCommercial-ShareAlike 4.0 International license. Open the
// following URL to see the details.
//
// http://creativecommons.org/licenses/by-nc-sa/4.0/
//------------------------------------------------------------------------------
// Notes:
//
//==============================================================================

#include <Arduino.h>

#pragma GCC optimize ("-O4")

#include "Emulator.h"

//==============================================================================
// Registers & State
//------------------------------------------------------------------------------

Word		Registers::pc;
Word		Registers::sp;
Word		Registers::dp;
Word		Registers::c;
Word		Registers::x;
Word		Registers::y;
Address		Registers::pbr;
Address		Registers::dbr;
Flags		Registers::p;

bool		Registers::e;

Interrupts	Registers::ier;
volatile Interrupts	Registers::ifr;

bool		Registers::stopped = true;
bool		Registers::interrupted;

const OpcodeSet *Registers::pOpcodeSet;

//------------------------------------------------------------------------------

void Emulator::setMode(void)
{
	if (e)
		pOpcodeSet = &CpuModeE11::opcodeSet;
	else {
		if (p.m)
			pOpcodeSet = p.x ? &CpuModeN11::opcodeSet : &CpuModeN10::opcodeSet;
		else
			pOpcodeSet = p.x ? &CpuModeN01::opcodeSet : &CpuModeN00::opcodeSet;
	}
}

void Emulator::reset(void)
{
	pc.w = Memory::getWord(0xfffc, 0xfffd);
	sp.w = 0x0100;
	p.i = 1;
	p.d = 0;
	p.m = 1;
	p.x = 1;
	pbr.a = 0;
	dbr.a = 0;
	e = true;

	stopped = false;
	interrupted = false;

	setMode();
}