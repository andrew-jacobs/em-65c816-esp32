
#ifndef EMULATOR_H
#define EMULATOR_H

#include "memory.h"

//==============================================================================

union Word {
    struct {
        uint8_t             l;
        uint8_t             h;
    };
    uint16_t            w;
};

union Address {
    struct {
        uint16_t            z;          // Always zero
        uint8_t             b;
    };
    uint32_t            a;
};

union Flags {
    struct {
        uint8_t             c : 1;
        uint8_t             z : 1;
        uint8_t             i : 1;
        uint8_t             d : 1;
        uint8_t             x : 1;
        uint8_t             m : 1;
        uint8_t             v : 1;
        uint8_t             n : 1;
    };
    uint8_t             b;
};


//==============================================================================

class Emulator {
private:
    Memory         &mem;                // Memory configuration

    Word            pc;
    Word            sp;
    Word            dp;
    Word            c;
    Word            x;
    Word            y;
    Flags           p;
    Address         pbr;
    Address         dbr;
    bool            e;

    uint16_t        ier;                // Interrupt Enable Flags
    uint16_t        ifr;                // Interrupt Flags
    uint32_t        ticks;              // Last timer value

    bool            trace;
    uint32_t        start;

    static uint16_t join (uint8_t l, uint8_t h);

    // Memory Access
    uint8_t getByte (uint32_t a);
    uint16_t getWord (uint32_t l, uint32_t h);
    uint32_t getAddr (uint32_t l, uint32_t h, uint32_t u);
    void setByte (uint32_t a, uint8_t b);
    void setWord (uint32_t l, uint32_t h, uint16_t w);

    // Stack Access
    void pushByte (uint8_t b);
    void pushWord (uint16_t w);
    uint8_t pullByte (void);
    uint16_t pullWord (void);

    // Flag Access
    void setn (bool f);
    void setv (bool f);
    void setd (bool f);
    void seti (bool f);
    void setz (bool f);
    void setc (bool f);
    void setnz_b (uint8_t v);
    void setnz_w (uint16_t v);

    // Addressing modes
    uint16_t am_absl (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h));   // abs
    uint16_t am_absx (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h));   // abs,X
    uint16_t am_absy (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h));   // abs,Y
    uint16_t am_absi (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h));   // (abs)
    uint16_t am_abxi (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h));   // (abs,X)
    uint16_t am_alng (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h));   // >lng
    uint16_t am_alnx (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h));   // >lng,X
    uint16_t am_abil (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h));   // [lng]
    uint16_t am_dpag (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h));   // dpg
    uint16_t am_dpgx (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h));   // dpg,X
    uint16_t am_dpgy (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h));   // dpg,Y
    uint16_t am_dpgi (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h));   // (dpg)
    uint16_t am_dpix (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h));   // (dpg,X)
    uint16_t am_dpiy (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h));   // (dpg),Y
    uint16_t am_dpil (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h));   // [dpg]
    uint16_t am_dily (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h));   // [dpg],Y
    uint16_t am_immb (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h));   // #
    uint16_t am_immw (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h));   // #
    uint16_t am_immm (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h));   // #
    uint16_t am_immx (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h));   // #
    uint16_t am_acc  (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h));   // A
    uint16_t am_impl (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h));   //
    uint16_t am_srel (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h));   // off,S
    uint16_t am_sriy (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h));   // (off,S),Y⌈
    uint16_t am_rela (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h));   // rel
    uint16_t am_lrel (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h));   // rel

    // Opcodes
    uint16_t op_adc (uint32_t l, uint32_t);
    uint16_t op_and (uint32_t l, uint32_t);
    uint16_t op_asl (uint32_t l, uint32_t);
    uint16_t op_asla (uint32_t l, uint32_t);
    uint16_t op_bcc (uint32_t l, uint32_t);
    uint16_t op_bcs (uint32_t l, uint32_t);
    uint16_t op_beq (uint32_t l, uint32_t);
    uint16_t op_bit (uint32_t l, uint32_t);
    uint16_t op_biti (uint32_t l, uint32_t);
    uint16_t op_bmi (uint32_t l, uint32_t);
    uint16_t op_bne (uint32_t l, uint32_t);
    uint16_t op_bpl (uint32_t l, uint32_t);
    uint16_t op_bra (uint32_t l, uint32_t);
    uint16_t op_brl (uint32_t l, uint32_t);
    uint16_t op_brk (uint32_t l, uint32_t);
    uint16_t op_bvc (uint32_t l, uint32_t);
    uint16_t op_bvs (uint32_t l, uint32_t);
    uint16_t op_clc (uint32_t l, uint32_t);
    uint16_t op_cld (uint32_t l, uint32_t);
    uint16_t op_cli (uint32_t l, uint32_t);
    uint16_t op_clv (uint32_t l, uint32_t);
    uint16_t op_cmp (uint32_t l, uint32_t);
    uint16_t op_cop (uint32_t l, uint32_t);
    uint16_t op_cpx (uint32_t l, uint32_t);
    uint16_t op_cpy (uint32_t l, uint32_t);
    uint16_t op_dec (uint32_t l, uint32_t);
    uint16_t op_deca (uint32_t l, uint32_t);
    uint16_t op_dex (uint32_t l, uint32_t);
    uint16_t op_dey (uint32_t l, uint32_t);
    uint16_t op_eor (uint32_t l, uint32_t);
    uint16_t op_inc (uint32_t l, uint32_t);
    uint16_t op_inca (uint32_t l, uint32_t);
    uint16_t op_inx (uint32_t l, uint32_t);
    uint16_t op_iny (uint32_t l, uint32_t);
    uint16_t op_jml (uint32_t l, uint32_t);
    uint16_t op_jmp (uint32_t l, uint32_t);
    uint16_t op_jsl (uint32_t l, uint32_t);
    uint16_t op_jsr (uint32_t l, uint32_t);
    uint16_t op_lda (uint32_t l, uint32_t);
    uint16_t op_ldx (uint32_t l, uint32_t);
    uint16_t op_ldy (uint32_t l, uint32_t);
    uint16_t op_lsr (uint32_t l, uint32_t);
    uint16_t op_lsra (uint32_t l, uint32_t);
    uint16_t op_mvn (uint32_t l, uint32_t);
    uint16_t op_mvp (uint32_t l, uint32_t);
    uint16_t op_nop (uint32_t l, uint32_t);
    uint16_t op_ora (uint32_t l, uint32_t);
    uint16_t op_pea (uint32_t l, uint32_t);
    uint16_t op_pei (uint32_t l, uint32_t);
    uint16_t op_per (uint32_t l, uint32_t);
    uint16_t op_pha (uint32_t l, uint32_t);
    uint16_t op_phb (uint32_t l, uint32_t);
    uint16_t op_phd (uint32_t l, uint32_t);
    uint16_t op_phk (uint32_t l, uint32_t);
    uint16_t op_php (uint32_t l, uint32_t);
    uint16_t op_phx (uint32_t l, uint32_t);
    uint16_t op_phy (uint32_t l, uint32_t);
    uint16_t op_pla (uint32_t l, uint32_t);
    uint16_t op_plb (uint32_t l, uint32_t);
    uint16_t op_pld (uint32_t l, uint32_t);
    uint16_t op_plp (uint32_t l, uint32_t);
    uint16_t op_plx (uint32_t l, uint32_t);
    uint16_t op_ply (uint32_t l, uint32_t);
    uint16_t op_rep (uint32_t l, uint32_t);
    uint16_t op_rol (uint32_t l, uint32_t);
    uint16_t op_rola (uint32_t l, uint32_t);
    uint16_t op_ror (uint32_t l, uint32_t);
    uint16_t op_rora (uint32_t l, uint32_t);
    uint16_t op_rti (uint32_t l, uint32_t);
    uint16_t op_rtl (uint32_t l, uint32_t);
    uint16_t op_rts (uint32_t l, uint32_t);
    uint16_t op_sbc (uint32_t l, uint32_t);
    uint16_t op_sec (uint32_t l, uint32_t);
    uint16_t op_sed (uint32_t l, uint32_t);
    uint16_t op_sei (uint32_t l, uint32_t);
    uint16_t op_sep (uint32_t l, uint32_t);
    uint16_t op_sta (uint32_t l, uint32_t);
    uint16_t op_stp (uint32_t l, uint32_t);
    uint16_t op_stx (uint32_t l, uint32_t);
    uint16_t op_sty (uint32_t l, uint32_t);
    uint16_t op_stz (uint32_t l, uint32_t);
    uint16_t op_tax (uint32_t l, uint32_t);
    uint16_t op_tay (uint32_t l, uint32_t);
    uint16_t op_tcd (uint32_t l, uint32_t);
    uint16_t op_tcs (uint32_t l, uint32_t);
    uint16_t op_tdc (uint32_t l, uint32_t);
    uint16_t op_trb (uint32_t l, uint32_t);
    uint16_t op_tsb (uint32_t l, uint32_t);
    uint16_t op_tsc (uint32_t l, uint32_t);
    uint16_t op_tsx (uint32_t l, uint32_t);
    uint16_t op_txa (uint32_t l, uint32_t);
    uint16_t op_txs (uint32_t l, uint32_t);
    uint16_t op_txy (uint32_t l, uint32_t);
    uint16_t op_tya (uint32_t l, uint32_t);
    uint16_t op_tyx (uint32_t l, uint32_t);
    uint16_t op_wai (uint32_t l, uint32_t);
    uint16_t op_wdm (uint32_t l, uint32_t);
    uint16_t op_xba (uint32_t l, uint32_t);
    uint16_t op_xce (uint32_t l, uint32_t);

    // Trace utilities
    void bytes (uint16_t n);
    void dump (const char *pMnem, uint32_t eal, uint32_t eah);

