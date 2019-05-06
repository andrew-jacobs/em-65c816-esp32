#ifndef EMULATOR_H
#define EMULATOR_H

#include <stdint.h>
#include <iostream>

using namespace std;

#include "Memory.h"

//==============================================================================
// Macros
//------------------------------------------------------------------------------

#if 0
#define SHOW_PC()		Trace::start()
#define SHOW_CY(CY)		Trace::cycles(CY)
#define BYTES(NM)		Trace::bytes(NM)
#define TRACE(OP)		Trace::trace (#OP, eal, eah)
#else
#define SHOW_PC()
#define SHOW_CY(CY)
#define BYTES(NM)
#define TRACE(OP)
#endif

//==============================================================================
// Data Types
//------------------------------------------------------------------------------

// A 16-bit word accessible as a whole or as high and lo bytes
union Word {
	struct {
		uint8_t				l;
		uint8_t				h;
	};
	uint16_t			w;
};

// A 24-bit address accessible as a whole or as an 8-bit bank number
union Address {
	struct {
		uint16_t			w;
		uint8_t				b;
	};
	uint32_t			a;
};

// The processor status flags as bits and as an 8-bit byte
union Flags {
	struct {
		uint8_t				c : 1;
		uint8_t				z : 1;
		uint8_t				i : 1;
		uint8_t				d : 1;
		uint8_t				x : 1;
		uint8_t				m : 1;
		uint8_t				v : 1;
		uint8_t				n : 1;
	};
	uint8_t				f;
};

// The interrupt enable and flag bits individually and as a 16-bit value
union Interrupts {
	struct {
		uint8_t				tmr : 1;
		uint8_t				u1rx : 1;
		uint8_t				u1tx : 1;
	};
	uint16_t			f;
};

//==============================================================================
// Opcode Function Table
//------------------------------------------------------------------------------

// A pointer to a opcode executing function
typedef uint8_t(*Opcode)(void);

// A set of pointers to functions that execution opcodes and interrupts
struct OpcodeSet {
	const Opcode		pIrq;
	const Opcode		pNmi;
	const Opcode		pAbort;
	const Opcode		pOpcode[256];
};

//==============================================================================
// Registers
//------------------------------------------------------------------------------

// The 65C816's register set and state variables
class Registers
{
protected:
	static volatile Word		pc;
	static volatile Word		sp;
	static volatile Word		dp;
	static volatile Word		c;
	static volatile Word		x;
	static volatile Word		y;
	static volatile Address		pbr;
	static volatile Address		dbr;
	static volatile Flags		p;
	static volatile bool		e;

	static volatile const OpcodeSet *pOpcodeSet;

	static volatile Interrupts	ier;
	static volatile Interrupts	ifr;

	static volatile bool 		stopped;
	static volatile bool		interrupted;

	Registers(void) { }

	// Set the Carry bit
	static void setc(bool v)
	{
		p.c = v ? 1 : 0;
	}

	// Set the Zero bit
	static void setz(bool v)
	{
		p.z = v ? 1 : 0;
	}

	// Set the Interrupt Disable bit
	static void seti(bool v)
	{
		p.i = v ? 1 : 0;
	}

	// Set the Decimal Arithmetic bit
	static void setd(bool v)
	{
		p.d = v ? 1 : 0;
	}

	// Set the Overflow bit
	static void setv(bool v)
	{
		p.v = v ? 1 : 0;
	}

	// Set the Negative bit
	static void setn(bool v)
	{
		p.n = v ? 1 : 0;
	}

	// Set the Negative and Zero flags to match an 8-bit value
	static void setnz_b(uint8_t v)
	{
		setn(v & 0x80);
		setz(v == 0x00);
	}

	// Set the Negative and Zero flags to match a 18-bit value
	static void setnz_w(uint16_t v)
	{
		setn(v & 0x8000);
		setz(v == 0x0000);
	}

	// Get a byte from memory
	static uint8_t getByte(uint32_t l)
	{
		return (Memory::getByte(l));
	}

	// Get a word from memory
	static uint16_t getWord(uint32_t l, uint32_t h)
	{
		return (getByte(l) | (getByte(h) << 8));
	}

	// Set a byte in memory
	static void setByte(uint32_t l, uint8_t b)
	{
		return (Memory::setByte(l, b));
	}

	// Set a word in memory
	static void setWord(uint32_t l, uint32_t h, uint16_t w)
	{
		setByte(l, w >> 0);
		setByte(h, w >> 8);
	}

	// Set the emulation as stopped.
	static void stop(void)
	{
		stopped = true;
	}

public:
	// Return the state of the stopped flag
	static bool isStopped(void)
	{
		return (stopped);
	}
};

//==============================================================================
// Trace Utility
//------------------------------------------------------------------------------

class Trace : public Registers
{
private:
	static bool			enabled;

	Trace(void) { }

	static const char *toHex(uint32_t value, uint16_t digits);

public:
	static void enable(bool state)
	{
		enabled = state;
	}

	static void start(void);
	static void bytes(uint16_t count);
	static void trace(const char *pOpcode, uint32_t eal, uint32_t eah);
	static void cycles(uint16_t cycles);
};

//==============================================================================
// Emulator
//------------------------------------------------------------------------------

class Emulator : public Registers
{
	friend class Common;
	friend class ModeE;
	friend class ModeN;
private:
	Emulator() { }

	static void setMode(void);
public:

	static void reset(void);

	static uint8_t step(void)
	{
		SHOW_PC();
		register uint8_t opcode = Memory::getByte(pbr.a | pc.w++);
		register uint8_t cycles = ((*(pOpcodeSet->pOpcode[opcode]))());
		SHOW_CY(cycles);
		return (cycles);
	}
};

//==============================================================================
// Common Opcodes and Addressing Modes
//------------------------------------------------------------------------------

class Common : public Registers
{
protected:
	// Absolute Indirect (JMP only)
	static uint8_t am_absi(uint32_t &eal, uint32_t &eah)
	{
		BYTES(2);

		register uint16_t	al = getByte(pbr.a | pc.w++);
		register uint16_t	ah = getByte(pbr.a | pc.w++);

		al = (ah << 8) | al;
		ah = al + 1;

		eal = getWord(al, ah);
		eah = 0;
		return (4);
	}

	// Absolute Indexed by X Indirect (JMP & JSR only)
	static uint8_t am_abxi(uint32_t &eal, uint32_t &eah)
	{
		BYTES(2);

		register uint16_t	al = getByte(pbr.a | pc.w++);
		register uint16_t	ah = getByte(pbr.a | pc.w++);

		al = (ah << 8) | al + x.w;
		ah = al + 1;

		eal = getWord(al, ah);
		eah = 0;
		return (5);
	}

	// Absolute Indirect Long
	static uint8_t am_abil(uint32_t &eal, uint32_t &eah)
	{
		BYTES(2);

		register uint16_t	al = getByte(pbr.a | pc.w++);
		register uint16_t	ah = getByte(pbr.a | pc.w++);

		al = (ah << 8) | al + x.w;
		ah = al + 1;

		register uint16_t	au = ah + 1;

		eal = (getByte(au) << 16) | getWord(al, ah);
		eah = eal + 1;
		return (4);
	}

	// Absolute Long
	static uint8_t am_alng(uint32_t &eal, uint32_t &eah)
	{
		BYTES(3);

		register uint8_t	al = getByte(pbr.a | pc.w++);
		register uint8_t	ah = getByte(pbr.a | pc.w++);
		register uint8_t	au = getByte(pbr.a | pc.w++);

		eal = ((au << 16) | (ah << 8) | al);
		eah = eal + 1;
		return (3);
	}

	// Absolute Long Indexed by X
	static uint8_t am_alnx(uint32_t &eal, uint32_t &eah)
	{
		BYTES(3);

		register uint8_t	al = getByte(pbr.a | pc.w++);
		register uint8_t	ah = getByte(pbr.a | pc.w++);
		register uint8_t	au = getByte(pbr.a | pc.w++);

		eal = ((au << 16) | (ah << 8) | al) + x.w;
		eah = eal + 1;
		return (3);
	}

	// Immediate Byte
	static uint8_t am_immb(uint32_t &eal, uint32_t &eah)
	{
		BYTES(1);

		eal = pbr.a | pc.w++;
		eah = 0;
		return (0);
	}

	// Immediate Word (PEA only)
	static uint8_t am_immw(uint32_t &eal, uint32_t &eah)
	{
		BYTES(2);

		eal = pbr.a | pc.w++;
		eah = pbr.a | pc.w++;
		return (0);
	}

	// Implied
	static uint8_t am_impl(uint32_t &eal, uint32_t &eah)
	{
		BYTES(0);

		eal = eah = 0;
		return (0);
	}

	// Relative
	static uint8_t am_rela(uint32_t &eal, uint32_t &eah)
	{
		BYTES(1);

		register uint8_t dl = getByte(pbr.a | pc.w++);

		eal = pbr.a | ((pc.w + (int8_t)dl) & 0xffff);
		eah = 0;
		return (1);
	}

	// Long Relative
	static uint8_t am_lrel(uint32_t &eal, uint32_t &eah)
	{
		BYTES(2);

		register uint8_t dl = getByte(pbr.a | pc.w++);
		register uint8_t dh = getByte(pbr.a | pc.w++);

		eal = pbr.a | ((pc.w + (int16_t)((dh << 8) | dl)) & 0xffff);
		eah = 0;
		return (2);
	}

	static uint8_t op_clc(uint32_t eal, uint32_t eah)
	{
		TRACE(clc);

		setc(false);
		return (2);
	}

	static uint8_t op_cld(uint32_t eal, uint32_t eah)
	{
		TRACE(cld);

		setd(false);
		return (2);
	}

	static uint8_t op_cli(uint32_t eal, uint32_t eah)
	{
		TRACE(cli);

		seti(false);
		return (2);
	}

	static uint8_t op_clv(uint32_t eal, uint32_t eah)
	{
		TRACE(clv);

		setv(false);
		return (2);
	}

	static uint8_t op_jml(uint32_t eal, uint32_t eah)
	{
		TRACE(jml);

		pbr.b = eal >> 16;
		pc.w = (uint16_t)eal;
		return (2);
	}

	static uint8_t op_jmp(uint32_t eal, uint32_t eah)
	{
		TRACE(jmp);

		pc.w = (uint16_t)eal;
		return (2);
	}

	static uint8_t op_nop(uint32_t eal, uint32_t eah)
	{
		TRACE(nop);

		return (2);
	}

	static uint8_t op_sec(uint32_t eal, uint32_t eah)
	{
		TRACE(sec);

		setc(true);
		return (2);
	}

	static uint8_t op_sed(uint32_t eal, uint32_t eah)
	{
		TRACE(sed);

		setd(true);
		return (2);
	}

	static uint8_t op_sei(uint32_t eal, uint32_t eah)
	{
		TRACE(sei);

		seti(true);
		return (2);
	}

	static uint8_t op_stp(uint32_t eal, uint32_t eah)
	{
		TRACE(stp);

		stop();
		--pc.w;
		return (2);
	}

	static uint8_t op_tcd(uint32_t eal, uint32_t eah)
	{
		TRACE(tcd);

		dp.w = c.w;
		return (2);
	}

	static uint8_t op_tcs(uint32_t eal, uint32_t eah)
	{
		TRACE(tcs);

		sp.w = c.w;
		return (2);
	}

	static uint8_t op_tdc(uint32_t eal, uint32_t eah)
	{
		TRACE(tdc);

		setnz_w(c.w = dp.w);
		return (2);
	}

	static uint8_t op_tsc(uint32_t eal, uint32_t eah)
	{
		TRACE(tsc);

		setnz_w(c.w = sp.w);
		return (2);
	}

	static uint8_t op_wai(uint32_t eal, uint32_t eah)
	{
		TRACE(wai);

		// TODO:
		return (2);
	}

