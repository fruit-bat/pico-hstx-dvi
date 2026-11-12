// Copyright (c) 2026 fruit-bat

// A small Z80 disassembler: see z80_disasm.h. It decodes each opcode by its
// fields, x (bits 7-6), y (5-3), z (2-0), p (5-4) and q (3), as in "Decoding
// Z80 opcodes" by Cristian Dinu.

#include "z80_disasm.h"
#include <cstdarg>
#include <cstdio>

namespace {

const char* const CC[8] = { "NZ", "Z", "NC", "C", "PO", "PE", "P", "M" };
const char* const ALU[8] = { "ADD A,", "ADC A,", "SUB ", "SBC A,", "AND ", "XOR ", "OR ", "CP " };
const char* const ROT[8] = { "RLC", "RRC", "RL", "RR", "SLA", "SRA", "SLL", "SRL" };
const char* const IM[8] = { "0", "0/1", "1", "2", "0", "0/1", "1", "2" };
const char* const BLI[4][4] = {
    { "LDI", "CPI", "INI", "OUTI" },
    { "LDD", "CPD", "IND", "OUTD" },
    { "LDIR", "CPIR", "INIR", "OTIR" },
    { "LDDR", "CPDR", "INDR", "OTDR" },
};
const char* const MISC0[8] = { "RLCA", "RRCA", "RLA", "RRA", "DAA", "CPL", "SCF", "CCF" };
const char* const MISC7[8] = { "LD I,A", "LD R,A", "LD A,I", "LD A,R", "RRD", "RLD", "NOP", "NOP" };

// One instruction being decoded
struct Dis {
    const uint8_t* m;
    uint16_t start, a; // Its address, and the next byte's
    int xy;            // 0 for HL, 1 for IX, 2 for IY
    bool disp_read;    // The displacement of (IX+d) read already (DD CB d op)
    int8_t disp;
    char* out;
    size_t size, n;

    uint8_t byte() { return m[a++]; }
    uint16_t word() { const uint8_t l = byte(); return l | byte() << 8; }

    void put(const char* fmt, ...) {
        if (n >= size) return;
        va_list v;
        va_start(v, fmt);
        const int k = vsnprintf(out + n, size - n, fmt, v);
        va_end(v);
        if (k > 0) n += (size_t)k;
        if (n >= size) n = size - 1;
    }
    // Numbers as Zilog has them: hex, with a leading 0 if a letter starts it
    void hex8(uint8_t v) { put(v >= 0xa0 ? "0%02Xh" : "%02Xh", v); }
    void hex16(uint16_t v) { put(v >= 0xa000 ? "0%04Xh" : "%04Xh", v); }
    void n8() { hex8(byte()); }
    void nn() { hex16(word()); }

    const char* hl() { return xy == 1 ? "IX" : xy == 2 ? "IY" : "HL"; }
    // (HL), or (IX+d), reading d if not yet read
    void mem_hl() {
        if (!xy) { put("(HL)"); return; }
        if (!disp_read) { disp = (int8_t)byte(); disp_read = true; }
        put("(%s", hl());
        if (disp < 0) { put("-"); hex8((uint8_t)-disp); }
        else { put("+"); hex8((uint8_t)disp); }
        put(")");
    }
    // A register by its 3 bits; H and L become IXH... unless plain (when the
    // instruction also has (IX+d))
    void r(int i, bool plain = false) {
        static const char* const R[8] = { "B", "C", "D", "E", "H", "L", "", "A" };
        if (i == 6) mem_hl();
        else if ((i == 4 || i == 5) && xy && !plain) put("%s%c", hl(), i == 4 ? 'H' : 'L');
        else put("%s", R[i]);
    }
    void rp(int p) { static const char* const RP[4] = { "BC", "DE", "", "SP" }; put("%s", p == 2 ? hl() : RP[p]); }
    void rp2(int p) { static const char* const RP[4] = { "BC", "DE", "", "AF" }; put("%s", p == 2 ? hl() : RP[p]); }

    void cb() {
        // DD CB d op: the displacement comes before the opcode
        if (xy) { disp = (int8_t)byte(); disp_read = true; }
        const uint8_t op = byte();
        const int x = op >> 6, y = (op >> 3) & 7, z = op & 7;
        if (x == 0) put("%s ", ROT[y]);
        else put("%s %d,", x == 1 ? "BIT" : x == 2 ? "RES" : "SET", y);
        if (xy) {
            mem_hl();
            // Undocumented: the result also goes to a register
            if (z != 6 && x != 1) { put(","); r(z, true); }
        }
        else r(z);
    }