public:
    uint32_t        cycles;

    Emulator (Memory &memory);

    void reset (void);
    uint16_t step (void);
};

//------------------------------------------------------------------------------

// Join two bytes to form a word.
inline uint16_t Emulator::join (uint8_t l, uint8_t h)
{
    return (l | (h << 8));
}

//------------------------------------------------------------------------------

inline uint8_t Emulator::getByte (uint32_t a)
{
    return (mem.getByte (a));
}

inline uint16_t Emulator::getWord (uint32_t l, uint32_t h)
{
    return (mem.getByte (l) | (mem.getByte (h) << 8));
}

inline uint32_t Emulator::getAddr (uint32_t l, uint32_t h, uint32_t u)
{
    return (mem.getByte (l) | (mem.getByte (h) << 8) | (mem.getByte (u) << 16));
}

inline void Emulator::setByte (uint32_t a, uint8_t b)
{
    mem.setByte (a, b);
}

inline void Emulator::setWord (uint32_t l, uint32_t h, uint16_t w)
{
    mem.setByte (l, w >> 0);
    mem.setByte (h, w >> 8);
}

//------------------------------------------------------------------------------

inline void Emulator::pushByte (uint8_t b)
{
    setByte (sp.w, b);

    if (e)
        --sp.l;
    else
        --sp.w; 
}

inline void Emulator::pushWord (uint16_t w)
{
    pushByte (w >> 8);
    pushByte (w >> 0);
}

inline uint8_t Emulator::pullByte (void)
{
    if (e)
        ++sp.l;
    else
        ++sp.w;

    return (getByte (sp.w));
}

inline uint16_t Emulator::pullWord (void)
{
    register uint8_t    l = pullByte ();
    register uint8_t    h = pullByte ();

    return (join (l, h));
}

//------------------------------------------------------------------------------

inline void Emulator::setn (bool f)
{
    p.n = f ? 1 : 0;
}

inline void Emulator::setv (bool f)
{
    p.v = f ? 1 : 0;
}

inline void Emulator::setd (bool f)
{
    p.d = f ? 1 : 0;
}

inline void Emulator::seti (bool f)
{
    p.i = f ? 1 : 0;
}

inline void Emulator::setz (bool f)
{
    p.z = f ? 1 : 0;
}

inline void Emulator::setc (bool f)
{
    p.c = f ? 1 : 0;
}

inline void Emulator::setnz_b (uint8_t v)
{
    setn (v &  0x80);
    setz (v == 0x00);
}

inline void Emulator::setnz_w (uint16_t v)
{
    setn (v &  0x8000);
    setz (v == 0x0000);
}

//------------------------------------------------------------------------------

// Absolute
inline uint16_t Emulator::am_absl (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h))
{
    if (trace) bytes (2);

    register uint8_t    al = mem.getByte (pbr.a | pc.w++);
    register uint8_t    ah = mem.getByte (pbr.a | pc.w++);
    register uint32_t   ea = dbr.a | join (al, ah);

    return ((*this.*pOp)(ea + 0, ea + 1) + 2);
}

// Absolute Indexed X
inline uint16_t Emulator::am_absx (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h))
{
    if (trace) bytes (2);

    register uint8_t    al = mem.getByte (pbr.a | pc.w++);
    register uint8_t    ah = mem.getByte (pbr.a | pc.w++);
    register uint32_t   ea = dbr.a | join (al, ah) + x.w;

    return ((*this.*pOp)(ea + 0, ea + 1) + 2);
}

// Absolute Indexed Y
inline uint16_t Emulator::am_absy (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h))
{
    if (trace) bytes (2);

    register uint8_t    al = mem.getByte (pbr.a | pc.w++);
    register uint8_t    ah = mem.getByte (pbr.a | pc.w++);
    register uint32_t   ea = dbr.a | join (al, ah) + y.w;

    return ((*this.*pOp)(ea + 0, ea + 1) + 2);
}

// Absolute Indirect (JMP (ind) only)
inline uint16_t Emulator::am_absi (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h))
{
    if (trace) bytes (2);

    register uint8_t    al = mem.getByte (pbr.a | pc.w++);
    register uint8_t    ah = mem.getByte (pbr.a | pc.w++);
    register uint16_t   ea = join (al, ah);

    al = getByte (ea++);
    ah = getByte (ea);      
    ea = join (al, ah);     // Always Bank 0

    register uint32_t   eal = pbr.a | ea++;
    register uint32_t   eah = pbr.a | ea;   

    return ((*this.*pOp)(eal, eah));
}

// Absolulte Indirect Indexed X (JMP/JSR (ind,X) only)
inline uint16_t Emulator::am_abxi (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h))
{
    if (trace) bytes (2);

    register uint8_t    al = mem.getByte (pbr.a | pc.w++);
    register uint8_t    ah = mem.getByte (pbr.a | pc.w++);
    register uint16_t   ea = join (al, ah) + x.w;

    al = getByte (pbr.a | ea++);
    ah = getByte (pbr.a | ea);
    ea = join (al, ah);

    register uint32_t   eal = pbr.a | ea++;
    register uint32_t   eah = pbr.a | ea;   

    return ((*this.*pOp)(eal, eah));
}