	static uint8_t op_wdm(uint32_t eal, uint32_t eah)
	{
		TRACE(wdm);

		register uint8_t	cmnd = getByte(eal);

		switch (cmnd) {
	//	case 0x01:	cout << c.l;		break;
	//	case 0x02:	cin >> c.l;			break;
		case 0xff:	stop();				break;
		}
		return (3);
	}

	static uint8_t op_xba(uint32_t eal, uint32_t eah)
	{
		TRACE(xba);

		c.w = (c.l << 8) | c.h;
		setnz_b(c.l);
		return (2);
	}

	static uint8_t op_xce(uint32_t eal, uint32_t eah)
	{
		TRACE(xce);

		register bool ne = p.c;

		p.c = e;
		e = ne;
		Emulator::setMode();
		return (2);
	}
};

//==============================================================================
// Emulation Mode
//------------------------------------------------------------------------------

class ModeE : public Registers
{
private:
	ModeE() { }

	static void pushByte(uint8_t b)
	{
		setByte(sp.w, b);
		--sp.l;
	}

	static uint8_t pullByte(void)
	{
		++sp.l;
		return (getByte(sp.w));
	}

protected:
	static uint8_t do_irq(void)
	{
		pushByte(pc.h);
		pushByte(pc.l);
		pushByte(p.f | 0x30);
		p.i = 1;
		p.d = 0;

		pc.w = getWord(0xfffe, 0xffff);
		pbr.b = 0;
		return (7);
	}

	static uint8_t do_nmi(void)
	{
		pushByte(pc.h);
		pushByte(pc.l);
		pushByte(p.f | 0x30);
		p.i = 1;
		p.d = 0;

		pc.w = getWord(0xfffa, 0xfffb);
		pbr.b = 0;
		return (7);
	}

	static uint8_t do_abort(void)
	{
		pushByte(pc.h);
		pushByte(pc.l);
		pushByte(p.f | 0x30);
		p.i = 1;
		p.d = 0;

		pc.w = getWord(0xfff8, 0xfff9);
		pbr.b = 0;
		return (7);
	}

	// Absolute
	static uint8_t am_absl(uint32_t &eal, uint32_t &eah)
	{
		BYTES(2);

		register uint8_t	al = getByte(pbr.a | pc.w++);
		register uint8_t	ah = getByte(pbr.a | pc.w++);

		eal = dbr.a | ((ah << 8) | al);
		eah = eal + 1;

		return (2);
	}

	// Absolute (JMP/JSR)
	static uint8_t am_absp(uint32_t &eal, uint32_t &eah)
	{
		BYTES(2);

		register uint8_t	al = getByte(pbr.a | pc.w++);
		register uint8_t	ah = getByte(pbr.a | pc.w++);

		eal = pbr.a | ((ah << 8) | al);
		eah = eal + 1;

		return (2);
	}

	// Absolute Indexed X
	static uint8_t am_absx(uint32_t &eal, uint32_t &eah)
	{
		BYTES(2);

		register uint8_t	al = getByte(pbr.a | pc.w++);
		register uint8_t	ah = getByte(pbr.a | pc.w++);

		eal = (dbr.a | ((ah << 8) | al)) + x.w;
		eah = eal + 1;

		return (2);
	}

	// Absolute Indexed Y
	static uint8_t am_absy(uint32_t &eal, uint32_t &eah)
	{
		BYTES(2);

		register uint8_t	al = getByte(pbr.a | pc.w++);
		register uint8_t	ah = getByte(pbr.a | pc.w++);

		eal = (dbr.a | ((ah << 8) | al)) + y.w;
		eah = eal + 1;

		return (2);
	}

	// Direct Page
	static uint8_t am_dpag(uint32_t &eal, uint32_t &eah)
	{
		BYTES(1);

		register uint8_t	of = getByte(pbr.a | pc.w++);

		eal = dp.w + ((of + 0) & 0xff);
		eah = dp.w + ((of + 1) & 0xff);

		return ((dp.l) ? 2 : 1);
	}

	// Direct Page Indexed X
	static uint8_t am_dpgx(uint32_t &eal, uint32_t &eah)
	{
		BYTES(1);

		register uint8_t	of = getByte(pbr.a | pc.w++);

		eal = (dp.w + ((of + x.l + 0) & 0xff)) & 0xffff;
		eah = (dp.w + ((of + x.l + 1) & 0xff)) & 0xffff;

		return ((dp.l) ? 2 : 1);
	}

	// Direct Page Indexed Y
	static uint8_t am_dpgy(uint32_t &eal, uint32_t &eah)
	{
		BYTES(1);

		register uint8_t	of = getByte(pbr.a | pc.w++);

		eal = (dp.w + ((of + y.l + 0) & 0xff)) & 0xffff;
		eah = (dp.w + ((of + y.l + 1) & 0xff)) & 0xffff;

		return ((dp.l) ? 2 : 1);
	}

	// Direct Page Indirect
	static uint8_t am_dpgi(uint32_t &eal, uint32_t &eah)
	{
		BYTES(1);

		register uint8_t	of = getByte(pbr.a | pc.w++);

		register uint16_t	al = (dp.w + ((of + 0) & 0xff));
		register uint16_t   ah = (dp.w + ((of + 1) & 0xff));

		eal = dbr.a | getWord(al, ah);
		eah = eal + 1;		
		return ((dp.l) ? 4 : 3);
	}

	static uint8_t am_dpix(uint32_t &eal, uint32_t &eah)
	{
		BYTES(1);

		register uint8_t	of = getByte(pbr.a | pc.w++);

		register uint16_t	al = (dp.w + ((of + x.l + 0) & 0xff));
		register uint16_t   ah = (dp.w + ((of + x.l + 1) & 0xff));

		eal = dbr.a | getWord(al, ah);
		eah = eal + 1;
		return ((dp.l) ? 4 : 3);
	}

	static uint8_t am_dpiy(uint32_t &eal, uint32_t &eah)
	{
		BYTES(1);

		register uint8_t	of = getByte(pbr.a | pc.w++);

		register uint16_t	al = (dp.w + ((of + 0) & 0xff));
		register uint16_t   ah = (dp.w + ((of + 1) & 0xff));

		eal = (dbr.a | getWord(al, ah)) + y.l;
		eah = eal + 1;
		return ((dp.l) ? 4 : 3);
	}

	static uint8_t am_dpil(uint32_t &eal, uint32_t &eah)
	{
		BYTES(1);

		register uint8_t	of = getByte(pbr.a | pc.w++);
		register uint16_t	al = (dp.w + ((of + 0) & 0xff));
		register uint16_t   ah = (dp.w + ((of + 1) & 0xff));
		register uint16_t   au = (dp.w + ((of + 2) & 0xff));

		eal = (au << 16) | (ah << 8) | al;
		eah = eal + 1;
		return (4);
	}

	static uint8_t am_dily(uint32_t &eal, uint32_t &eah)
	{
		BYTES(1);

		register uint8_t	of = getByte(pbr.a | pc.w++);
		register uint16_t	al = (dp.w + ((of + 0) & 0xff));
		register uint16_t   ah = (dp.w + ((of + 1) & 0xff));
		register uint16_t   au = (dp.w + ((of + 2) & 0xff));

		eal = ((au << 16) | (ah << 8) | al) + y.l;
		eah = eal + 1;
		return (4);
	}

	// Immediate (based on M)
	static uint8_t am_immm(uint32_t &eal, uint32_t &eah)
	{
		BYTES(1);

		eal = pbr.a | pc.w++;
		eah = 0;
		return (0);
	}

	// Immediate (based on X)
	static uint8_t am_immx(uint32_t &eal, uint32_t &eah)
	{
		BYTES(1);

		eal = pbr.a | pc.w++;
		eah = 0;
		return (0);
	}

	// Stack Relative
	static uint8_t am_srel(uint32_t &eal, uint32_t &eah)
	{
		BYTES(1);

		register Word		ad;

		ad.w = sp.w;
		ad.l += getByte(pbr.a | pc.w++);
		eal = ad.w;
		eah = 0;
		return (2);
	}

	static uint8_t am_sriy(uint32_t &eal, uint32_t &eah)
	{
		BYTES(1);

		register Word		ad;

		ad.w = sp.w;
		ad.l += getByte(pbr.a | pc.w++);

		register uint8_t	al = getByte(ad.w++);
		register uint8_t	ah = getByte(ad.w++);
		
		eal = (dbr.a | (ah << 8) | al) + y.l;
		eah = eal + 1;
		return (5);
	}

	static uint8_t op_adc(uint32_t eal, uint32_t eah)
	{
		TRACE(adc);

		register uint8_t	data = Memory::getByte(eal);
		register uint16_t	temp = c.l + data + p.c;

		if (p.d) {
			if ((temp & 0x0f) > 0x09) temp += 0x06;
			if ((temp & 0xf0) > 0x90) temp += 0x60;
		}

		setc(temp & 0x100);
		setv((~(c.l ^ data)) & (c.l ^ temp) & 0x80);
		setnz_b(c.l = (uint8_t)temp);
		return (2);
	}

	static uint8_t op_and(uint32_t eal, uint32_t eah)
	{
		TRACE(and);

		setnz_b(c.l &= Memory::getByte(eal));
		return (2);
	}

	static uint8_t op_asl(uint32_t eal, uint32_t eah)
	{
		TRACE(asl);

		register uint8_t	data = Memory::getByte(eal);

		setc(data & 0x80);
		setnz_b(data <<= 1);
		Memory::setByte(eal, data);
		return (4);
	}

	static uint8_t op_asla(uint32_t eal, uint32_t eah)
	{
		TRACE(asl);

		setc(c.l & 0x80);
		setnz_b(c.l <<= 1);
		return (2);
	}

	static uint8_t op_bcc(uint32_t eal, uint32_t eah)
	{
		TRACE(bcc);

		register bool samePage = ((pc.w ^ eal) & 0xff00) == 0x0000;

		if (p.c == 0) {
			pc.w = eal;
			return (samePage ? 3 : 4);
		}
		return (2);
	}

	static uint8_t op_bcs(uint32_t eal, uint32_t eah)
	{
		TRACE(bcs);

		register bool samePage = ((pc.w ^ eal) & 0xff00) == 0x0000;

		if (p.c == 1) {
			pc.w = eal;
			return (samePage ? 3 : 4);
		}
		return (2);
	}

	static uint8_t op_beq(uint32_t eal, uint32_t eah)
	{
		TRACE(beq);

		register bool samePage = ((pc.w ^ eal) & 0xff00) == 0x0000;

		if (p.z == 1) {
			pc.w = eal;
			return (samePage ? 3 : 4);
		}
		return (2);
	}

	static uint8_t op_bit(uint32_t eal, uint32_t eah)
	{
		TRACE(bit);

		register uint8_t data = getByte(eal);

		setn(data & 0x80);
		setv(data & 0x40);
		setz(data & c.l);
		return (2);
	}

	static uint8_t op_biti(uint32_t eal, uint32_t eah)
	{
		TRACE(bit);

		register uint8_t data = getByte(eal);

		setz(data & c.l);
		return (2);
	}

	static uint8_t op_bmi(uint32_t eal, uint32_t eah)
	{
		TRACE(bmi);

		register bool samePage = ((pc.w ^ eal) & 0xff00) == 0x0000;

		if (p.n == 1) {
			pc.w = eal;
			return (samePage ? 3 : 4);
		}
		return (2);
	}

	static uint8_t op_bne(uint32_t eal, uint32_t eah)
	{
		TRACE(bne);

		register bool samePage = ((pc.w ^ eal) & 0xff00) == 0x0000;

		if (p.z == 0) {
			pc.w = eal;
			return (samePage ? 3 : 4);
		}
		return (2);
	}

