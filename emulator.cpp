
#include <Arduino.h>

#include "emulator.h"

//==============================================================================

Emulator::Emulator (Memory &memory)
    : mem(memory), trace (true)
{ }

void Emulator::reset ()
{
    pc.w = getWord (0xfffc, 0xfffd);
    dp.w = 0x0000;
    sp.h = 0x01;
    pbr.a = 0x000000;
    dbr.a = 0x000000;
    p.m = 1;
    p.x = 1;
    p.i = 1;
    p.d = 0;
    e = true;

    cycles = 0;
}

uint16_t Emulator::step (void)
{
    if (trace) Serial.printf ("%.2x:%.4x %.2x ", pbr.b, pc.w, mem.getByte (pbr.a | pc.w));

    switch (mem.getByte (pbr.a | pc.w++)) {
    case 0x00:  return (am_imm8 (&Emulator::op_brk));       // BRK #
    case 0x01:  return (am_dpix (&Emulator::op_ora));       // ORA (dpg,X)
    case 0x02:  return (am_imm8 (&Emulator::op_cop));       // COP #
    case 0x03:  return (am_srel (&Emulator::op_ora));       // ORA off,S
    case 0x04:  return (am_dpag (&Emulator::op_tsb));       // TSB dpg
    case 0x05:  return (am_dpag (&Emulator::op_ora));       // ORA dpg
    case 0x06:  return (am_dpag (&Emulator::op_asl));       // ASL dpg
    case 0x07:  return (am_dpil (&Emulator::op_ora));       // ora [dpg]
    case 0x08:  return (am_impl (&Emulator::op_php));       // PHP
    case 0x09:  return (am_immm (&Emulator::op_ora));       // ORA #
    case 0x0a:  return (am_acc (&Emulator::op_asla));       // ASL A
    case 0x0b:  return (am_impl (&Emulator::op_phd));       // PHD
    case 0x0c:  return (am_absl (&Emulator::op_tsb));       // TSB abs
    case 0x0d:  return (am_absl (&Emulator::op_ora));       // ORA abs
    case 0x0e:  return (am_absl (&Emulator::op_asl));       // ASL abs
    case 0x0f:  return (am_alng (&Emulator::op_ora));       // ORA [lng]

    case 0x10:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x11:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x12:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x13:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x14:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x15:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x16:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x17:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x18:  return (am_impl (&Emulator::op_clc));       // CLC
    case 0x19:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x1a:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x1b:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x1c:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x1d:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x1e:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x1f:  return (am_xxxx (&Emulator::op_xxx));       //

    case 0x20:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x21:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x22:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x23:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x24:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x25:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x26:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x27:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x28:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x29:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x2a:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x2b:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x2c:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x2d:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x2e:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x2f:  return (am_xxxx (&Emulator::op_xxx));       //

    case 0x30:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x31:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x32:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x33:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x34:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x35:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x36:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x37:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x38:  return (am_impl (&Emulator::op_sec));       // SEC
    case 0x39:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x3a:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x3b:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x3c:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x3d:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x3e:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x3f:  return (am_xxxx (&Emulator::op_xxx));       //

    case 0x40:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x41:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x42:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x43:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x44:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x45:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x46:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x47:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x48:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x49:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x4a:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x4b:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x4c:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x4d:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x4e:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x4f:  return (am_xxxx (&Emulator::op_xxx));       //

    case 0x50:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x51:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x52:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x53:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x54:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x55:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x56:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x57:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x58:  return (am_impl (&Emulator::op_cli));       // CLI
    case 0x59:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x5a:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x5b:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x5c:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x5d:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x5e:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x5f:  return (am_xxxx (&Emulator::op_xxx));       //

    case 0x60:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x61:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x62:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x63:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x64:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x65:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x66:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x67:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x68:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x69:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x6a:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x6b:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x6c:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x6d:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x6e:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x6f:  return (am_xxxx (&Emulator::op_xxx));       //

    case 0x70:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x71:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x72:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x73:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x74:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x75:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x76:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x77:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x78:  return (am_impl (&Emulator::op_sei));       // SEI
    case 0x79:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x7a:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x7b:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x7c:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x7d:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x7e:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x7f:  return (am_xxxx (&Emulator::op_xxx));       //

    case 0x80:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x81:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x82:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x83:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x84:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x85:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x86:  return (am_dpag (&Emulator::op_stx));       // STX dpg
    case 0x87:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x88:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x89:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x8a:  return (am_impl (&Emulator::op_txa));       // TXA
    case 0x8b:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x8c:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x8d:  return (am_absl (&Emulator::op_sta));       // STA abs
    case 0x8e:  return (am_absl (&Emulator::op_stx));       // STX abs
    case 0x8f:  return (am_xxxx (&Emulator::op_xxx));       //

    case 0x90:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x91:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x92:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x93:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x94:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x95:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x96:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x97:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x98:  return (am_impl (&Emulator::op_tya));       // TYA
    case 0x99:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x9a:  return (am_impl (&Emulator::op_txs));       // TXS
    case 0x9b:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x9c:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x9d:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x9e:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0x9f:  return (am_xxxx (&Emulator::op_xxx));       //

    case 0xa0:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xa1:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xa2:  return (am_immx (&Emulator::op_ldx));       // LDX #
    case 0xa3:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xa4:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xa5:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xa6:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xa7:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xa8:  return (am_impl (&Emulator::op_tay));       // TAY
    case 0xa9:  return (am_immm (&Emulator::op_lda));       // LDA #
    case 0xaa:  return (am_impl (&Emulator::op_tax));       // TAX
    case 0xab:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xac:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xad:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xae:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xaf:  return (am_xxxx (&Emulator::op_xxx));       //

    case 0xb0:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xb1:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xb2:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xb3:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xb4:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xb5:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xb6:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xb7:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xb8:  return (am_impl (&Emulator::op_clv));       // CLV
    case 0xb9:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xba:  return (am_impl (&Emulator::op_tsx));       // TSX
    case 0xbb:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xbc:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xbd:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xbe:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xbf:  return (am_xxxx (&Emulator::op_xxx));       //

    case 0xc0:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xc1:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xc2:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xc3:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xc4:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xc5:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xc6:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xc7:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xc8:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xc9:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xca:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xcb:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xcc:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xcd:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xce:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xcf:  return (am_xxxx (&Emulator::op_xxx));       //

    case 0xd0:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xd1:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xd2:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xd3:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xd4:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xd5:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xd6:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xd7:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xd8:  return (am_impl (&Emulator::op_cld));       // CLD
    case 0xd9:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xda:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xdb:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xdc:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xdd:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xde:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xdf:  return (am_xxxx (&Emulator::op_xxx));       //

    case 0xe0:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xe1:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xe2:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xe3:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xe4:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xe5:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xe6:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xe7:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xe8:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xe9:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xea:  return (am_impl (&Emulator::op_nop));       // NOP
    case 0xeb:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xec:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xed:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xee:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xef:  return (am_xxxx (&Emulator::op_xxx));       //

    case 0xf0:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xf1:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xf2:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xf3:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xf4:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xf5:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xf6:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xf7:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xf8:  return (am_impl (&Emulator::op_sed));       // SED
    case 0xf9:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xfa:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xfb:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xfc:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xfd:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xfe:  return (am_xxxx (&Emulator::op_xxx));       //
    case 0xff:  return (am_xxxx (&Emulator::op_xxx));       //
    }

    delay (5000);
    return (0);
}

