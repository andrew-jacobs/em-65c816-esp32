#include "Emulator.h"

//==============================================================================
// Registers & State
//------------------------------------------------------------------------------

Word			Registers::pc;
Word			Registers::sp;
Word			Registers::dp;
Word			Registers::c;
Word			Registers::x;
Word			Registers::y;
Address			Registers::pbr;
Address			Registers::dbr;
Flags			Registers::p;

bool			Registers::e;

Interrupts		Registers::ier;
Interrupts		Registers::ifr;

bool			Registers::stopped;
bool			Registers::interrupted;

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