// Absolute Long
inline uint16_t Emulator::am_alng (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h))
{
    if (trace) bytes (3);

    register uint8_t    al = mem.getByte (pbr.a | pc.w++);
    register uint8_t    ah = mem.getByte (pbr.a | pc.w++);
    register uint8_t    au = mem.getByte (pbr.a | pc.w++);
    register uint32_t   ea = (au << 16) | join (al, ah);

    return ((*this.*pOp)(ea + 0, ea + 1) + 3);
}

// Absolute Long Indexed X
inline uint16_t Emulator::am_alnx (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h))
{
    if (trace) bytes (3);

    register uint8_t    al = mem.getByte (pbr.a | pc.w++);
    register uint8_t    ah = mem.getByte (pbr.a | pc.w++);
    register uint8_t    au = mem.getByte (pbr.a | pc.w++);
    register uint32_t   ea = (au << 16) | join (al, ah) + x.w;

    return ((*this.*pOp)(ea + 0, ea + 1));
}

// Absolute Indirect Long
inline uint16_t Emulator::am_abil (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h))
{
    if (trace) bytes (2);

    register uint8_t    al = mem.getByte (pbr.a | pc.w++);
    register uint8_t    ah = mem.getByte (pbr.a | pc.w++);
    register uint32_t   ea = join (al, ah);

    al = getByte (ea++);
    ah = getByte (ea++);
    ea = (getByte (ea) << 16) | join (al, ah);

    return ((*this.*pOp)(ea + 0, ea + 1));
}

// Direct Page
inline uint16_t Emulator::am_dpag (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h))
{
    if (trace) bytes (1);

    register uint8_t    al = mem.getByte (pbr.a | pc.w++);
    register uint16_t   eal = dp.w + ((al + 0) & (e ? 0x00ff : 0xffff));
    register uint16_t   eah = dp.w + ((al + 1) & (e ? 0x00ff : 0xffff));

    return ((*this.*pOp)(eal, eah) +  dp.l ? 2 : 1);
}

// Direct Page Indexed X
inline uint16_t Emulator::am_dpgx (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h))
{
    if (trace) bytes (1);

    register uint8_t    al = mem.getByte (pbr.a | pc.w++);
    register uint16_t   eal = dp.w + ((al + x.w + 0) & (e ? 0x00ff : 0xffff));
    register uint16_t   eah = dp.w + ((al + x.w + 1) & (e ? 0x00ff : 0xffff));

    return ((*this.*pOp)(eal, eah) +  dp.l ? 2 : 1);
}

// Direct Page Indexed Y
inline uint16_t Emulator::am_dpgy (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h))
{
    if (trace) bytes (1);

    register uint8_t    al = mem.getByte (pbr.a | pc.w++);
    register uint16_t   eal = dp.w + ((al + y.w + 0) & (e ? 0x00ff : 0xffff));
    register uint16_t   eah = dp.w + ((al + y.w + 1) & (e ? 0x00ff : 0xffff));

    return ((*this.*pOp)(eal, eah) +  dp.l ? 2 : 1);
}

// Direct Page Indirect
inline uint16_t Emulator::am_dpgi (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h))
{
    if (trace) bytes (1);

    register uint8_t    al = mem.getByte (pbr.a | pc.w++);
    register uint8_t    ah;
    register uint32_t   eal = dp.w + ((al + 0) & (e ? 0x00ff : 0xffff));
    register uint32_t   eah = dp.w + ((al + 1) & (e ? 0x00ff : 0xffff));
    
    al = getByte (eal);
    ah = getByte (eah);
    eal = dbr.a | getWord (al, ah);
    eah = eal + 1;

    return ((*this.*pOp)(eal, eah) +  dp.l ? 4 : 3);
}

// Direct Page Indexed X Indirect
inline uint16_t Emulator::am_dpix (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h))
{
    if (trace) bytes (1);

    register uint8_t    al = mem.getByte (pbr.a | pc.w++);
    register uint8_t    ah;
    register uint32_t   eal = dp.w + ((al + x.w + 0) & (e ? 0x00ff : 0xffff));
    register uint32_t   eah = dp.w + ((al + x.w + 1) & (e ? 0x00ff : 0xffff));
    
    al = getByte (eal);
    ah = getByte (eah);
    eal = dbr.a | getWord (al, ah);
    eah = eal + 1;

    return ((*this.*pOp)(eal, eah) +  dp.l ? 4 : 3);
}

// Direct Page Indirect Indexed Y
inline uint16_t Emulator::am_dpiy (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h))
{
    if (trace) bytes (1);

    register uint8_t    al = mem.getByte (pbr.a | pc.w++);
    register uint8_t    ah;
    register uint32_t   eal = dp.w + ((al + 0) & (e ? 0x00ff : 0xffff));
    register uint32_t   eah = dp.w + ((al + 1) & (e ? 0x00ff : 0xffff));
    
    al = getByte (eal);
    ah = getByte (eah);
    eal =(dbr.a | getWord (al, ah)) + y.w;
    eah = eal + 1;

    return ((*this.*pOp)(eal, eah) +  dp.l ? 4 : 3); 
}

// Direct Page Indirect Long
inline uint16_t Emulator::am_dpil (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h))
{
    if (trace) bytes (1);

    register uint8_t    al = mem.getByte (pbr.a | pc.w++);
    register uint8_t    ah;
    register uint32_t   eal = dp.w + ((al + 0) & (e ? 0x00ff : 0xffff));
    register uint32_t   eah = dp.w + ((al + 1) & (e ? 0x00ff : 0xffff));
    register uint32_t   eau = dp.w + ((al + 2) & (e ? 0x00ff : 0xffff));

    al = getByte (eal);
    ah = getByte (eah);
    eal =(getByte (eau) << 16) | getWord (al, ah);
    eah = eal + 1;

    return ((*this.*pOp)(eal, eah) +  dp.l ? 5 : 4);
}

// Direct Page Indirect Long Indexed Y
inline uint16_t Emulator::am_dily (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h))
{
    if (trace) bytes (1);

    register uint8_t    al = mem.getByte (pbr.a | pc.w++);
    register uint8_t    ah;
    register uint32_t   eal = dp.w + ((al + 0) & (e ? 0x00ff : 0xffff));
    register uint32_t   eah = dp.w + ((al + 1) & (e ? 0x00ff : 0xffff));
    register uint32_t   eau = dp.w + ((al + 1) & (e ? 0x00ff : 0xffff));

    al = getByte (eal);
    ah = getByte (eah);
    eal = ((getByte (eau) << 16) | getWord (al, ah)) + y.w;
    eah = eal + 1;

    return ((*this.*pOp)(eal, eah) +  dp.l ? 5 : 4);
}

// Immediate Byte
inline uint16_t Emulator::am_immb (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h))
{
    if (trace) bytes (1);

    register uint32_t   eal = pbr.a | pc.w++;

    return ((*this.*pOp)(eal, 0));
}