void Emulator::bytes (uint16_t n)
{
    Serial.printf (((n > 0) ? "%.2x " : "   "), mem.getByte (pbr.a | ((uint16_t) pc.w + 0)));
    Serial.printf (((n > 1) ? "%.2x " : "   "), mem.getByte (pbr.a | ((uint16_t) pc.w + 1)));
    Serial.printf (((n > 2) ? "%.2x " : "   "), mem.getByte (pbr.a | ((uint16_t) pc.w + 2)));
} 

void Emulator::dump (const char *pMnem, uint32_t eal, uint32_t eah)
{
    Serial.printf ("%s {%.2x:%.4x,%.2x:%.4x} ", pMnem, (eal >> 16), eal, (eah >> 16), eah);

    Serial.printf ("E=%c ", e ? '1' : '0');
    Serial.printf ("P=%c%c%c%c%c%c%c%c ",
        p.n ? 'N' : '.', p.v ? 'V' : '.', p.m ? 'M' : '.', p.x ? 'X' : '.',
        p.d ? 'D' : '.', p.i ? 'I' : '.', p.z ? 'Z' : '.', p.c ? 'C' : '.');

    Serial.printf ((e || p.m) ? "A=%.2x[%.2x] " : "A=[%.2x%.2x] ", c.h, c.l);
    Serial.printf ((e || p.x) ? "X=%.2x[%.2x] " : "X=[%.2x%.2x] ", x.h, x.l);
    Serial.printf ((e || p.x) ? "Y=%.2x[%.2x] " : "Y=[%.2x%.2x] ", y.h, y.l);

    Serial.printf ("DP=%.4x ", dp.w);
    Serial.printf (e ? "SP=%.2x[%.2x] " : "SP=[%.2x%.2x] ", sp.h, sp.l);

    Serial.printf ("{ %.2x %.2x %.2x %.2x } ",
        getByte (((uint16_t) sp.w + 1)), getByte (((uint16_t) sp.w + 2)),
        getByte (((uint16_t) sp.w + 3)), getByte (((uint16_t) sp.w + 4)));

    Serial.printf ("DBR=%.2x ", dbr.b);
}