	static uint8_t op_bpl(uint32_t eal, uint32_t eah)
	{
		TRACE(bpl);

		register bool samePage = ((pc.w ^ eal) & 0xff00) == 0x0000;

		if (p.n == 0) {
			pc.w = eal;
			return (samePage ? 3 : 4);
		}
		return (2);
	}

	static uint8_t op_bra(uint32_t eal, uint32_t eah)
	{
		TRACE(bra);

		register bool samePage = ((pc.w ^ eal) & 0xff00) == 0x0000;

		pc.w = eal;
		return (samePage ? 3 : 4);
	}

	static uint8_t op_brl(uint32_t eal, uint32_t eah)
	{
		TRACE(brl);

		pc.w = eal;
		return (3);
	}

	static uint8_t op_brk(uint32_t eal, uint32_t eah)
	{
		TRACE(brk);

		pushByte(pc.h);
		pushByte(pc.l);
		pushByte(p.f & 0xef);
		p.i = 1;
		p.d = 0;

		pc.w = getWord(0xfffe, 0xffff);
		pbr.b = 0;
		return (6);
	}

	static uint8_t op_bvc(uint32_t eal, uint32_t eah)
	{
		TRACE(bvc);

		register bool samePage = ((pc.w ^ eal) & 0xff00) == 0x0000;

		if (p.v == 0) {
			pc.w = eal;
			return (samePage ? 3 : 4);
		}
		return (2);
	}

	static uint8_t op_bvs(uint32_t eal, uint32_t eah)
	{
		TRACE(bvs);

		register bool samePage = ((pc.w ^ eal) & 0xff00) == 0x0000;

		if (p.v == 1) {
			pc.w = eal;
			return (samePage ? 3 : 4);
		}
		return (2);
	}

	static uint8_t op_cop(uint32_t eal, uint32_t eah)
	{
		TRACE(cop);

		pushByte(pc.h);
		pushByte(pc.l);
		pushByte(p.f | 0x30);
		p.i = 1;
		p.d = 0;

		pc.w = getWord(0xfff4, 0xfff5);
		pbr.b = 0;
		return (7);
	}

	static uint8_t op_cmp(uint32_t eal, uint32_t eah)
	{
		TRACE(cmp);

		register uint8_t  data = getByte(eal);
		register uint16_t diff = c.l - data;

		setnz_b((uint8_t)diff);
		setc(!(diff & 0x100));
		return (2);
	}

	static uint8_t op_cpx(uint32_t eal, uint32_t eah)
	{
		TRACE(cpx);

		register uint8_t	data = getByte(eal);
		register uint16_t	diff = x.l - data;

		setnz_b((uint8_t)diff);
		setc(!(diff & 0x100));
		return (2);
	}

	static uint8_t op_cpy(uint32_t eal, uint32_t eah)
	{
		TRACE(cpy);

		register uint8_t	data =getByte(eal);
		register uint16_t	diff = y.l - data;

		setnz_b((uint8_t)diff);
		setc(!(diff & 0x100));
		return (2);
	}

	static uint8_t op_dec(uint32_t eal, uint32_t eah)
	{
		TRACE(dec);

		register uint8_t	data = getByte(eal);

		setByte(eal, --data);
		setnz_b(data);
		return (4);
	}

	static uint8_t op_deca(uint32_t eal, uint32_t eah)
	{
		TRACE(dec);

		setnz_b(--c.l);
		return (2);
	}

	static uint8_t op_dex(uint32_t eal, uint32_t eah)
	{
		TRACE(dex);

		setnz_b(--x.l);
		return (2);
	}

	static uint8_t op_dey(uint32_t eal, uint32_t eah)
	{
		TRACE(dey);

		setnz_b(--y.l);
		return (2);
	}

	static uint8_t op_eor(uint32_t eal, uint32_t eah)
	{
		TRACE(eor);

		setnz_b(c.l ^= getByte(eal));
		return (2);
	}

	static uint8_t op_inc(uint32_t eal, uint32_t eah)
	{
		TRACE(inc);

		register uint8_t	data = getByte(eal);

		setByte(eal, ++data);
		setnz_b(data);
		return (4);
	}

	static uint8_t op_inca(uint32_t eal, uint32_t eah)
	{
		TRACE(inc);

		setnz_b(++c.l);
		return (2);
	}

	static uint8_t op_inx(uint32_t eal, uint32_t eah)
	{
		TRACE(inx);

		setnz_b(++x.l);
		return (2);
	}

	static uint8_t op_iny(uint32_t eal, uint32_t eah)
	{
		TRACE(iny);

		setnz_b(++y.l);
		return (2);
	}

	static uint8_t op_jsl(uint32_t eal, uint32_t eah)
	{
		TRACE(jsl);

		--pc.w;
		pushByte(pbr.b);
		pushByte(pc.h);
		pushByte(pc.l);

		pbr.b = eal >> 16;
		pc.w = (uint16_t)eal;
		return (2);
	}

	static uint8_t op_jsr(uint32_t eal, uint32_t eah)
	{
		TRACE(jsr);

		--pc.w;
		pushByte(pc.h);
		pushByte(pc.l);

		pc.w = (uint16_t)eal;
		return (4);
	}

	static uint8_t op_lda(uint32_t eal, uint32_t eah)
	{
		TRACE(lda);

		setnz_b(c.l = getByte(eal));
		return (2);
	}

	static uint8_t op_ldx(uint32_t eal, uint32_t eah)
	{
		TRACE(ldx);

		setnz_b(x.l = getByte(eal));
		return (2);
	}

	static uint8_t op_ldy(uint32_t eal, uint32_t eah)
	{
		TRACE(ldy);

		setnz_b(y.l = getByte(eal));
		return (2);
	}

	static uint8_t op_lsr(uint32_t eal, uint32_t eah)
	{
		TRACE(lsr);

		register uint8_t	data = getByte(eal);

		setc(data & 0x01);
		setnz_b(data >>= 1);
		setByte(eal, data);
		return (4);
	}

	static uint8_t op_lsra(uint32_t eal, uint32_t eah)
	{
		TRACE(lsr);

		setc(c.l & 0x01);
		setnz_b(c.l >>= 1);
		return (2);
	}

	static uint8_t op_mvn(uint32_t eal, uint32_t eah)
	{
		TRACE(mvn);

		register uint32_t	dst = getByte(eal) << 16;
		register uint32_t	src = getByte(eah) << 16;

		setByte(dbr.a = dst | y.l++, getByte(src | x.l++));
		if (c.l-- != 0x00) pc.w -= 3;
		return (7);
	}

	static uint8_t op_mvp(uint32_t eal, uint32_t eah)
	{
		TRACE(mvp);

		register uint32_t	dst = getByte(eal) << 16;
		register uint32_t	src = getByte(eah) << 16;

		setByte(dbr.a = dst | y.l--, getByte(src | x.l--));
		if (c.l-- != 0x00) pc.w -= 3;
		return (7);
	}

	static uint8_t op_ora(uint32_t eal, uint32_t eah)
	{
		TRACE(ora);

		setnz_b(c.l |= getByte(eal));
		return (2);
	}

	static uint8_t op_pea(uint32_t eal, uint32_t eah)
	{
		TRACE(pea);

		pushByte(getByte(eah));
		pushByte(getByte(eal));
		return (5);
	}

	static uint8_t op_pei(uint32_t eal, uint32_t eah)
	{
		TRACE(pei);

		pushByte(getByte(eah));
		pushByte(getByte(eal));
		return (5);
	}

	static uint8_t op_per(uint32_t eal, uint32_t eah)
	{
		TRACE(pel);

		pushByte(eal >> 8);
		pushByte(eal >> 0);
		return (5);
	}

	static uint8_t op_pha(uint32_t eal, uint32_t eah)
	{
		TRACE(pha);

		pushByte(c.l);
		return (3);
	}

	static uint8_t op_phb(uint32_t eal, uint32_t eah)
	{
		TRACE(phb);

		pushByte(dbr.b);
		return (3);
	}

	static uint8_t op_phd(uint32_t eal, uint32_t eah)
	{
		TRACE(phd);

		pushByte(dp.h);
		pushByte(dp.l);
		return (4);
	}

	static uint8_t op_phk(uint32_t eal, uint32_t eah)
	{
		TRACE(phk);

		pushByte(pbr.b);
		return (3);
	}

	static uint8_t op_plb(uint32_t eal, uint32_t eah)
	{
		TRACE(plb);

		dbr.b = pullByte();
		setnz_b(dbr.b);
		return (4);
	}

	static uint8_t op_pld(uint32_t eal, uint32_t eah)
	{
		TRACE(pld);

		dp.l = pullByte();
		dp.h = pullByte();
		setnz_w(dp.w);
		return (5);
	}

	static uint8_t op_php(uint32_t eal, uint32_t eah)
	{
		TRACE(php);

		pushByte(p.f | 0x30);
		return (3);
	}

	static uint8_t op_phx(uint32_t eal, uint32_t eah)
	{
		TRACE(phx);

		pushByte(x.l);
		return (3);
	}

	static uint8_t op_phy(uint32_t eal, uint32_t eah)
	{
		TRACE(phy);

		pushByte(y.l);
		return (3);
	}

	static uint8_t op_pla(uint32_t eal, uint32_t eah)
	{
		TRACE(pla);

		setnz_b(c.l = pullByte());
		return (4);
	}

	static uint8_t op_plp(uint32_t eal, uint32_t eah)
	{
		TRACE(plp);

		setnz_b(p.f = pullByte() | 0x30);
		return (4);
	}

	static uint8_t op_plx(uint32_t eal, uint32_t eah)
	{
		TRACE(plx);

		setnz_b(x.l = pullByte());
		return (4);
	}

	static uint8_t op_ply(uint32_t eal, uint32_t eah)
	{
		TRACE(ply);

		setnz_b(y.l = pullByte());
		return (4);
	}

	static uint8_t op_rep(uint32_t eal, uint32_t eah)
	{
		TRACE(rep);

		p.f &= ~getByte(eal);
		p.m = p.x = 1;
		return (3);
	}

	static uint8_t op_rol(uint32_t eal, uint32_t eah)
	{
		TRACE(rol);

		register uint8_t data = getByte(eal);
		register uint8_t cin  = p.c ? 0x01 : 0x00;

		setc(data & 0x80);
		data = (data << 1) | cin;
		setByte(eal, data);
		setnz_b(data);
		return (4);
	}

	static uint8_t op_rola(uint32_t eal, uint32_t eah)
	{
		TRACE(rol);

		register uint8_t cin = p.c ? 0x01 : 0x00;

		setc(c.l & 0x80);
		c.l = (c.l << 1) | cin;
		setnz_b(c.l);
		return (2);
	}

	static uint8_t op_ror(uint32_t eal, uint32_t eah)
	{
		TRACE(ror);

		register uint8_t data = getByte(eal);
		register uint8_t cin = p.c ? 0x80 : 0x00;

		setc(data & 0x01);
		data = (data >> 1) | cin;
		setByte(eal, data);
		setnz_b(data);
		return (4);
	}

	static uint8_t op_rora(uint32_t eal, uint32_t eah)
	{
		TRACE(ror);

		register uint8_t cin = p.c ? 0x80 : 0x00;

		setc(c.l & 0x01);
		c.l = (c.l >> 1) | cin;
		setnz_b(c.l);
		return (2);
	}

	static uint8_t op_rti(uint32_t eal, uint32_t eah)
	{
		TRACE(rti);

		p.f = pullByte() | 0x30;
		pc.l = pullByte();
		pc.h = pullByte();
		return (6);
	}

	static uint8_t op_rtl(uint32_t eal, uint32_t eah)
	{
		TRACE(rtl);

		pc.l = pullByte();
		pc.h = pullByte();
		pbr.b = pullByte();
		++pc.w;
		return (6);
	}

