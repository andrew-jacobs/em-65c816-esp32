
#include "Emulator.h"

bool Trace::enabled = false;

const char *Trace::toHex(uint32_t value, uint16_t digits)
{
	static char buffer[32];
	char *pBuffer = &(buffer[32]);

	*--pBuffer = '\0';
	while (digits--) {
		*--pBuffer = "0123456789abcdef"[value & 0x0f];
		value >>= 4;
	}
	return (pBuffer);
}

void Trace::start(void)
{
	if (enabled) {
		cout << toHex(pbr.b, 2) << ':';
		cout << toHex(pc.w, 4) << ' ';
		cout << toHex(Memory::getByte(pbr.a | pc.w), 2) << ' ';
	}
}

void Trace::bytes(uint16_t count)
{
	if (enabled) {
		cout << ((count >= 1) ? toHex(Memory::getByte(pbr.a | (pc.w + 0)), 2) : "  ") << ' ';
		cout << ((count >= 2) ? toHex(Memory::getByte(pbr.a | (pc.w + 1)), 2) : "  ") << ' ';
		cout << ((count >= 3) ? toHex(Memory::getByte(pbr.a | (pc.w + 2)), 2) : "  ") << ' ';
	}
}

void Trace::trace(const char *pOpcode, uint32_t eal, uint32_t eah)
{
	if (enabled) {
		cout << pOpcode << " {";
		cout << toHex(eal >> 16, 2) << ':';
		cout << toHex(eal, 4) << ',';
		cout << toHex(eah >> 16, 2) << ':';
		cout << toHex(eah, 4) << "} ";

		cout << "E=" << (e ? '1' : '0') << ' ';

		cout << "P="
			<< (p.n ? 'N' : '.')
			<< (p.v ? 'V' : '.')
			<< (p.m ? 'M' : '.')
			<< (p.x ? 'X' : '.')
			<< (p.d ? 'D' : '.')
			<< (p.i ? 'I' : '.')
			<< (p.z ? 'Z' : '.')
			<< (p.c ? 'C' : '.') << ' ';

		cout << "C=";
		if (e || p.m) {
			cout << toHex(c.h, 2) << '[';
			cout << toHex(c.l, 2) << "] ";
		}
		else
			cout << '[' << toHex(c.w, 4) << "] ";

		cout << "X=";
		if (e || p.x) {
			cout << toHex(x.h, 2) << '[';
			cout << toHex(x.l, 2) << "] ";
		}
		else
			cout << '[' << toHex(x.w, 4) << "] ";

		cout << "Y=";
		if (e || p.x) {
			cout << toHex(y.h, 2) << '[';
			cout << toHex(y.l, 2) << "] ";
		}
		else
			cout << '[' << toHex(y.w, 4) << "] ";

		cout << "DP=" << toHex(dp.w, 4) << ' ';

		cout << "SP=";
		if (e) {
			cout << toHex(sp.h, 2) << '[';
			cout << toHex(sp.l, 2) << "] {";

			Word xp = sp;

			++xp.l; cout << toHex(Memory::getByte(xp.w), 2) << ' ';
			++xp.l; cout << toHex(Memory::getByte(xp.w), 2) << ' ';
			++xp.l; cout << toHex(Memory::getByte(xp.w), 2) << ' ';
			++xp.l; cout << toHex(Memory::getByte(xp.w), 2) << "} ";
		}
		else {
			cout << '[' << toHex(sp.w, 4) << "] {";

			cout << toHex(Memory::getByte(sp.w + 1), 2) << ' ';
			cout << toHex(Memory::getByte(sp.w + 2), 2) << ' ';
			cout << toHex(Memory::getByte(sp.w + 3), 2) << ' ';
			cout << toHex(Memory::getByte(sp.w + 4), 2) << "} ";
		}

		cout << "DBR=" << toHex(dbr.b, 2) << ' ';
	}
}

void Trace::cycles(uint16_t cycles)
{
	if (enabled) {
		cout << "CY=" << cycles << endl;
	}
}