    void ed() {
        const uint8_t op = byte();
        const int x = op >> 6, y = (op >> 3) & 7, z = op & 7, p = y >> 1, q = y & 1;
        if (x == 1) {
            switch (z) {
                case 0: if (y == 6) put("IN (C)"); else { put("IN "); r(y); put(",(C)"); } return;
                case 1: if (y == 6) put("OUT (C),0"); else { put("OUT (C),"); r(y); } return;
                case 2: put(q ? "ADC HL," : "SBC HL,"); rp(p); return;
                case 3:
                    if (q) { put("LD "); rp(p); put(",("); nn(); put(")"); }
                    else { put("LD ("); nn(); put("),"); rp(p); }
                    return;
                case 4: put("NEG"); return;
                case 5: put(y == 1 ? "RETI" : "RETN"); return;
                case 6: put("IM %s", IM[y]); return;
                case 7: put("%s", MISC7[y]); return;
            }
        }
        if (x == 2 && z <= 3 && y >= 4) { put("%s", BLI[y - 4][z]); return; }
        put("DB 0EDh,"); hex8(op);
    }

    void main_op(uint8_t op) {
        const int x = op >> 6, y = (op >> 3) & 7, z = op & 7, p = y >> 1, q = y & 1;
        switch (x) {
        case 0:
            switch (z) {
            case 0:
                if (y == 0) put("NOP");
                else if (y == 1) put("EX AF,AF'");
                else {
                    const int8_t d = (int8_t)byte();
                    if (y == 2) put("DJNZ ");
                    else if (y == 3) put("JR ");
                    else put("JR %s,", CC[y - 4]);
                    hex16((uint16_t)(a + d));
                }
                return;
            case 1:
                if (q) { put("ADD %s,", hl()); rp(p); }
                else { put("LD "); rp(p); put(","); nn(); }
                return;
            case 2:
                switch (y) {
                    case 0: put("LD (BC),A"); return;
                    case 1: put("LD A,(BC)"); return;
                    case 2: put("LD (DE),A"); return;
                    case 3: put("LD A,(DE)"); return;
                    case 4: put("LD ("); nn(); put("),%s", hl()); return;
                    case 5: put("LD %s,(", hl()); nn(); put(")"); return;
                    case 6: put("LD ("); nn(); put("),A"); return;
                    case 7: put("LD A,("); nn(); put(")"); return;
                }
                return;
            case 3: put(q ? "DEC " : "INC "); rp(p); return;
            case 4: put("INC "); r(y); return;
            case 5: put("DEC "); r(y); return;
            case 6: put("LD "); r(y); put(","); n8(); return;
            case 7: put("%s", MISC0[y]); return;
            }
            return;
        case 1:
            if (y == 6 && z == 6) { put("HALT"); return; }
            // With (IX+d), the other register is plain H or L
            put("LD "); r(y, z == 6); put(","); r(z, y == 6);
            return;
        case 2:
            put("%s", ALU[y]); r(z);
            return;
        case 3:
            switch (z) {
            case 0: put("RET %s", CC[y]); return;
            case 1:
                if (!q) { put("POP "); rp2(p); return; }
                switch (p) {
                    case 0: put("RET"); return;
                    case 1: put("EXX"); return;
                    case 2: put("JP (%s)", hl()); return;
                    case 3: put("LD SP,%s", hl()); return;
                }
                return;
            case 2: put("JP %s,", CC[y]); nn(); return;
            case 3:
                switch (y) {
                    case 0: put("JP "); nn(); return;
                    case 1: cb(); return;
                    case 2: put("OUT ("); n8(); put("),A"); return;
                    case 3: put("IN A,("); n8(); put(")"); return;
                    case 4: put("EX (SP),%s", hl()); return;
                    case 5: put("EX DE,HL"); return;
                    case 6: put("DI"); return;
                    case 7: put("EI"); return;
                }
                return;
            case 4: put("CALL %s,", CC[y]); nn(); return;
            case 5:
                if (!q) { put("PUSH "); rp2(p); return; }
                if (p == 0) { put("CALL "); nn(); return; }
                if (p == 2) { ed(); return; }
                // DD or FD: a prefix for the next opcode, unless that is
                // another prefix, when this one does nothing
                {
                    const uint8_t next = m[a];
                    if (xy || next == 0xdd || next == 0xfd || next == 0xed) { put("NOP*"); return; }
                    xy = p == 1 ? 1 : 2;
                    main_op(byte());
                }
                return;
            case 6: put("%s", ALU[y]); n8(); return;
            case 7: put("RST "); hex8((uint8_t)(y * 8)); return;
            }
        }
    }
};

}

uint32_t z80_disasm(const uint8_t* memory, uint16_t addr, char* out, size_t size) {
    if (!size) return 1;
    out[0] = 0;
    Dis d { memory, addr, addr, 0, false, 0, out, size, 0 };
    d.main_op(d.byte());
    return (uint16_t)(d.a - d.start);
}