	static uint8_t op_rts(uint32_t eal, uint32_t eah)
	{
		TRACE(rts);

		pc.l = pullByte();
		pc.h = pullByte();
		++pc.w;
		return (6);
	}

	static uint8_t op_sbc(uint32_t eal, uint32_t eah)
	{
		TRACE(sbc);

		register uint8_t	data = getByte(eal);
		register uint16_t	temp = c.l + (data ^ 0xff) + p.c;

		if (p.d) {
			if ((temp & 0x0f) > 0x09) temp += 0x06;
			if ((temp & 0xf0) > 0x90) temp += 0x60;
		}

		setc(temp & 0x100);
		setv((~(c.l ^ data)) & (c.l ^ temp) & 0x80);
		setnz_b(c.l = (uint8_t)temp);
		return (2);
	}

	static uint8_t op_sep(uint32_t eal, uint32_t eah)
	{
		TRACE(sep);

		p.f |= getByte(eal);
		return (3);
	}

	static uint8_t op_sta(uint32_t eal, uint32_t eah)
	{
		TRACE(sta);

		setByte(eal, c.l);
		return (2);
	}

	static uint8_t op_stx(uint32_t eal, uint32_t eah)
	{
		TRACE(stx);

		setByte(eal, x.l);
		return (2);
	}

	static uint8_t op_sty(uint32_t eal, uint32_t eah)
	{
		TRACE(sty);

		setByte(eal, y.l);
		return (2);
	}

	static uint8_t op_stz(uint32_t eal, uint32_t eah)
	{
		TRACE(stz);

		setByte(eal, 0x00);
		return (2);
	}

	static uint8_t op_tax(uint32_t eal, uint32_t eah)
	{
		TRACE(tax);

		setnz_b(x.l = c.l);
		return (2);
	}

	static uint8_t op_tay(uint32_t eal, uint32_t eah)
	{
		TRACE(tay);

		setnz_b(y.l = c.l);
		return (2);
	}

	static uint8_t op_trb(uint32_t eal, uint32_t eah)
	{
		TRACE(trb);

		register uint8_t data = getByte(eal);

		setz(data & c.l);
		setByte(eal, data & ~c.l);
		return (2);
	}

	static uint8_t op_tsb(uint32_t eal, uint32_t eah)
	{
		TRACE(tsb);

		register uint8_t data = getByte(eal);

		setz(data & c.l);
		setByte(eal, data | c.l);
		return (2);
	}

	static uint8_t op_tsx(uint32_t eal, uint32_t eah)
	{
		TRACE(tsx);

		setnz_b(x.l = sp.l);
		return (2);
	}

	static uint8_t op_txa(uint32_t eal, uint32_t eah)
	{
		TRACE(txa);

		setnz_b(c.l = x.l);
		return (2);
	}

	 static uint8_t op_txs(uint32_t eal, uint32_t eah)
	{
		TRACE(txs);

		sp.l = x.l;
		return (2);
	}

	static uint8_t op_txy(uint32_t eal, uint32_t eah)
	{
		TRACE(txy);

		y.l = x.l;
		return (2);
	}

	static uint8_t op_tya(uint32_t eal, uint32_t eah)
	{
		TRACE(tya);

		setnz_b(c.l = y.l);
		return (2);
	}

	static uint8_t op_tyx(uint32_t eal, uint32_t eah)
	{
		TRACE(tyx);

		setnz_b(x.l = y.l);
		return (2);
	}
};

//==============================================================================
// Native Mode
//------------------------------------------------------------------------------

class ModeN : public Registers
{
	friend class ModeM0;
	friend class ModeM1;
	friend class ModeX0;
	friend class ModeX1;
private:
	ModeN(void) { }

	static void pushByte(uint8_t b)
	{
		setByte(sp.w, b);
		--sp.w;
	}

	static uint8_t pullByte(void)
	{
		++sp.w;
		return (getByte(sp.w));
	}

protected:
	static uint8_t do_irq(void)
	{
		pushByte(pbr.b);
		pushByte(pc.h);
		pushByte(pc.l);
		pushByte(p.f);
		p.i = 1;
		p.d = 0;

		pc.w = getWord(0xffee, 0xffef);
		pbr.b = 0;
		return (8);
	}

	static uint8_t do_nmi(void)
	{
		pushByte(pbr.b);
		pushByte(pc.h);
		pushByte(pc.l);
		pushByte(p.f);
		p.i = 1;
		p.d = 0;

		pc.w = getWord(0xffea, 0xffeb);
		pbr.b = 0;
		return (8);
	}

	static uint8_t do_abort(void)
	{
		pushByte(pbr.b);
		pushByte(pc.h);
		pushByte(pc.l);
		pushByte(p.f);
		p.i = 1;
		p.d = 0;

		pc.w = getWord(0xffe8, 0xffe9);
		pbr.b = 0;
		return (8);
	}

	// Absolute
	static uint8_t am_absl(uint32_t &eal, uint32_t &eah)
	{
		BYTES(2);

		register uint8_t	al = getByte(pbr.a | pc.w++);
		register uint8_t	ah = getByte(pbr.a | pc.w++);

		eal = dbr.a | ((ah << 8) | al);
		eah = eal + 1;

		return (2);
	}

	// Absolute (JMP/JSR)
	static uint8_t am_absp(uint32_t &eal, uint32_t &eah)
	{
		BYTES(2);

		register uint8_t	al = getByte(pbr.a | pc.w++);
		register uint8_t	ah = getByte(pbr.a | pc.w++);

		eal = pbr.a | ((ah << 8) | al);
		eah = eal + 1;

		return (2);
	}

	// Absolute Indexed X
	static uint8_t am_absx(uint32_t &eal, uint32_t &eah)
	{
		BYTES(2);

		register uint8_t	al = getByte(pbr.a | pc.w++);
		register uint8_t	ah = getByte(pbr.a | pc.w++);

		eal = (dbr.a | ((ah << 8) | al)) + x.w;
		eah = eal + 1;

		return (2);
	}

	// Absolute Indexed Y
	static uint8_t am_absy(uint32_t &eal, uint32_t &eah)
	{
		BYTES(2);

		register uint8_t	al = getByte(pbr.a | pc.w++);
		register uint8_t	ah = getByte(pbr.a | pc.w++);

		eal = (dbr.a | ((ah << 8) | al)) + y.w;
		eah = eal + 1;

		return (2);
	}

	// Direct Page
	static uint8_t am_dpag(uint32_t &eal, uint32_t &eah)
	{
		BYTES(1);

		register uint8_t	of = getByte(pbr.a | pc.w++);

		eal = (dp.w + of + 0) & 0xffff;
		eah = (dp.w + of + 1) & 0xffff;

		return ((dp.l) ? 2 : 1);
	}

	// Direct Page Indexed X
	static uint8_t am_dpgx(uint32_t &eal, uint32_t &eah)
	{
		BYTES(1);

		register uint8_t	of = getByte(pbr.a | pc.w++);

		eal = (dp.w + of + x.w + 0) & 0xffff;
		eah = (dp.w + of + x.w + 1) & 0xffff;

		return ((dp.l) ? 2 : 1);
	}

	// Direct Page Indexed Y
	static uint8_t am_dpgy(uint32_t &eal, uint32_t &eah)
	{
		BYTES(1);

		register uint8_t	of = getByte(pbr.a | pc.w++);

		eal = (dp.w + of + y.w + 0) & 0xffff;
		eah = (dp.w + of + y.w + 1) & 0xffff;

		return ((dp.l) ? 2 : 1);
	}

	static uint8_t am_dpgi(uint32_t &eal, uint32_t &eah)
	{
		BYTES(1);

		register uint8_t	of = getByte(pbr.a | pc.w++);

		register uint16_t	al = (dp.w + (of + 0));
		register uint16_t   ah = (dp.w + (of + 1));

		eal = dbr.a | getWord(al, ah);
		eah = eal + 1;
		return ((dp.l) ? 4 : 3);
	}

	static uint8_t am_dpix(uint32_t &eal, uint32_t &eah)
	{
		BYTES(1);

		register uint8_t	of = getByte(pbr.a | pc.w++);

		register uint16_t	al = (dp.w + (of + x.w + 0));
		register uint16_t   ah = (dp.w + (of + x.w + 1));

		eal = dbr.a | getWord(al, ah);
		eah = eal + 1;
		return ((dp.l) ? 4 : 3);
	}

	static uint8_t am_dpiy(uint32_t &eal, uint32_t &eah)
	{
		BYTES(1);

		register uint8_t	of = getByte(pbr.a | pc.w++);

		register uint16_t	al = (dp.w + (of + 0));
		register uint16_t   ah = (dp.w + (of + 1));

		eal = (dbr.a | getWord(al, ah)) + y.w;
		eah = eal + 1;
		return ((dp.l) ? 4 : 3);
	}

	static uint8_t am_dpil(uint32_t &eal, uint32_t &eah)
	{
		BYTES(1);

		register uint8_t	of = getByte(pbr.a | pc.w++);
		register uint16_t	al = (dp.w + ((of + 0) & 0xff));
		register uint16_t   ah = (dp.w + ((of + 1) & 0xff));
		register uint16_t   au = (dp.w + ((of + 2) & 0xff));

		eal = (au << 16) | (ah << 8) | al;
		eah = eal + 1;
		return (4);
	}

	static uint8_t am_dily(uint32_t &eal, uint32_t &eah)
	{
		BYTES(1);

		register uint8_t	of = getByte(pbr.a | pc.w++);
		register uint16_t	al = (dp.w + ((of + 0) & 0xff));
		register uint16_t   ah = (dp.w + ((of + 1) & 0xff));
		register uint16_t   au = (dp.w + ((of + 2) & 0xff));

		eal = ((au << 16) | (ah << 8) | al) + y.w;
		eah = eal + 1;
		return (4);
	}

	static uint8_t am_srel(uint32_t &eal, uint32_t &eah)
	{
		BYTES(1);

		register Word		ad;

		ad.w = sp.w;
		ad.w += getByte(pbr.a | pc.w++);
		eal = ad.w;
		eah = eal + 1;
		return (2);
	}

	static uint8_t am_sriy(uint32_t &eal, uint32_t &eah)
	{
		BYTES(1);

		register Word		ad;

		ad.w = sp.w;
		ad.w += getByte(pbr.a | pc.w++);

		register uint8_t	al = getByte(ad.w++);
		register uint8_t	ah = getByte(ad.w++);

		eal = (dbr.a | (ah << 8) | al) + y.w;
		eah = eal + 1;
		return (5);
	}

	static uint8_t op_bcc(uint32_t eal, uint32_t eah)
	{
		TRACE(bcc);

		if (p.c == 0) {
			pc.w = eal;
			return (3);
		}
		return (2);
	}

	static uint8_t op_bcs(uint32_t eal, uint32_t eah)
	{
		TRACE(bcs);

		if (p.c == 1) {
			pc.w = eal;
			return (3);
		}
		return (2);
	}

	static uint8_t op_beq(uint32_t eal, uint32_t eah)
	{
		TRACE(beq);

		if (p.z == 1) {
			pc.w = eal;
			return (3);
		}
		return (2);
	}

	static uint8_t op_bmi(uint32_t eal, uint32_t eah)
	{
		TRACE(bmi);

		if (p.n == 1) {
			pc.w = eal;
			return (3);
		}
		return (2);
	}

	static uint8_t op_bne(uint32_t eal, uint32_t eah)
	{
		TRACE(bne);

		if (p.z == 0) {
			pc.w = eal;
			return (3);
		}
		return (2);
	}

	static uint8_t op_bpl(uint32_t eal, uint32_t eah)
	{
		TRACE(bpl);

		if (p.n == 0) {
			pc.w = eal;
			return (3);
		}
		return (2);
	}