// Immediate Word
inline uint16_t Emulator::am_immw (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h))
{
    if (trace) bytes (2);

    register uint32_t   eal = pbr.a | pc.w++;
    register uint32_t   eah = pbr.a | pc.w++;

    return ((*this.*pOp)(eal, eah));
}

// Immediate Byte/Word (determined by M)
inline uint16_t Emulator::am_immm (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h))
{
    register uint32_t   eal;
    register uint32_t   eah;

    if (e || p.m) {
        if (trace) bytes (1);

        eal = pbr.a | pc.w++;
        eah = 0;

        return ((*this.*pOp)(eal, eah));
    }
    else {
        if (trace) bytes (2);

        eal = pbr.a | pc.w++;
        eah = pbr.a | pc.w++;

        return ((*this.*pOp)(eal, eah));
    }
}

// Immediate Byte/Word (determined by X)
inline uint16_t Emulator::am_immx (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h))
{
    register uint32_t   eal;
    register uint32_t   eah;

    if (e || p.x) {
        if (trace) bytes (1);

        eal = pbr.a | pc.w++;
        eah = 0;

        return ((*this.*pOp)(eal, eah));
    }
    else {
        if (trace) bytes (2);

        eal = pbr.a | pc.w++;
        eah = pbr.a | pc.w++;

        return ((*this.*pOp)(eal, eah));
    }
}

// Implied
inline uint16_t Emulator::am_impl (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h))
{
    if (trace) bytes (0);

    return ((*this.*pOp)(0, 0));
}

// Stack Relative
inline uint16_t Emulator::am_srel (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h))
{
    if (trace) bytes (1);

    register uint8_t    al = mem.getByte (pbr.a | pc.w++);
    register uint32_t   eal = sp.w + ((al + 0) & (e ? 0x00ff : 0xffff));
    register uint32_t   eah = sp.w + ((al + 1) & (e ? 0x00ff : 0xffff));

    return ((*this.*pOp)(eal, eah) + 1);
}

// Stack Relative Indirect Indexed Y
inline uint16_t Emulator::am_sriy (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h))
{
    if (trace) bytes (1);

    register uint8_t    al = mem.getByte (pbr.a | pc.w++);
    register uint8_t    ah;
    register uint32_t   eal = dp.w + ((al + 0) & (e ? 0x00ff : 0xffff));
    register uint32_t   eah = dp.w + ((al + 1) & (e ? 0x00ff : 0xffff));

    al = getByte (eal);
    ah = getByte (eah);
    eal = (dbr.a | join (al, ah)) + y.w;
    eah = eal + 1;

    return ((*this.*pOp)(eal, eah) + 3);
}

// Relative
inline uint16_t Emulator::am_rela (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h))
{
    if (trace) bytes (1);

    register uint8_t    ol = getByte (pbr.a | pc.w++);
    register uint16_t   ea = pc.w + (int8_t) ol;

    return ((*this.*pOp)(pbr.a | ea, 0) + 0);
}

// Long Relative
inline uint16_t Emulator::am_lrel (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h))
{
    if (trace) bytes (2);

    register uint8_t    ol = (int8_t) getByte (pbr.a | pc.w++);
    register uint8_t    oh = (int8_t) getByte (pbr.a | pc.w++);
    register uint16_t   ea = pc.w + (int16_t) join (ol, oh);

    return ((*this.*pOp)(pbr.a | ea, 0) + 1);
}

// Accumulator
inline uint16_t Emulator::am_acc (uint16_t (Emulator::*pOp)(uint32_t l, uint32_t h))
{
    if (trace) bytes (1);

    return ((*this.*pOp)(0, 0));
}

//------------------------------------------------------------------------------

inline uint16_t Emulator::op_adc (uint32_t l, uint32_t h)
{
    if (trace) dump ("ADC", l, h);

    if (e || p.m) {
        register uint8_t    data = getByte (l);
        register uint16_t   temp = c.l + data + p.c;

        if(p.d) {
            if ((temp & 0x0f) > 0x09) temp += 0x06;
            if ((temp & 0xf0) > 0x90) temp += 0x60;
        }

        setc (temp & 0x100);
        setv ((~(c.l ^ data)) & (c.l ^ temp) & 0x80);
        setnz_b (c.l = temp);
        return (2);
    }
    else {
        register uint16_t   data = getWord (l, h);
        register uint32_t   temp = c.w + data + p.c;

        if(p.d) {
            if ((temp & 0x000f) > 0x0009) temp += 0x0006;
            if ((temp & 0x00f0) > 0x0090) temp += 0x0060;
            if ((temp & 0x0f00) > 0x0900) temp += 0x0600;
            if ((temp & 0xf000) > 0x9000) temp += 0x6000;
        }

        setc (temp & 0x10000);
        setv ((~(c.w ^ data)) & (c.w ^ temp) & 0x8000);
        setnz_w (c.w = temp);
        return (3);
    }
}

inline uint16_t Emulator::op_and (uint32_t l, uint32_t h)
{
    if (trace) dump ("AND", l, h);

    if (e || p.m) {
        setnz_b (c.l &= getByte (l));
        return (2);
    }
    else {
        setnz_w (c.w &= getWord (l, h));
        return (3);
    }
}

inline uint16_t Emulator::op_asl (uint32_t l, uint32_t h)
{
    if (trace) dump ("ASL", l, h);

		if (e || p.m) {
			register uint8_t    data = getByte (l);

			setc (data & 0x80);
			setnz_b (data <<= 1);
			setByte (l, data);
			return (4);
		}
		else {
			register uint16_t   data = getWord (l, h);

			setc (data & 0x8000);
			setnz_w (data <<= 1);
			setWord (l, h, data);
			return (5);
		}
}

inline uint16_t Emulator::op_asla (uint32_t l, uint32_t h)
{
    if (trace) dump ("ASL", l, h);

		if (e || p.m) {
			setc (c.l & 0x80);
			setnz_b (c.l <<= 1);
			return (2);
		}
		else {
			setc (c.w & 0x8000);
			setnz_w (c.w <<= 1);
			return (2);
		}
}

inline uint16_t Emulator::op_bcc (uint32_t l, uint32_t h)
{
    if (trace) dump ("BCC", l, h);

    if (p.c == 0) {
        register uint16_t page = (pc.w ^ l) & 0xff00;

        pc.w = (uint16_t) l;
        return ((e && page) ? 4 : 3);
    }
    return (2);
}

inline uint16_t Emulator::op_bcs (uint32_t l, uint32_t h)
{
    if (trace) dump ("BCS", l, h);

    if (p.c == 1) {
        register uint16_t page = (pc.w ^ l) & 0xff00;

        pc.w = (uint16_t) l;
        return ((e && page) ? 4 : 3);
    }
    return (2);
}

inline uint16_t Emulator::op_beq (uint32_t l, uint32_t h)
{
    if (trace) dump ("BEQ", l, h);

    if (p.z == 1) {
        register uint16_t page = (pc.w ^ l) & 0xff00;

        pc.w = (uint16_t) l;
        return ((e && page) ? 4 : 3);
    }
    return (2);
}

inline uint16_t Emulator::op_bit (uint32_t l, uint32_t h)
{
    if (trace) dump ("BIT", l, h);

    if (e || p.m) {
        register uint8_t    data = getByte (l);

        setz ((c.l & data) == 0);
        setn (data & 0x80);
        setv (data & 0x40);
        return (2);
    }
    else {
       register uint16_t   data = getWord (l, h);

        setz ((c.w & data) == 0);
        setn (data & 0x8000);
        setv (data & 0x4000);
        return (3);
    }
}