	static uint8_t op_bra(uint32_t eal, uint32_t eah)
	{
		TRACE(bra);

		pc.w = eal;
		return (3);
	}

	static uint8_t op_brl(uint32_t eal, uint32_t eah)
	{
		TRACE(brl);

		pc.w = eal;
		return (4);
	}

	static uint8_t op_brk(uint32_t eal, uint32_t eah)
	{
		TRACE(brk);

		pushByte(pbr.b);
		pushByte(pc.h);
		pushByte(pc.l);
		pushByte(p.f);
		p.i = 1;
		p.d = 0;

		pc.w = getWord(0xffe6, 0xffe7);
		pbr.b = 0;
		return (8);
	}

	static uint8_t op_bvc(uint32_t eal, uint32_t eah)
	{
		TRACE(bvc);

		if (p.v == 0) {
			pc.w = eal;
			return (3);
		}
		return (2);
	}

	static uint8_t op_bvs(uint32_t eal, uint32_t eah)
	{
		TRACE(bvs);

		if (p.v == 1) {
			pc.w = eal;
			return (3);
		}
		return (2);
	}

	static uint8_t op_cop(uint32_t eal, uint32_t eah)
	{
		TRACE(cop);

		pushByte(pbr.b);
		pushByte(pc.h);
		pushByte(pc.l);
		pushByte(p.f);
		p.i = 1;
		p.d = 0;

		pc.w = getWord(0xffe4, 0xffe5);
		pbr.b = 0;
		return (8);
	}

	static uint8_t op_jsl(uint32_t eal, uint32_t eah)
	{
		TRACE(jsl);

		--pc.w;
		pushByte(pbr.b);
		pushByte(pc.h);
		pushByte(pc.l);

		pbr.b = eal >> 16;
		pc.w = (uint16_t)eal;
		return (5);
	}

	static uint8_t op_jsr(uint32_t eal, uint32_t eah)
	{
		TRACE(jsr);

		--pc.w;
		pushByte(pc.h);
		pushByte(pc.l);

		pc.w = (uint16_t)eal;
		return (4);
	}

	static uint8_t op_mvn(uint32_t eal, uint32_t eah)
	{
		TRACE(mvn);

		register uint32_t	dst = getByte(eal) << 16;
		register uint32_t	src = getByte(eah) << 16;

		setByte(dbr.a = dst | y.w++, getByte(src | x.w++));
		if (c.w-- != 0x0000) pc.w -= 3;
		return (7);
	}

	static uint8_t op_mvp(uint32_t eal, uint32_t eah)
	{
		TRACE(mvp);

		register uint32_t	dst = getByte(eal) << 16;
		register uint32_t	src = getByte(eah) << 16;

		setByte(dbr.a = dst | y.w--, getByte(src | x.w--));
		if (c.w-- != 0x0000) pc.w -= 3;
		return (7);
	}

	static uint8_t op_pea(uint32_t eal, uint32_t eah)
	{
		TRACE(pea);

		pushByte(getByte(eah));
		pushByte(getByte(eal));
		return (2);
	}

	static uint8_t op_pei(uint32_t eal, uint32_t eah)
	{
		TRACE(pei);

		pushByte(getByte(eah));
		pushByte(getByte(eal));
		return (5);
	}

	static uint8_t op_per(uint32_t eal, uint32_t eah)
	{
		TRACE(per);

		pushByte(eal >> 8);
		pushByte(eal >> 0);
		return (5);
	}

	static uint8_t op_phb(uint32_t eal, uint32_t eah)
	{
		TRACE(phb);

		pushByte(pbr.b);
		return (3);
	}

	static uint8_t op_phd(uint32_t eal, uint32_t eah)
	{
		TRACE(phd);

		pushByte(dp.h);
		pushByte(dp.l);
		return (4);
	}

	static uint8_t op_phk(uint32_t eal, uint32_t eah)
	{
		TRACE(phk);

		pushByte(pbr.b);
		return (3);
	}

	static uint8_t op_php(uint32_t eal, uint32_t eah)
	{
		TRACE(php);

		pushByte(p.f);
		return (3);
	}

	static uint8_t op_plb(uint32_t eal, uint32_t eah)
	{
		TRACE(plb);

		dbr.b = pullByte();
		setnz_b(dbr.b);
		return (4);
	}

	static uint8_t op_pld(uint32_t eal, uint32_t eah)
	{
		TRACE(pld);

		dp.l = pullByte();
		dp.h = pullByte();
		setnz_w(dp.w);
		return (5);
	}

	static uint8_t op_plp(uint32_t eal, uint32_t eah)
	{
		TRACE(plp);

		p.f = pullByte();
		Emulator::setMode();
		return (4);
	}

	static uint8_t op_rep(uint32_t eal, uint32_t eah)
	{
		TRACE(rep);

		p.f &= ~getByte(eal);
		Emulator::setMode();
		return (3);
	}

	static uint8_t op_rts(uint32_t eal, uint32_t eah)
	{
		TRACE(rts);

		pc.l = pullByte();
		pc.h = pullByte();
		++pc.w;
		return (6);
	}

	static uint8_t op_rti(uint32_t eal, uint32_t eah)
	{
		TRACE(rti);

		p.f = pullByte();
		pc.l = pullByte();
		pc.h = pullByte();
		pbr.b = pullByte();
		Emulator::setMode();
		return (7);
	}

	static uint8_t op_rtl(uint32_t eal, uint32_t eah)
	{
		TRACE(rtl);

		pc.l = pullByte ();
		pc.h = pullByte();
		pbr.b = pullByte();
		++pc.w;
		return (6);
	}

	static uint8_t op_sep(uint32_t eal, uint32_t eah)
	{
		TRACE(sep);

		p.f |= getByte(eal);
		Emulator::setMode();
		return (3);
	}
};

//==============================================================================
// Opcodes Affected by M bit
//------------------------------------------------------------------------------

class ModeM0 : public Registers
{
private:
	ModeM0() { }

protected:
	static uint8_t am_immm(uint32_t &eal, uint32_t &eah)
	{
		BYTES(2);

		eal = pbr.b | pc.w++;
		eah = pbr.b | pc.w++;
		return (0);
	}

	static uint8_t op_adc(uint32_t eal, uint32_t eah)
	{
		TRACE(adc);

		register uint16_t	data = getWord(eal, eah);
		register uint32_t	temp = c.w + data + p.c;

		if (p.d) {
			if ((temp & 0x000f) > 0x0009) temp += 0x0006;
			if ((temp & 0x00f0) > 0x0090) temp += 0x0060;
			if ((temp & 0x0f00) > 0x0900) temp += 0x0600;
			if ((temp & 0xf000) > 0x9000) temp += 0x6000;
		}

		setc(temp & 0x10000);
		setv((~(c.w ^ data)) & (c.w ^ temp) & 0x8000);
		setnz_w(c.w = (uint16_t)temp);
		return (3);
	}

	static uint8_t op_and(uint32_t eal, uint32_t eah)
	{
		TRACE(and);

		setnz_w(c.w &= ModeN::getWord(eal, eah));
		return (3);
	}

	static uint8_t op_asl(uint32_t eal, uint32_t eah)
	{
		TRACE(asl);

		register uint16_t data = ModeN::getWord(eal, eah);

		setc(data & 0x8000);
		setnz_w(data <<= 1);
		ModeN::setWord(eal, eah, data);
		return (4);
	}

	static uint8_t op_asla(uint32_t eal, uint32_t eah)
	{
		TRACE(asl);

		setc(c.w & 0x8000);
		setnz_w(c.w <<= 1);
		return (2);
	}

	static uint8_t op_bit(uint32_t eal, uint32_t eah)
	{
		TRACE(bit);

		register uint16_t data = ModeN::getWord(eal, eah);

		setn(data & 0x8000);
		setv(data & 0x4000);
		setz(data & c.w);
		return (3);
	}

	static uint8_t op_biti(uint32_t eal, uint32_t eah)
	{
		TRACE(bit);

		register uint16_t data = ModeN::getWord(eal, eah);

		setz(data & c.w);
		return (3);
	}

	static uint8_t op_cmp(uint32_t eal, uint32_t eah)
	{
		TRACE(cmp);

		register uint16_t data = ModeN::getWord(eal, eah);
		register uint32_t diff = c.w - data;

		setnz_w((uint16_t)diff);
		setc(diff & 0x10000);
		return (2);
	}

	static uint8_t op_dec(uint32_t eal, uint32_t eah)
	{
		TRACE(dec);

		register uint16_t data = ModeN::getWord(eal, eah);

		setnz_w(--data);
		ModeN::setWord(eal, eah, data);
		return (4);
	}

	static uint8_t op_deca(uint32_t eal, uint32_t eah)
	{
		TRACE(dec);

		setnz_w(--c.w);
		return (2);
	}

	static uint8_t op_eor(uint32_t eal, uint32_t eah)
	{
		TRACE(eor);

		setnz_w(c.w ^= ModeN::getWord(eal, eah));
		return (3);
	}

	static uint8_t op_inc(uint32_t eal, uint32_t eah)
	{
		TRACE(inc);

		register uint16_t data = ModeN::getWord(eal, eah);

		setnz_w(++data);
		ModeN::setWord(eal, eah, data);
		return (4);
	}

	static uint8_t op_inca(uint32_t eal, uint32_t eah)
	{
		TRACE(dec);

		setnz_w(++c.w);
		return (2);
	}

	static uint8_t op_lda(uint32_t eal, uint32_t eah)
	{
		TRACE(lda);

		setnz_w(c.w = ModeN::getWord(eal, eah));
		return (4);
	}

	static uint8_t op_lsr(uint32_t eal, uint32_t eah)
	{
		TRACE(lsr);

		register uint16_t data = ModeN::getWord(eal, eah);

		setc(data & 0x0001);
		setnz_w(data >>= 1);
		ModeN::setWord(eal, eah, data);
		return (4);
	}

	static uint8_t op_lsra(uint32_t eal, uint32_t eah)
	{
		TRACE(lsr);

		setc(c.w & 0x0001);
		setnz_w(c.w >>= 1);
		return (2);
	}

	static uint8_t op_ora(uint32_t eal, uint32_t eah)
	{
		TRACE(ora);

		setnz_w(c.w |= ModeN::getWord(eal, eah));
		return (3);
	}

	static uint8_t op_pha(uint32_t eal, uint32_t eah)
	{
		TRACE(pha);

		ModeN::pushByte(c.h);
		ModeN::pushByte(c.l);
		return (5);
	}

	static uint8_t op_pla(uint32_t eal, uint32_t eah)
	{
		TRACE(pla);

		c.l = ModeN::pullByte();
		c.h = ModeN::pullByte();
		setnz_w(c.w);
		return (2);
	}

	static uint8_t op_rol(uint32_t eal, uint32_t eah)
	{
		TRACE(rol);

		register uint16_t data = ModeN::getWord(eal, eah);
		register uint16_t cin = p.c ? 0x0001 : 0x0000;

		setc(data & 0x8000);
		setnz_w(data = (data << 1) | cin);
		ModeN::setWord(eal, eah, data);
		return (4);
	}

	static uint8_t op_rola(uint32_t eal, uint32_t eah)
	{
		TRACE(rol);

		register uint16_t cin = p.c ? 0x0001 : 0x0000;

		setc(c.w & 0x8000);
		setnz_w(c.w = (c.w << 1) | cin);
		return (2);
	}

	static uint8_t op_ror(uint32_t eal, uint32_t eah)
	{
		TRACE(ror);

		register uint16_t data = ModeN::getWord(eal, eah);
		register uint16_t cin = p.c ? 0x8000 : 0x0000;

		setc(data & 0x0001);
		setnz_w(data = (data >> 1) | cin);
		ModeN::setWord(eal, eah, data);
		return (4);
	}

	static uint8_t op_rora(uint32_t eal, uint32_t eah)
	{
		TRACE(ror);

		register uint16_t cin = p.c ? 0x8000 : 0x0000;

		setc(c.w & 0x0001);
		setnz_w(c.w = (c.w >> 1) | cin);
		return (2);
	}

	static uint8_t op_sbc(uint32_t eal, uint32_t eah)
	{
		TRACE(sbc);

		register uint16_t	data = getWord(eal, eah);
		register uint32_t	temp = c.w + (data ^ 0xffff) + p.c;

		if (p.d) {
			if ((temp & 0x000f) > 0x0009) temp += 0x0006;
			if ((temp & 0x00f0) > 0x0090) temp += 0x0060;
			if ((temp & 0x0f00) > 0x0900) temp += 0x0600;
			if ((temp & 0xf000) > 0x9000) temp += 0x6000;
		}

		setc(temp & 0x10000);
		setv((~(c.w ^ data)) & (c.w ^ temp) & 0x8000);
		setnz_w(c.w = (uint16_t)temp);
		return (3);
	}

	static uint8_t op_sta(uint32_t eal, uint32_t eah)
	{
		TRACE(sta);

		ModeN::setWord(eal, eah, c.w);
		return (3);
	}

	static uint8_t op_stz(uint32_t eal, uint32_t eah)
	{
		TRACE(stz);

		ModeN::setWord(eal, eah, 0x0000);
		return (3);
	}

	static uint8_t op_trb(uint32_t eal, uint32_t eah)
	{
		TRACE(trb);

		register uint16_t data = ModeN::getWord(eal, eah);

		setz(data & c.w);
		ModeN::setWord(eal, eah, data & ~c.w);
		return (3);
	}

	static uint8_t op_tsb(uint32_t eal, uint32_t eah)
	{
		TRACE(tsb);

		register uint16_t data = ModeN::getWord(eal, eah);

		setz(data & c.w);
		ModeN::setWord(eal, eah, data | c.w);
		return (3);
	}

	static uint8_t op_txa(uint32_t eal, uint32_t eah)
	{
		TRACE(txa);

		setnz_w(c.w = x.w);
		return (2);
	}

	static uint8_t op_tya(uint32_t eal, uint32_t eah)
	{
		TRACE(tya);

		setnz_w(c.w = y.w);
		return (2);
	}
};

class ModeM1 : public Registers
{
private:
	ModeM1() { }

protected:
	static uint8_t am_immm(uint32_t &eal, uint32_t &eah)
	{
		BYTES(1);

		eal = pbr.b | pc.w++;
		eah = 0;
		return (0);
	}

	static uint8_t op_adc(uint32_t eal, uint32_t eah)
	{
		TRACE(adc);

		register uint8_t	data = getByte(eal);
		register uint16_t	temp = c.l + data + p.c;

		if (p.d) {
			if ((temp & 0x0f) > 0x09) temp += 0x06;
			if ((temp & 0xf0) > 0x90) temp += 0x60;
		}

		setc(temp & 0x100);
		setv((~(c.l ^ data)) & (c.l ^ temp) & 0x80);
		setnz_b(c.l = (uint8_t)temp);
		return (2);
	}

	static uint8_t op_and(uint32_t eal, uint32_t eah)
	{
		TRACE(and);

		setnz_b(c.l &= getByte(eal));
		return (2);
	}

	static uint8_t op_asl(uint32_t eal, uint32_t eah)
	{
		TRACE(asl);

		register uint8_t data = ModeN::getByte(eal);

		setc(data & 0x80);
		setnz_b(data <<= 1);
		setByte(eal, data);
		return (4);
	}

	static uint8_t op_asla(uint32_t eal, uint32_t eah)
	{
		TRACE(asl);

		setc(c.l & 0x80);
		setnz_b(c.l <<= 1);
		return (2);
	}

	static uint8_t op_bit(uint32_t eal, uint32_t eah)
	{
		TRACE(bit);

		register uint8_t data = ModeN::getByte(eal);

		setn(data & 0x80);
		setv(data & 0x40);
		setz(data & c.l);
		return (2);
	}

	static uint8_t op_biti(uint32_t eal, uint32_t eah)
	{
		TRACE(bit);

		register uint8_t data = ModeN::getByte(eal);

		setz(data & c.l);
		return (2);
	}

	static uint8_t op_cmp(uint32_t eal, uint32_t eah)
	{
		TRACE(cmp);

		register uint8_t  data = getByte(eal);
		register uint16_t diff = c.l - data;

		setnz_b((uint8_t) diff);
		setc(!(diff & 0x100));
		return (2);
	}

	static uint8_t op_dec(uint32_t eal, uint32_t eah)
	{
		TRACE(dec);

		register uint8_t data = ModeN::getByte(eal);

		setnz_b(--data);
		ModeN::setByte(eal, data);
		return (4);
	}

	static uint8_t op_deca(uint32_t eal, uint32_t eah)
	{
		TRACE(dec);

		setnz_b(--c.l);
		return (2);
	}

	static uint8_t op_eor(uint32_t eal, uint32_t eah)
	{
		TRACE(eor);

		setnz_b(c.l ^= getByte(eal));
		return (2);
	}

	static uint8_t op_inc(uint32_t eal, uint32_t eah)
	{
		TRACE(inc);

		register uint8_t data = ModeN::getByte(eal);

		setnz_b(++data);
		ModeN::setByte(eal, data);
		return (4);
	}

	static uint8_t op_inca(uint32_t eal, uint32_t eah)
	{
		TRACE(inc);

		setnz_b(++c.l);
		return (2);
	}

	static uint8_t op_lda(uint32_t eal, uint32_t eah)
	{
		TRACE(lda);

		setnz_b(c.l = getByte(eal));
		return (2);
	}

	static uint8_t op_lsr(uint32_t eal, uint32_t eah)
	{
		TRACE(lsr);

		register uint8_t data = ModeN::getByte(eal);

		setc(data & 0x01);
		setnz_b(data >>= 1);
		ModeN::setByte(eal, data);
		return (2);
	}

	static uint8_t op_lsra(uint32_t eal, uint32_t eah)
	{
		TRACE(lsr);

		setc(c.l & 0x01);
		setnz_b(c.l >>= 1);
		return (2);
	}

	static uint8_t op_ora(uint32_t eal, uint32_t eah)
	{
		TRACE(ora);

		setnz_b(c.l |= getByte(eal));
		return (2);
	}

	static uint8_t op_pha(uint32_t eal, uint32_t eah)
	{
		TRACE(pha);

		ModeN::pushByte(c.l);
		return (2);
	}

	static uint8_t op_pla(uint32_t eal, uint32_t eah)
	{
		TRACE(pla);

		c.l = ModeN::pullByte();
		return (2);
	}

	static uint8_t op_rol(uint32_t eal, uint32_t eah)
	{
		TRACE(rol);

		register uint8_t data = ModeN::getByte(eal);
		register uint8_t cin = p.c ? 0x01 : 0x00;

		setc(data & 0x80);
		setnz_b(data = (data << 1) | cin);
		ModeN::setByte(eal, data);
		return (2);
	}

	static uint8_t op_rola(uint32_t eal, uint32_t eah)
	{
		TRACE(rol);

		register uint8_t cin = p.c ? 0x01 : 0x00;

		setc(c.l & 0x80);
		setnz_b(c.l = (c.l << 1) | cin);
		return (2);
	}

	static uint8_t op_ror(uint32_t eal, uint32_t eah)
	{
		TRACE(roe);

		register uint8_t data = ModeN::getByte(eal);
		register uint8_t cin = p.c ? 0x80 : 0x00;

		setc(data & 0x01);
		setnz_b(data = (data >> 1) | cin);
		ModeN::setByte(eal, data);
		return (2);
	}

	static uint8_t op_rora(uint32_t eal, uint32_t eah)
	{
		TRACE(ror);

		register uint8_t cin = p.c ? 0x80 : 0x00;

		setc(c.l & 0x01);
		setnz_b(c.l = (c.l >> 1) | cin);
		return (2);
	}

	static uint8_t op_sbc(uint32_t eal, uint32_t eah)
	{
		TRACE(sbc);

		register uint8_t	data = getByte(eal);
		register uint16_t	temp = c.l + (data ^ 0xff) + p.c;

		if (p.d) {
			if ((temp & 0x0f) > 0x09) temp += 0x06;
			if ((temp & 0xf0) > 0x90) temp += 0x60;
		}

		setc(temp & 0x100);
		setv((~(c.l ^ data)) & (c.l ^ temp) & 0x80);
		setnz_b(c.l = (uint8_t)temp);
		return (2);
	}

	static uint8_t op_sta(uint32_t eal, uint32_t eah)
	{
		TRACE(sta);

		ModeN::setByte(eal, c.l);
		return (2);
	}

	static uint8_t op_stz(uint32_t eal, uint32_t eah)
	{
		TRACE(stz);

		ModeN::setByte(eal, 0x00);
		return (2);
	}

	static uint8_t op_trb(uint32_t eal, uint32_t eah)
	{
		TRACE(trb);

		register uint8_t data = ModeN::getByte(eal);

		setz(data & c.l);
		ModeN::setByte(eal, data & ~c.l);
		return (2);
	}

	static uint8_t op_tsb(uint32_t eal, uint32_t eah)
	{
		TRACE(tsb);

		register uint8_t data = ModeN::getByte(eal);

		setz(data & c.l);
		ModeN::setByte(eal, data | c.l);
		return (2);
	}

	static uint8_t op_txa(uint32_t eal, uint32_t eah)
	{
		TRACE(txa);

		setnz_b(c.l = x.l);
		return (2);
	}

	static uint8_t op_tya(uint32_t eal, uint32_t eah)
	{
		TRACE(tya);

		setnz_b(c.l = y.l);
		return (2);
	}
};

//==============================================================================
// Opcodes Affected by X bit
//------------------------------------------------------------------------------

class ModeX0 : public Registers
{
private:
	ModeX0() { }

protected:
	static uint8_t am_immx(uint32_t &eal, uint32_t &eah)
	{
		BYTES(2);

		eal = pbr.b | pc.w++;
		eah = pbr.b | pc.w++;
		return (0);
	}

	static uint8_t op_cpx(uint32_t eal, uint32_t eah)
	{
		TRACE(cpx);

		register uint8_t  data = ModeN::getByte(eal);
		register uint16_t diff = x.l - data;

		setnz_b((uint8_t) diff);
		setc(diff & 0x0100);
		return (2);
	}

	static uint8_t op_cpy(uint32_t eal, uint32_t eah)
	{
		TRACE(cpy);

		register uint8_t  data = ModeN::getByte(eal);
		register uint16_t diff = y.l - data;

		setnz_b((uint8_t) diff);
		setc(diff & 0x0100);
		return (2);
	}

	static uint8_t op_dex(uint32_t eal, uint32_t eah)
	{
		TRACE(dex);

		setnz_b(--x.l);
		return (2);
	}

	static uint8_t op_dey(uint32_t eal, uint32_t eah)
	{
		TRACE(dey);

		setnz_b(--y.l);
		return (2);
	}

	static uint8_t op_inx(uint32_t eal, uint32_t eah)
	{
		TRACE(inx);

		setnz_b(++x.l);
		return (2);
	}

	static uint8_t op_iny(uint32_t eal, uint32_t eah)
	{
		TRACE(iny);

		setnz_b(++y.l);
		return (2);
	}

	static uint8_t op_ldx(uint32_t eal, uint32_t eah)
	{
		TRACE(ldx);

		setnz_b(x.l = ModeN::getByte(eal));
		return (3);
	}

	static uint8_t op_ldy(uint32_t eal, uint32_t eah)
	{
		TRACE(ldy);

		setnz_b(y.l = ModeN::getByte(eal));
		return (4);
	}