inline uint16_t Emulator::op_biti (uint32_t l, uint32_t h)
{
    if (trace) dump ("BIT", l, h);

    if (e || p.m) {
        register uint8_t    data = getByte (l);

        setz ((c.l & data) == 0);
        return (2);
    }
    else {
       register uint16_t   data = getWord (l, h);

        setz ((c.w & data) == 0);
        return (3);
    }
}

inline uint16_t Emulator::op_bmi (uint32_t l, uint32_t h)
{
    if (trace) dump ("BMI", l, h);

    if (p.n == 1) {
        register uint16_t page = (pc.w ^ l) & 0xff00;

        pc.w = (uint16_t) l;
        return ((e && page) ? 4 : 3);
    }
    return (2);
}

inline uint16_t Emulator::op_bne (uint32_t l, uint32_t h)
{
    if (trace) dump ("BNE", l, h);

    if (p.z == 0) {
        register uint16_t page = (pc.w ^ l) & 0xff00;

        pc.w = (uint16_t) l;
        return ((e && page) ? 4 : 3);
    }
    return (2);
}

inline uint16_t Emulator::op_bpl (uint32_t l, uint32_t h)
{
    if (trace) dump ("BPL", l, h);

    if (p.n == 0) {
        register uint16_t page = (pc.w ^ l) & 0xff00;

        pc.w = (uint16_t) l;
        return ((e && page) ? 4 : 3);
    }
    return (2);
}

inline uint16_t Emulator::op_bra (uint32_t l, uint32_t h)
{
    if (trace) dump ("BRA", l, h);

    register uint16_t page = (pc.w ^ l) & 0xff00;

    pc.w = (uint16_t) l;
    return ((e && page) ? 4 : 3);
}

inline uint16_t Emulator::op_brk (uint32_t l, uint32_t h)
{
    if (trace) dump ("BRK", l, h);

    if (e) {
        pushWord (pc.w);
        pushByte (p.b | 0x10);

        p.i = 1;
        p.d = 0;
        pbr.b = 0;

        pc.w = getWord (0xfffe, 0xffff);
        return (7);
    }
    else {
        pushByte (pbr.b);
        pushWord (pc.w);
        pushByte (p.b);

        p.i = 1;
        p.d = 0;
        pbr.b = 0;

        pc.w = getWord (0xffe6, 0xffe7);
        return (8);
    }
}

inline uint16_t Emulator::op_brl (uint32_t l, uint32_t h)
{
    if (trace) dump ("BRL", l, h);

    pc.w = l;
    return (4);
}

inline uint16_t Emulator::op_bvc (uint32_t l, uint32_t h)
{
    if (trace) dump ("BVC", l, h);

    if (p.v == 0) {
        register uint16_t page = (pc.w ^ l) & 0xff00;

        pc.w = (uint16_t) l;
        return ((e && page) ? 4 : 3);
    }
    return (2);
}

inline uint16_t Emulator::op_bvs (uint32_t l, uint32_t h)
{
    if (trace) dump ("BVS", l, h);

    if (p.v == 1) {
        register uint16_t page = (pc.w ^ l) & 0xff00;

        pc.w = (uint16_t) l;
        return ((e && page) ? 4 : 3);
    }
    return (2);
}

inline uint16_t Emulator::op_clc (uint32_t l, uint32_t h)
{
    if (trace) dump ("CLC", l, h);

    setc (false);
    return (2);
}

inline uint16_t Emulator::op_cld (uint32_t l, uint32_t h)
{
    if (trace) dump ("CLD", l, h);

    setd (false);
    return (2);
}

inline uint16_t Emulator::op_cli (uint32_t l, uint32_t h)
{
    if (trace) dump ("CLI", l, h);

    seti (false);
    return (2);
}

inline uint16_t Emulator::op_clv (uint32_t l, uint32_t h)
{
    if (trace) dump ("CLV", l, h);

    setv (false);
    return (2);
}

inline uint16_t Emulator::op_cmp (uint32_t l, uint32_t h)
{
    if (trace) dump ("CMP", l, h);

    if (e || p.m) {
        register uint8_t    data = getByte (l);
        register uint16_t   temp = c.l - data;

        setc (temp & 0x100);
        setnz_b (temp);
        return (2);
    }
    else {
        register uint16_t	data = getWord (l, h);
        register uint32_t   temp = c.w - data;

        setc (temp & 0x10000);
        setnz_w (temp);
        return (3);
    }
}

inline uint16_t Emulator::op_cop (uint32_t l, uint32_t h)
{
    if (trace) dump ("COP", l, h);

    if (e) {
        pushWord (pc.w);
        pushByte (p.b);

        p.i = 1;
        p.d = 0;
        pbr.b = 0;

        pc.w = getWord (0xfff4, 0xfff5);
       return (7);
    }
    else {
        pushByte (pbr.b);
        pushWord (pc.w);
        pushByte (p.b);

        p.i = 1;
        p.d = 0;
        pbr.b = 0;

        pc.w = getWord (0xffe4, 0xffe5);
        return (8);
    }
}

inline uint16_t Emulator::op_cpx (uint32_t l, uint32_t h)
{
    if (trace) dump ("CPX", l, h);

    if (e || p.x) {
        register uint8_t    data = getByte (l);
        register uint16_t   temp = x.l - data;

        setc (temp & 0x100);
        setnz_b (temp);
        return (2);
    }
    else {
        register uint16_t	data = getWord (l, h);
        register uint32_t	temp = x.w - data;

        setc (temp & 0x10000);
        setnz_w (temp);
        return (3);
    }
}

inline uint16_t Emulator::op_cpy (uint32_t l, uint32_t h)
{
    if (trace) dump ("CPY", l, h);

   if (e || p.x) {
        register uint8_t    data = getByte (l);
        register uint16_t   temp = y.l - data;

        setc (temp & 0x100);
        setnz_b (temp);
        return (2);
    }
    else {
        register uint16_t	data = getWord (l, h);
        register uint32_t	temp = y.w - data;

        setc (temp & 0x10000);
        setnz_w (temp);
        return (3);
    }
}

inline uint16_t Emulator::op_dec (uint32_t l, uint32_t h)
{
    if (trace) dump ("DEC", l, h);

    if (e || p.m) {
        register uint8_t    data = getByte (l);

        setByte (l, --data);
        setnz_b (data);
        return (4);
    }
    else {
        register uint16_t   data = getWord (l, h);

        setWord (l, h, --data);
        setnz_w (data);
        return (5);
    }
}

inline uint16_t Emulator::op_deca (uint32_t l, uint32_t h)
{
    if (trace) dump ("DEC", l, h);

    if (e || p.m)
        setnz_b (--c.l);
    else
        setnz_w (--c.w);

    return (2);
}

inline uint16_t Emulator::op_dex (uint32_t l, uint32_t h)
{
    if (trace) dump ("DEX", l, h);

    if (e || p.x)
        setnz_b (--x.l);
    else
        setnz_w (--x.w);

    return (2);
}

inline uint16_t Emulator::op_dey (uint32_t l, uint32_t h)
{
    if (trace) dump ("DEY", l, h);

    if (e || p.x)
        setnz_b (--y.l);
    else
        setnz_w (--y.w);

    return (2);
}