	static uint8_t op_phx(uint32_t eal, uint32_t eah)
	{
		TRACE(phx);

		ModeN::pushByte(x.l);
		return (4);
	}

	static uint8_t op_phy(uint32_t eal, uint32_t eah)
	{
		TRACE(phy);

		ModeN::pushByte(y.l);
		return (4);
	}

	static uint8_t op_plx(uint32_t eal, uint32_t eah)
	{
		TRACE(plx);

		x.l = ModeN::pullByte();
		setnz_b(x.l);
		return (4);
	}

	static uint8_t op_ply(uint32_t eal, uint32_t eah)
	{
		TRACE(ply);

		y.l = ModeN::pullByte();
		setnz_b(y.l);
		return (4);
	}

	static uint8_t op_stx(uint32_t eal, uint32_t eah)
	{
		TRACE(stx);

		ModeN::setByte(eal, x.l);
		return (2);
	}

	static uint8_t op_sty(uint32_t eal, uint32_t eah)
	{
		TRACE(sty);

		ModeN::setByte(eal, y.l);
		return (2);
	}

	static uint8_t op_tax(uint32_t eal, uint32_t eah)
	{
		TRACE(tax);

		setnz_b(x.l = c.l);
		return (2);
	}

	static uint8_t op_tay(uint32_t eal, uint32_t eah)
	{
		TRACE(tay);

		setnz_b(y.l = c.l);
		return (2);
	}

	static uint8_t op_tsx(uint32_t eal, uint32_t eah)
	{
		TRACE(tsx);

		setnz_b(x.l = sp.l);
		return (2);
	}

	static uint8_t op_txs(uint32_t eal, uint32_t eah)
	{
		TRACE(txs);

		sp.w = x.l;
		return (2);
	}

	static uint8_t op_txy(uint32_t eal, uint32_t eah)
	{
		TRACE(txy);

		setnz_b(y.l = x.l);
		return (2);
	}

	static uint8_t op_tyx(uint32_t eal, uint32_t eah)
	{
		TRACE(tyx);

		setnz_b(x.l = y.l);
		return (2);
	}
};

class ModeX1 : public Registers
{
private:
	ModeX1() { }

protected:
	static uint8_t am_immx(uint32_t &eal, uint32_t &eah)
	{
		BYTES(1);

		eal = pbr.b | pc.w++;
		eah = 0;
		return (0);
	}

	static uint8_t op_cpx(uint32_t eal, uint32_t eah)
	{
		TRACE(cpx);

		register uint16_t data = ModeN::getWord(eal, eah);
		register uint32_t diff = x.w - data;

		setnz_w(diff);
		setc(diff & 0x010000);
		return (2);
	}

	static uint8_t op_cpy(uint32_t eal, uint32_t eah)
	{
		TRACE(cpy);

		register uint16_t data = ModeN::getWord(eal, eah);
		register uint32_t diff = y.w - data;

		setnz_w(diff);
		setc(diff & 0x10000);
		return (2);
	}

	static uint8_t op_dex(uint32_t eal, uint32_t eah)
	{
		TRACE(dex);

		setnz_w(--x.w);
		return (2);
	}

	static uint8_t op_dey(uint32_t eal, uint32_t eah)
	{
		TRACE(dey);

		setnz_w(--y.w);
		return (2);
	}

	static uint8_t op_inx(uint32_t eal, uint32_t eah)
	{
		TRACE(inx);

		setnz_w(++x.w);
		return (2);
	}

	static uint8_t op_iny(uint32_t eal, uint32_t eah)
	{
		TRACE(iny);

		setnz_w(++y.w);
		return (2);
	}

	static uint8_t op_ldx(uint32_t eal, uint32_t eah)
	{
		TRACE(ldx);

		setnz_w(x.w = ModeN::getWord(eal, eah));
		return (4);
	}

	static uint8_t op_ldy(uint32_t eal, uint32_t eah)
	{
		TRACE(ldy);

		setnz_w(y.w = ModeN::getWord(eal, eah));
		return (4);
	}

	static uint8_t op_phx(uint32_t eal, uint32_t eah)
	{
		TRACE(phx);

		ModeN::pushByte(x.h);
		ModeN::pushByte(x.l);
		return (5);
	}

	static uint8_t op_phy(uint32_t eal, uint32_t eah)
	{
		TRACE(phy);

		ModeN::pushByte(y.h);
		ModeN::pushByte(y.l);
		return (5);
	}

	static uint8_t op_plx(uint32_t eal, uint32_t eah)
	{
		TRACE(plx);

		x.l = ModeN::pullByte();
		x.h = ModeN::pullByte();
		setnz_w(x.w);
		return (5);
	}

	static uint8_t op_ply(uint32_t eal, uint32_t eah)
	{
		TRACE(ply);

		y.l = ModeN::pullByte();
		y.h = ModeN::pullByte();
		setnz_w(y.w);
		return (5);
	}

	static uint8_t op_stx(uint32_t eal, uint32_t eah)
	{
		TRACE(stx);

		ModeN::setWord(eal, eah, x.w);
		return (3);
	}

	static uint8_t op_sty(uint32_t eal, uint32_t eah)
	{
		TRACE(sty);

		ModeN::setWord(eal, eah, y.w);
		return (3);
	}

	static uint8_t op_tax(uint32_t eal, uint32_t eah)
	{
		TRACE(tax);

		setnz_w(x.w = p.m ? c.w : c.l);
		return (2);
	}

	static uint8_t op_tay(uint32_t eal, uint32_t eah)
	{
		TRACE(tay);

		setnz_w(y.w = p.m ? c.w : c.l);
		return (2);
	}

	static uint8_t op_tsx(uint32_t eal, uint32_t eah)
	{
		TRACE(tsx);

		setnz_w(x.w = sp.w);
		return (2);
	}

	static uint8_t op_txs(uint32_t eal, uint32_t eah)
	{
		TRACE(txs);

		sp.w = x.w;
		return (2);
	}

	static uint8_t op_txy(uint32_t eal, uint32_t eah)
	{
		TRACE(txy);

		setnz_w(y.w = x.w);
		return (2);
	}

	static uint8_t op_tyx(uint32_t eal, uint32_t eah)
	{
		TRACE(tyx);

		setnz_w(x.w = y.w);
		return (2);
	}
};

//==============================================================================
// Opcode Sets
//------------------------------------------------------------------------------

#define OPCODE(HX,AM,OP,AD) \
	static uint8_t op_##HX (void) \
	{ \
		register uint32_t	eal,eah; \
		register uint8_t	cycles = AM (eal, eah) + AD; \
		return (cycles + OP(eal, eah)); \
	}