inline uint16_t Emulator::op_eor (uint32_t l, uint32_t h)
{
    if (trace) dump ("EOR", l, h);

    if (e || p.m) {
        setnz_b (c.l ^= getByte (l));
        return (2);
    }
    else {
        setnz_w (c.w ^= getWord (l, h));
        return (3);
    }
}

inline uint16_t Emulator::op_inc (uint32_t l, uint32_t h)
{
    if (trace) dump ("INC", l, h);

    if (e || p.m) {
        register uint8_t    data = getByte (l);

        setByte (l, ++data);
        setnz_b (data);
        return (4);
    }
    else {
        register uint16_t   data = getWord (l, h);

        setWord (l, h, ++data);
        setnz_w (data);
        return (5);
    }
}

inline uint16_t Emulator::op_inca (uint32_t l, uint32_t h)
{
    if (trace) dump ("INC", l, h);

    if (e || p.m)
        setnz_b (++c.l);
    else
        setnz_w (++c.w);

    return (2);
}

inline uint16_t Emulator::op_inx (uint32_t l, uint32_t h)
{
    if (trace) dump ("INX", l, h);

    if (e || p.x)
        setnz_b (++x.l);
    else
        setnz_w (++x.w);

    return (2);
}

inline uint16_t Emulator::op_iny (uint32_t l, uint32_t h)
{
    if (trace) dump ("INY", l, h);

    if (e || p.x)
        setnz_b (++y.l);
    else
        setnz_w (++y.w);

    return (2);
}

inline uint16_t Emulator::op_jml (uint32_t l, uint32_t h)
{
    if (trace) dump ("JML", l, h);

    pbr.a = l & 0xff0000;
    pc.w  = l;
    return (1);
}

inline uint16_t Emulator::op_jmp (uint32_t l, uint32_t h)
{
    if (trace) dump ("JMP", l, h);

    pc.w  = l;
    return (1);
}

inline uint16_t Emulator::op_jsl (uint32_t l, uint32_t h)
{
    if (trace) dump ("JSL", l, h);

    pushByte (pbr.b);
    pushWord (pc.w - 1);

    pbr.a = l & 0xff0000;
    pc.w  = l;
    return (5);
}

inline uint16_t Emulator::op_jsr (uint32_t l, uint32_t h)
{
    if (trace) dump ("JSR", l, h);

    pushWord (pc.w - 1);

    pc.w  = l;
    return (4);
}

inline uint16_t Emulator::op_lda (uint32_t l, uint32_t h)
{
    if (trace) dump ("LDA", l, h);

    if (e || p.m) {
        setnz_b (c.l = getByte (l));
        return (2);
    }
    else
    {
        setnz_w (c.w = getWord (l, h));
        return (3);
    }
}

inline uint16_t Emulator::op_ldx (uint32_t l, uint32_t h)
{
    if (trace) dump ("LDX", l, h);

    if (e || p.x) {
        setnz_b (x.l = getByte (l));
        return (2);
    }
    else
    {
        setnz_w (x.w = getWord (l, h));
        return (3);
    }
}

inline uint16_t Emulator::op_ldy (uint32_t l, uint32_t h)
{
    if (trace) dump ("LDY", l, h);

    if (e || p.x) {
        setnz_b (y.l = getByte (l));
        return (2);
    }
    else
    {
        setnz_w (y.w = getWord (l, h));
        return (3);
    }
}

inline uint16_t Emulator::op_lsr (uint32_t l, uint32_t h)
{
    if (trace) dump ("LSR", l, h);

    if (e || p.m) {
        setc (c.l & 0x01);
        setnz_b (c.l >>= 1);
    }
    else {
        setc (c.w & 0x0001);
        setnz_w (c.w >>= 1);
    }
    return (2);
}

inline uint16_t Emulator::op_lsra (uint32_t l, uint32_t h)
{
    if (trace) dump ("LSR", l, h);

    if (e || p.m) {
        register uint8_t    data = getByte (l);

        setc (data & 0x01);
        setnz_b (data >>= 1);
        setByte (l, data);
        return (4);
    }
    else {
        register uint16_t   data = getWord (l, h);

        setc (data & 0x0001);
        setnz_w (data >>= 1);
        setWord (l, h, data);
        return (5);
    }
}

inline uint16_t Emulator::op_mvn (uint32_t l, uint32_t h)
{
    if (trace) dump ("MVN", l, h);

    register uint32_t dst = getByte (l) << 16;
    register uint32_t src = getByte (h) << 16;

    setByte (dbr.a = dst | y.w++, getByte (src | x.w++));
    if (c.w-- != 0x0000) pc.w -= 3;

    return (7);
}

inline uint16_t Emulator::op_mvp (uint32_t l, uint32_t h)
{
    if (trace) dump ("MVP", l, h);

    register uint32_t dst = getByte (l) << 16;
    register uint32_t src = getByte (h) << 16;

    setByte (dbr.a = dst | y.w--, getByte (src | x.w--));
    if (c.w-- != 0x0000) pc.w -= 3;

    return (7);
}

inline uint16_t Emulator::op_nop (uint32_t l, uint32_t h)
{
    if (trace) dump ("NOP", l, h);

    return (2);
}

inline uint16_t Emulator::op_ora (uint32_t l, uint32_t h)
{
    if (trace) dump ("ORA", l, h);

    if (e || p.m) {
        setnz_b (c.l |= getByte (l));
        return (2);
    }
    else {
        setnz_w (c.w |= getWord (l, h));
        return (3);
    }
}

inline uint16_t Emulator::op_pea (uint32_t l, uint32_t h)
{
    if (trace) dump ("PEA", l, h);

    pushWord (getWord (l, h));
    return (5);
}

inline uint16_t Emulator::op_pei (uint32_t l, uint32_t h)
{
    if (trace) dump ("PEI", l, h);

    pushWord (getWord (l, h));
    return (6);
}

inline uint16_t Emulator::op_per (uint32_t l, uint32_t h)
{
    if (trace) dump ("PER", l, h);

    pushWord (l);
    return (6);
}

inline uint16_t Emulator::op_pha (uint32_t l, uint32_t h)
{
    if (trace) dump ("PHA", l, h);

    if (e || p.m) {
        pushByte (c.l);
        return (3);
    }
    else {
        pushWord (c.w);
        return (4);
    }
}

inline uint16_t Emulator::op_phb (uint32_t l, uint32_t h)
{
    if (trace) dump ("PHB", l, h);

    pushByte (dbr.b);
    return (3);
}

inline uint16_t Emulator::op_phd (uint32_t l, uint32_t h)
{
    if (trace) dump ("PHD", l, h);

    pushWord (dp.w);
    return (4);
}

inline uint16_t Emulator::op_phk (uint32_t l, uint32_t h)
{
    if (trace) dump ("PHK", l, h);

    pushByte (pbr.b);
    return (3);
}

inline uint16_t Emulator::op_php (uint32_t l, uint32_t h)
{
    if (trace) dump ("PHP", l, h);

    pushByte (p.b | (e ? 0x30 : 0x00));
    return (3);
}

inline uint16_t Emulator::op_phx (uint32_t l, uint32_t h)
{
    if (trace) dump ("PHX", l, h);

    if (e || p.x) {
        pushByte (x.l);
        return (3);
    }
    else {
        pushWord (x.w);
        return (4);
    }
}