#define ALL_OPCODES \
	OPCODE(00, am_immb, op_brk, 0) \
	OPCODE(01, am_dpix, op_ora, 0) \
	OPCODE(02, am_immb, op_cop, 0) \
	OPCODE(03, am_srel, op_ora, 0) \
	OPCODE(04, am_dpag, op_tsb, 0) \
	OPCODE(05, am_dpag, op_ora, 0) \
	OPCODE(06, am_dpag, op_asl, 0) \
	OPCODE(07, am_dpil, op_ora, 0) \
	OPCODE(08, am_impl, op_php, 0) \
	OPCODE(09, am_immm, op_ora, 0) \
	OPCODE(0a, am_impl, op_asla, 0) \
	OPCODE(0b, am_impl, op_phd, 0) \
	OPCODE(0c, am_absl, op_tsb, 0) \
	OPCODE(0d, am_absl, op_ora, 0) \
	OPCODE(0e, am_absl, op_asl, 0) \
	OPCODE(0f, am_alng, op_ora, 0) \
	OPCODE(10, am_rela, op_bpl, 0) \
	OPCODE(11, am_dpiy, op_ora, 0) \
	OPCODE(12, am_dpgi, op_ora, 0) \
	OPCODE(13, am_sriy, op_ora, 0) \
	OPCODE(14, am_dpag, op_trb, 0) \
	OPCODE(15, am_dpgx, op_ora, 0) \
	OPCODE(16, am_dpgx, op_asl, 0) \
	OPCODE(17, am_dily, op_ora, 0) \
	OPCODE(18, am_impl, op_clc, 0) \
	OPCODE(19, am_absy, op_ora, 0) \
	OPCODE(1a, am_impl, op_inca, 0) \
	OPCODE(1b, am_impl, op_tcs, 0) \
	OPCODE(1c, am_absl, op_trb, 0) \
	OPCODE(1d, am_absx, op_ora, 0) \
	OPCODE(1e, am_absx, op_asl, 0) \
	OPCODE(1f, am_alnx, op_ora, 0) \
	OPCODE(20, am_absp, op_jsr, 0) \
	OPCODE(21, am_dpix, op_and, 0) \
	OPCODE(22, am_alng, op_jsl, 0) \
	OPCODE(23, am_srel, op_and, 0) \
	OPCODE(24, am_dpag, op_bit, 0) \
	OPCODE(25, am_dpag, op_and, 0) \
	OPCODE(26, am_dpag, op_rol, 0) \
	OPCODE(27, am_dpil, op_and, 0) \
	OPCODE(28, am_impl, op_plp, 0) \
	OPCODE(29, am_immm, op_and, 0) \
	OPCODE(2a, am_impl, op_rola, 0) \
	OPCODE(2b, am_impl, op_pld, 0) \
	OPCODE(2c, am_absl, op_bit, 0) \
	OPCODE(2d, am_absl, op_and, 0) \
	OPCODE(2e, am_absl, op_rol, 0) \
	OPCODE(2f, am_alng, op_and, 0) \
	OPCODE(30, am_rela, op_bmi, 0) \
	OPCODE(31, am_dpiy, op_and, 0) \
	OPCODE(32, am_dpgi, op_and, 0) \
	OPCODE(33, am_sriy, op_and, 0) \
	OPCODE(34, am_dpgx, op_bit, 0) \
	OPCODE(35, am_dpgx, op_and, 0) \
	OPCODE(36, am_dpgx, op_rol, 0) \
	OPCODE(37, am_dily, op_and, 0) \
	OPCODE(38, am_impl, op_sec, 0) \
	OPCODE(39, am_absy, op_and, 0) \
	OPCODE(3a, am_impl, op_deca, 0) \
	OPCODE(3b, am_impl, op_tsc, 0) \
	OPCODE(3c, am_absx, op_bit, 0) \
	OPCODE(3d, am_absx, op_and, 0) \
	OPCODE(3e, am_absx, op_rol, 0) \
	OPCODE(3f, am_alnx, op_and, 0) \
	OPCODE(40, am_impl, op_rti, 0) \
	OPCODE(41, am_dpix, op_eor, 0) \
	OPCODE(42, am_immb, op_wdm, 0) \
	OPCODE(43, am_srel, op_eor, 0) \
	OPCODE(44, am_immw, op_mvp, 0) \
	OPCODE(45, am_dpag, op_eor, 0) \
	OPCODE(46, am_dpag, op_lsr, 0) \
	OPCODE(47, am_dpil, op_eor, 0) \
	OPCODE(48, am_impl, op_pha, 0) \
	OPCODE(49, am_immm, op_eor, 0) \
	OPCODE(4a, am_impl, op_lsra, 0) \
	OPCODE(4b, am_impl, op_phk, 0) \
	OPCODE(4c, am_absp, op_jmp, 0) \
	OPCODE(4d, am_absl, op_eor, 0) \
	OPCODE(4e, am_absl, op_lsr, 0) \
	OPCODE(4f, am_alng, op_eor, 0) \
	OPCODE(50, am_rela, op_bvc, 0) \
	OPCODE(51, am_dpiy, op_eor, 0) \
	OPCODE(52, am_dpgi, op_eor, 0) \
	OPCODE(53, am_sriy, op_eor, 0) \
	OPCODE(54, am_immw, op_mvn, 0) \
	OPCODE(55, am_dpgx, op_eor, 0) \
	OPCODE(56, am_dpgx, op_lsr, 0) \
	OPCODE(57, am_dpil, op_eor, 0) \
	OPCODE(58, am_impl, op_cli, 0) \
	OPCODE(59, am_absy, op_eor, 0) \
	OPCODE(5a, am_impl, op_phy, 0) \
	OPCODE(5b, am_impl, op_tcd, 0) \
	OPCODE(5c, am_alng, op_jmp, 0) \
	OPCODE(5d, am_absx, op_eor, 0) \
	OPCODE(5e, am_absx, op_lsr, 0) \
	OPCODE(5f, am_alnx, op_eor, 0) \
	OPCODE(60, am_impl, op_rts, 0) \
	OPCODE(61, am_dpix, op_adc, 0) \
	OPCODE(62, am_lrel, op_per, 0) \
	OPCODE(63, am_srel, op_adc, 0) \
	OPCODE(64, am_dpag, op_stz, 0) \
	OPCODE(65, am_dpag, op_adc, 0) \
	OPCODE(66, am_dpag, op_ror, 0) \
	OPCODE(67, am_dpil, op_adc, 0) \
	OPCODE(68, am_impl, op_pla, 0) \
	OPCODE(69, am_immm, op_adc, 0) \
	OPCODE(6a, am_impl, op_rora, 0) \
	OPCODE(6b, am_impl, op_rtl, 0) \
	OPCODE(6c, am_absi, op_jmp, 0) \
	OPCODE(6d, am_absl, op_adc, 0) \
	OPCODE(6e, am_absl, op_ror, 0) \
	OPCODE(6f, am_alng, op_adc, 0) \
	OPCODE(70, am_rela, op_bvs, 0) \
	OPCODE(71, am_dpiy, op_adc, 0) \
	OPCODE(72, am_dpgi, op_adc, 0) \
	OPCODE(73, am_sriy, op_adc, 0) \
	OPCODE(74, am_dpgx, op_stz, 0) \
	OPCODE(75, am_dpgx, op_adc, 0) \
	OPCODE(76, am_dpgx, op_ror, 0) \
	OPCODE(77, am_dily, op_adc, 0) \
	OPCODE(78, am_impl, op_sei, 0) \
	OPCODE(79, am_absy, op_adc, 0) \
	OPCODE(7a, am_impl, op_ply, 0) \
	OPCODE(7b, am_impl, op_tdc, 0) \
	OPCODE(7c, am_abxi, op_jmp, 0) \
	OPCODE(7d, am_absx, op_adc, 0) \
	OPCODE(7e, am_absx, op_ror, 0) \
	OPCODE(7f, am_alnx, op_adc, 0) \
	OPCODE(80, am_rela, op_bra, 0) \
	OPCODE(81, am_dpix, op_sta, 0) \
	OPCODE(82, am_lrel, op_brl, 0) \
	OPCODE(83, am_srel, op_sta, 0) \
	OPCODE(84, am_dpag, op_sty, 0) \
	OPCODE(85, am_dpag, op_sta, 0) \
	OPCODE(86, am_dpag, op_stx, 0) \
	OPCODE(87, am_dpil, op_sta, 0) \
	OPCODE(88, am_impl, op_dey, 0) \
	OPCODE(89, am_immm, op_biti, 0) \
	OPCODE(8a, am_impl, op_txa, 0) \
	OPCODE(8b, am_impl, op_phb, 0) \
	OPCODE(8c, am_absl, op_sty, 0) \
	OPCODE(8d, am_absl, op_sta, 0) \
	OPCODE(8e, am_absl, op_stx, 0) \
	OPCODE(8f, am_alng, op_sta, 0) \
	OPCODE(90, am_rela, op_bcc, 0) \
	OPCODE(91, am_dpiy, op_sta, 0) \
	OPCODE(92, am_dpgi, op_sta, 0) \
	OPCODE(93, am_sriy, op_sta, 0) \
	OPCODE(94, am_dpgx, op_sty, 0) \
	OPCODE(95, am_dpgx, op_sta, 0) \
	OPCODE(96, am_dpgy, op_stx, 0) \
	OPCODE(97, am_dily, op_sta, 0) \
	OPCODE(98, am_impl, op_tya, 0) \
	OPCODE(99, am_absy, op_sta, 0) \
	OPCODE(9a, am_impl, op_txs, 0) \
	OPCODE(9b, am_impl, op_txy, 0) \
	OPCODE(9c, am_absl, op_stz, 0) \
	OPCODE(9d, am_absx, op_sta, 0) \
	OPCODE(9e, am_absx, op_stz, 0) \
	OPCODE(9f, am_alnx, op_sta, 0) \
	OPCODE(a0, am_immx, op_ldy, 0) \
	OPCODE(a1, am_dpix, op_lda, 0) \
	OPCODE(a2, am_immx, op_ldx, 0) \
	OPCODE(a3, am_srel, op_lda, 0) \
	OPCODE(a4, am_dpag, op_ldy, 0) \
	OPCODE(a5, am_dpag, op_lda, 0) \
	OPCODE(a6, am_dpag, op_ldx, 0) \
	OPCODE(a7, am_dpil, op_lda, 0) \
	OPCODE(a8, am_impl, op_tay, 0) \
	OPCODE(a9, am_immm, op_lda, 0) \
	OPCODE(aa, am_impl, op_tax, 0) \
	OPCODE(ab, am_impl, op_plb, 0) \
	OPCODE(ac, am_absl, op_ldy, 0) \
	OPCODE(ad, am_absl, op_lda, 0) \
	OPCODE(ae, am_absl, op_ldx, 0) \
	OPCODE(af, am_alng, op_lda, 0) \
	OPCODE(b0, am_rela, op_bcs, 0) \
	OPCODE(b1, am_dpiy, op_lda, 0) \
	OPCODE(b2, am_dpgi, op_lda, 0) \
	OPCODE(b3, am_sriy, op_lda, 0) \
	OPCODE(b4, am_dpgx, op_ldy, 0) \
	OPCODE(b5, am_dpgx, op_lda, 0) \
	OPCODE(b6, am_dpgy, op_ldx, 0) \
	OPCODE(b7, am_dpil, op_lda, 0) \
	OPCODE(b8, am_impl, op_clv, 0) \
	OPCODE(b9, am_absy, op_lda, 0) \
	OPCODE(ba, am_impl, op_tsx, 0) \
	OPCODE(bb, am_impl, op_tyx, 0) \
	OPCODE(bc, am_absx, op_ldy, 0) \
	OPCODE(bd, am_absx, op_lda, 0) \
	OPCODE(be, am_absy, op_ldx, 0) \
	OPCODE(bf, am_alnx, op_lda, 0) \
	OPCODE(c0, am_immx, op_cpy, 0) \
	OPCODE(c1, am_dpix, op_cmp, 0) \
	OPCODE(c2, am_immb, op_rep, 0) \
	OPCODE(c3, am_srel, op_cmp, 0) \
	OPCODE(c4, am_dpag, op_cpy, 0) \
	OPCODE(c5, am_dpag, op_cmp, 0) \
	OPCODE(c6, am_dpag, op_dec, 0) \
	OPCODE(c7, am_dpil, op_cmp, 0) \
	OPCODE(c8, am_impl, op_iny, 0) \
	OPCODE(c9, am_immm, op_cmp, 0) \
	OPCODE(ca, am_impl, op_dex, 0) \
	OPCODE(cb, am_impl, op_wai, 0) \
	OPCODE(cc, am_absl, op_cpy, 0) \
	OPCODE(cd, am_absl, op_cmp, 0) \
	OPCODE(ce, am_absl, op_dec, 0) \
	OPCODE(cf, am_alng, op_cmp, 0) \
	OPCODE(d0, am_rela, op_bne, 0) \
	OPCODE(d1, am_dpiy, op_cmp, 0) \
	OPCODE(d2, am_dpgi, op_cmp, 0) \
	OPCODE(d3, am_sriy, op_cmp, 0) \
	OPCODE(d4, am_dpgi, op_pei, 0) \
	OPCODE(d5, am_dpgx, op_cmp, 0) \
	OPCODE(d6, am_dpgx, op_dec, 0) \
	OPCODE(d7, am_dily, op_cmp, 0) \
	OPCODE(d8, am_impl, op_cld, 0) \
	OPCODE(d9, am_absy, op_cmp, 0) \
	OPCODE(da, am_impl, op_phx, 0) \
	OPCODE(db, am_impl, op_stp, 0) \
	OPCODE(dc, am_abil, op_jmp, 0) \
	OPCODE(dd, am_absx, op_cmp, 0) \
	OPCODE(de, am_absx, op_dec, 0) \
	OPCODE(df, am_alnx, op_cmp, 0) \
	OPCODE(e0, am_immx, op_cpx, 0) \
	OPCODE(e1, am_dpix, op_sbc, 0) \
	OPCODE(e2, am_immb, op_sep, 0) \
	OPCODE(e3, am_srel, op_sbc, 0) \
	OPCODE(e4, am_dpag, op_cpx, 0) \
	OPCODE(e5, am_dpag, op_sbc, 0) \
	OPCODE(e6, am_dpag, op_inc, 0) \
	OPCODE(e7, am_dpil, op_sbc, 0) \
	OPCODE(e8, am_impl, op_inx, 0) \
	OPCODE(e9, am_immm, op_sbc, 0) \
	OPCODE(ea, am_impl, op_nop, 0) \
	OPCODE(eb, am_impl, op_xba, 0) \
	OPCODE(ec, am_absl, op_cpx, 0) \
	OPCODE(ed, am_absl, op_sbc, 0) \
	OPCODE(ee, am_absl, op_inc, 0) \
	OPCODE(ef, am_alng, op_sbc, 0) \
	OPCODE(f0, am_rela, op_beq, 0) \
	OPCODE(f1, am_dpiy, op_sbc, 0) \
	OPCODE(f2, am_dpgi, op_sbc, 0) \
	OPCODE(f3, am_sriy, op_sbc, 0) \
	OPCODE(f4, am_immw, op_pea, 0) \
	OPCODE(f5, am_dpgx, op_sbc, 0) \
	OPCODE(f6, am_dpgx, op_inc, 0) \
	OPCODE(f7, am_dily, op_sbc, 0) \
	OPCODE(f8, am_impl, op_sed, 0) \
	OPCODE(f9, am_absy, op_sbc, 0) \
	OPCODE(fa, am_impl, op_plx, 0) \
	OPCODE(fb, am_impl, op_xce, 0) \
	OPCODE(fc, am_abxi, op_jsr, 0) \
	OPCODE(fd, am_absx, op_sbc, 0) \
	OPCODE(fe, am_absx, op_inc, 0) \
	OPCODE(ff, am_alnx, op_sbc, 0)

//==============================================================================

class CpuModeE11 : public Common, ModeE
{
	friend class Emulator;
private:
	static const OpcodeSet	opcodeSet;

protected:
	ALL_OPCODES
};

class CpuModeN00 : public Common, ModeN, ModeM0, ModeX0
{
	friend class Emulator;
private:
	static const OpcodeSet	opcodeSet;

protected:
	ALL_OPCODES
};

class CpuModeN01 : public Common, ModeN, ModeM0, ModeX1
{
	friend class Emulator;
private:
	static const OpcodeSet	opcodeSet;

protected:
	ALL_OPCODES
};

class CpuModeN10 : public Common, ModeN, ModeM1, ModeX0
{
	friend class Emulator;
private:
	static const OpcodeSet	opcodeSet;

protected:
	ALL_OPCODES
};

class CpuModeN11 : public Common, ModeN, ModeM1, ModeX1
{
	friend class Emulator;
private:
	static const OpcodeSet	opcodeSet;

protected:
	ALL_OPCODES
};

#endif