inline uint16_t Emulator::op_phy (uint32_t l, uint32_t h)
{
    if (trace) dump ("PHY", l, h);

    if (e || p.x) {
        pushByte (y.l);
        return (3);
    }
    else {
        pushWord (y.w);
        return (4);
    }
}

inline uint16_t Emulator::op_pla (uint32_t l, uint32_t h)
{
    if (trace) dump ("PLA", l, h);

    if (e || p.m) {
        setnz_b (c.l = pullByte ());
        return (4);
    }
    else {
        setnz_b (c.w = pullWord ());
        return (5);
    }
}

inline uint16_t Emulator::op_plb (uint32_t l, uint32_t h)
{
    if (trace) dump ("PLB", l, h);

    setnz_b (dbr.b = pullByte ());
    return (4);
}

inline uint16_t Emulator::op_pld (uint32_t l, uint32_t h)
{
    if (trace) dump ("PLD", l, h);

    setnz_w (dp.w = pullWord ());
    return (5);
}

inline uint16_t Emulator::op_plp (uint32_t l, uint32_t h)
{
    if (trace) dump ("PLP", l, h);

    p.b = pullByte ();
    if (e)
        p.b |= 0x30;
    else {
        if (p.x) {
            x.w = x.l;
            y.w = y.l;
        }
    }
    return (4);
}

inline uint16_t Emulator::op_plx (uint32_t l, uint32_t h)
{
    if (trace) dump ("PLX", l, h);

    if (e || p.x) {
        setnz_b (x.l = pullByte ());
        return (4);
    }
    else {
        setnz_b (x.w = pullWord ());
        return (5);
    }
}

inline uint16_t Emulator::op_ply (uint32_t l, uint32_t h)
{
    if (trace) dump ("PLY", l, h);

    if (e || p.x) {
        setnz_b (y.l = pullByte ());
        return (4);
    }
    else {
        setnz_b (y.w = pullWord ());
        return (5);
    }
}

inline uint16_t Emulator::op_rep (uint32_t l, uint32_t h)
{
    if (trace) dump ("REP", l, h);

    p.b &= ~getByte (l);
    if (e) p.b |= 0x30;
    return (3);
}

inline uint16_t Emulator::op_rol (uint32_t l, uint32_t h)
{
    if (trace) dump ("ROL", l, h);

    if (e || p.m) {
        register uint8_t    data = getByte (l);
        register uint8_t    carry = p.c ? 0x01 : 0x00;

        setc (data & 0x80);
        setnz_b (data = (data << 1) | carry);
        setByte (l, data);
        return (4);
    }
    else {
        register uint16_t   data = getWord(l, h);
        register uint16_t   carry = p.c ? 0x0001 : 0x0000;

        setc (data & 0x8000);
        setnz_w (data = (data << 1) | carry);
        setWord (l, h, data);
        return (5);
    }
}

inline uint16_t Emulator::op_rola (uint32_t l, uint32_t h)
{
    if (trace) dump ("ROL", l, h);

    if (e || p.m) {
        register uint8_t    carry = p.c ? 0x01 : 0x00;

        setc (c.l & 0x80);
        setnz_b (c.l = (c.l << 1) | carry);
        return (4);
    }
    else {
        register uint16_t   carry = p.c ? 0x0001 : 0x0000;

        setc (c.w & 0x8000);
        setnz_w (c.w = (c.w << 1) | carry);
        return (5);
    }
}

inline uint16_t Emulator::op_ror (uint32_t l, uint32_t h)
{
    if (trace) dump ("ROR", l, h);

    if (e || p.m) {
        register uint8_t    data = getByte (l);
        register uint8_t    carry = p.c ? 0x80 : 0x00;

        setc (data & 0x01);
        setnz_b (data = (data >> 1) | carry);
        setByte (l, data);
        return (4);
    }
    else {
        register uint16_t   data = getWord(l, h);
        register uint16_t   carry = p.c ? 0x8000 : 0x0000;

        setc (data & 0x0001);
        setnz_w (data = (data >> 1) | carry);
        setWord (l, h, data);
        return (5);
    }
}

inline uint16_t Emulator::op_rora (uint32_t l, uint32_t h)
{
    if (trace) dump ("ROR", l, h);

    if (e || p.m) {
        register uint8_t    carry = p.c ? 0x80 : 0x00;

        setc (c.l & 0x01);
        setnz_b (c.l = (c.l >> 1) | carry);
        return (4);
    }
    else {
        register uint16_t   carry = p.c ? 0x8000 : 0x0000;

        setc (c.w & 0x0001);
        setnz_w (c.w = (c.w >> 1) | carry);
        return (5);
    }
}

inline uint16_t Emulator::op_rti (uint32_t l, uint32_t h)
{
    if (trace) dump ("RTI", l, h);

    if (e) {
        p.b = pullByte ();
        pc.w = pullWord ();
        p.i = 0;
        return (6);
    }
    else {
        p.b = pullByte ();
        pc.w = pullWord ();
        pbr.b = pullByte ();
        p.i = 0;
        return (7);
    }
}

inline uint16_t Emulator::op_rtl (uint32_t l, uint32_t h)
{
    if (trace) dump ("RTL", l, h);

    pc.w = pullWord () + 1;
    pbr.b = pullByte ();
    return (7);
}

inline uint16_t Emulator::op_rts (uint32_t l, uint32_t h)
{
    if (trace) dump ("RTS", l, h);

    pc.w = pullWord () + 1;
    return (6);
}

inline uint16_t Emulator::op_sbc(uint32_t l, uint32_t h)
{
    if (trace) dump ("SBC", l, h);

    if (e || p.m) {
        register uint8_t	data = ~getByte (l);
        register uint16_t   temp = c.l + data + p.c;
        
        if (p.d) {
            if ((temp & 0x0f) > 0x09) temp += 0x06;
            if ((temp & 0xf0) > 0x90) temp += 0x60;
        }

        setc (temp & 0x100);
        setv ((~(c.l ^ data)) & (c.l ^ temp) & 0x80);
        setnz_b (c.l = temp);
        return (2);
    }
    else {
        register uint16_t   data = ~getWord (l, h);
        register uint32_t   temp = c.w + data + p.c;

        if (p.d) {
            if ((temp & 0x000f) > 0x0009) temp += 0x0006;
            if ((temp & 0x00f0) > 0x0090) temp += 0x0060;
            if ((temp & 0x0f00) > 0x0900) temp += 0x0600;
            if ((temp & 0xf000) > 0x9000) temp += 0x6000;
        }

        setc (temp & 0x10000);
        setv ((~(c.w ^ data)) & (c.w ^ temp) & 0x8000);
        setnz_w (c.w = temp);
        return (3);
    }
}

inline uint16_t Emulator::op_sec (uint32_t l, uint32_t h)
{
    if (trace) dump ("SEC", l, h);

    setc (true);
    return (2);
}

inline uint16_t Emulator::op_sed (uint32_t l, uint32_t h)
{
    if (trace) dump ("SED", l, h);

    setd (true);
    return (2);
}

inline uint16_t Emulator::op_sei (uint32_t l, uint32_t h)
{
    if (trace) dump ("SEI", l, h);

    seti (true);
    return (2);
}

inline uint16_t Emulator::op_sep (uint32_t l, uint32_t h)
{
    if (trace) dump ("SEP", l, h);

    p.b |= getByte (l);
    if (e) p.m = p.x = 1;

    if (p.x) {
        x.w = x.l;
        y.w = y.l;
    }
    return (3);
}

inline uint16_t Emulator::op_sta (uint32_t l, uint32_t h)
{
    if (trace) dump ("STA", l, h);

    if (e || p.m) {
        setByte (l, c.l);
        return (2);
    }
    else {
        setWord (l, h, c.w);
        return (3);
    }
}

inline uint16_t Emulator::op_stp (uint32_t l, uint32_t h)
{
    if (trace) dump ("STP", l, h);

    --pc.w; 
    return (3);
}

inline uint16_t Emulator::op_stx (uint32_t l, uint32_t h)
{
    if (trace) dump ("STX", l, h);

    if (e || p.x) {
        setByte (l, x.l);
        return (2);
    }
    else {
        setWord (l, h, x.w);
        return (3);
    }
}

inline uint16_t Emulator::op_sty (uint32_t l, uint32_t h)
{
    if (trace) dump ("STY ", l, h);

    if (e || p.x) {
        setByte (l, y.l);
        return (2);
    }
    else {
        setWord (l, h, y.w);
        return (3);
    }
}

inline uint16_t Emulator::op_stz (uint32_t l, uint32_t h)
{
    if (trace) dump ("STZ", l, h);

    if (e || p.x) {
        setByte (l, 0);
        return (2);
    }
    else {
        setWord (l, h, 0);
        return (3);
    }
}

inline uint16_t Emulator::op_tax (uint32_t l, uint32_t h)
{
    if (trace) dump ("TAX", l, h);

	if (e || p.x)
        setnz_b (x.l = c.l);
    else
        setnz_w (x.w = c.w);

    return (2);
}

inline uint16_t Emulator::op_tay (uint32_t l, uint32_t h)
{
    if (trace) dump ("TAY", l, h);

	if (e || p.x)
        setnz_b (y.l = c.l);
    else
        setnz_w (y.w = c.w);

    return (2);
}

inline uint16_t Emulator::op_tcd (uint32_t l, uint32_t h)
{
    if (trace) dump ("TCD", l, h);

    dp.w = c.w;
    return (2);
}

inline uint16_t Emulator::op_tcs (uint32_t l, uint32_t h)
{
    if (trace) dump ("TCS", l, h);

    sp.w = e ? (0x0100 | c.l) : c.w;
    return (2);
}

inline uint16_t Emulator::op_tdc (uint32_t l, uint32_t h)
{
    if (trace) dump ("TDC", l, h);

    if (e || p.m)
        setnz_b (c.w = dp.w);
    else
        setnz_w (c.w = dp.w);

    return (2);
}

inline uint16_t Emulator::op_trb (uint32_t l, uint32_t h)
{
    if (trace) dump ("TRB", l, h);

    if (e || p.m) {
        register uint8_t    data = getByte (l);

        setByte (l, data & ~c.l);
        setz ((c.l & data) == 0);
        return (4);
    }
    else {
        register uint16_t   data = getWord (l, h);

        setWord (l, h, data & ~c.w);
        setz ((c.w & data) == 0);
        return (5);
    }
}

inline uint16_t Emulator::op_tsc (uint32_t l, uint32_t h)
{
    if (trace) dump ("TSC", l, h);

    if (e || p.m)
        setnz_b (c.w = sp.w);
    else
        setnz_w (c.w = sp.w);

    return (2);
}

inline uint16_t Emulator::op_tsb (uint32_t l, uint32_t h)
{
    if (trace) dump ("TSB", l, h);

    if (e || p.m) {
        register uint8_t    data = getByte (l);

        setByte (l, data | c.l);
        setz ((c.l & data) == 0);
        return (4);
    }
    else {
        register uint16_t   data = getWord (l, h);

        setWord (l, h, data | c.w);
        setz ((c.w & data) == 0);
        return (5);
    }
}

inline uint16_t Emulator::op_tsx (uint32_t l, uint32_t h)
{
    if (trace) dump ("TSX", l, h);

	if (e)
        setnz_b (x.l = sp.l);
    else
        setnz_w (x.w = sp.w);

    return (2);
}

inline uint16_t Emulator::op_txa (uint32_t l, uint32_t h)
{
    if (trace) dump ("TXA", l, h);

	if (e || p.m)
        setnz_b (c.l = x.l);
    else
        setnz_w (c.w = x.w);

    return (2);
}

inline uint16_t Emulator::op_txs (uint32_t l, uint32_t h)
{
    if (trace) dump ("TXS", l, h);

    if (e)
        sp.l = x.l;
    else
        sp.w = x.w;
    
    return (2);
}

inline uint16_t Emulator::op_txy (uint32_t l, uint32_t h)
{
    if (trace) dump ("TXY", l, h);

	if (e || p.x)
        setnz_b (y.l = x.l);
    else
        setnz_w (y.w = x.w);

   return (2);
}

inline uint16_t Emulator::op_tya (uint32_t l, uint32_t h)
{
    if (trace) dump ("TYA", l, h);

	if (e || p.m)
        setnz_b (c.l = y.l);
    else
        setnz_w (c.w = y.w);

    return (2);
}

inline uint16_t Emulator::op_tyx (uint32_t l, uint32_t h)
{
    if (trace) dump ("TYX", l, h);

	if (e || p.x)
        setnz_b (x.l = y.l);
    else
        setnz_w (x.w = y.w);

    return (2);
}

inline uint16_t Emulator::op_wai (uint32_t l, uint32_t h)
{
    if (trace) dump ("WAI", l, h);

    --pc.w; 
    return (3);
}

inline uint16_t Emulator::op_wdm (uint32_t l, uint32_t h)
{
    if (trace) dump ("WDM", l, h);

    // Decode the signature byte
    switch (getByte (l)) {
    case 0x00:  c.w  = ier;     break;
    case 0x01:  ier  =  c.w;    break;
    case 0x02:  ier |=  c.w;    break;
    case 0x03:  ier &= ~c.w;    break;

    case 0x04:  c.w  = ifr;     break;
    case 0x05:  ifr  =  c.w;    break;
    case 0x06:  ifr |=  c.w;    break;
    case 0x07:  ifr &= ~c.w;    break;

    case 0x08:  c.w = Serial.read (); break;
    case 0x09:  Serial.write (c.w); break;

    case 0xff:  {
            uint32_t            delta = micros () - start;

            Serial.println ("\n>> Halting:");
            Serial.printf (">> %d cycles in %d uS => %.2f MHz\n", cycles, delta, ((double) delta) / cycles);
            for (;;) ;
        }
    }

    return (3);
}

inline uint16_t Emulator::op_xba (uint32_t l, uint32_t h)
{
    if (trace) dump ("XBA", l, h);

    c.w = (c.w >> 8) | (c.w << 8);
    setnz_b (c.l);
    return (3);
}

inline uint16_t Emulator::op_xce (uint32_t l, uint32_t h)
{
    if (trace) dump ("XCE", l, h);

    register bool       oe = e;

    e = p.c;
    p.c = oe;

    if (e) {
        p.b |= 0x30;
        sp.w = 0x0100 | sp.l;
    }
    return (2);
}

#endif
