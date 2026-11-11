// Copyright (c) 2026 fruit-bat

// The settings screen: see cpm_settings.h.

#include "cpm_settings.h"
#include "z80_disasm.h"
#include "host_console.h"
#include "console.h"
#include "hstx_dvi_vt.h"
#include "pico/stdlib.h"
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>

// The speeds to choose from, in kHz, 0 being as fast as it goes
static const struct {
    uint32_t khz;
    const char* note;
} _speeds[] = {
    { 0, "as fast as it goes" },
    { 1000, "" },
    { 2000, "an 8080's speed" },
    { 2500, "the first Z80" },
    { 4000, "a Z80A" },
    { 6000, "a Z80B" },
    { 8000, "a Z80H" },
    { 10000, "" },
    { 20000, "the fastest CMOS Z80" },
};
static constexpr uint32_t SPEEDS = sizeof(_speeds) / sizeof(_speeds[0]);
static uint32_t _speed = 0; // Index of _speeds

// The Z80 and its memory, as last seen by the monitor
static Z80* _cpu = nullptr;
static uint8_t* _mem = nullptr;
static uint64_t _cycles = 0;

// The monitor's next look at the time and keys, in cycles; 0 to stop at the
// next step
static uint64_t _check_at = 0;
// Stop at the next step, and whether stopped by one
static bool _step = false;
static bool _stepped = false;
// Where the last steps stopped, latest first, for the code shown
static constexpr uint32_t HISTORY = 3;
static uint16_t _history[HISTORY];
static uint32_t _history_n = 0;
// Where the BDOS and BIOS are, which the emulator runs itself
static uint16_t _bdos, _bios;
// In the monitor (else the settings screen was opened from the BIOS)
static bool _in_monitor = false;

// Keeping to the speed: the cycles and time at a starting point
static uint64_t _t0, _c0;
// Measuring the speed: cycles and time counted, and the last result in kHz
static uint64_t _mt, _mc, _last_t, _last_c;
static uint32_t _measured = 0;

// How often the monitor looks at the time and keys, as fast as it goes
static constexpr uint32_t CHECK_CYCLES_FAST = 20000;

// Starts keeping to the speed, and measuring it, afresh from here
static void rebase(uint64_t cycles) {
    _t0 = _last_t = time_us_64();
    _c0 = _last_c = cycles;
}

void cpm_settings_init(uint32_t khz, uint16_t bdos, uint16_t bios) {
    _bdos = bdos;
    _bios = bios;
    for (uint32_t i = 0; i < SPEEDS; ++i) {
        if (_speeds[i].khz == khz) _speed = i;
    }
    rebase(0);
    _mt = _mc = 0;
}

// ---- Drawing on the terminal's alternate screen

static vt_emu_t* _emu;
static vt_term_t _saved; // The terminal as CP/M had it

static void put(const char* s) {
    vt_emu_put_str(_emu, (vt_char_t*)s);
}

static void putf(const char* fmt, ...) {
    char buf[128];
    va_list a;
    va_start(a, fmt);
    vsnprintf(buf, sizeof(buf), fmt, a);
    va_end(a);
    put(buf);
}

// Moves to a row (from 1) and clears it
static void row(uint32_t r) {
    putf("\x1b[%lu;1H\x1b[0m\x1b[2K", (unsigned long)r);
}

static void enter_screen() {
    _emu = hstx_dvi_vt_emu_get();
    _saved = _emu->term;
    // Cancel any escape sequence CP/M was part way through, then switch
    put("\x18");
    vt_term_alt_set(&_emu->term, true);
    put("\x1b[0m\x1b[?25l\x1b[2J");
}

static void leave_screen() {
    // Everything as CP/M left it, including its screen
    _emu->term = _saved;
    rebase(_cycles);
}

static void put_speed(uint32_t khz) {
    if (khz % 1000) putf("%lu.%lu MHz", (unsigned long)(khz / 1000), (unsigned long)(khz % 1000 / 100));
    else putf("%lu MHz", (unsigned long)(khz / 1000));
}

// The first address of the memory shown, following PC until moved, and the
// rows of it
static uint16_t _addr = 0x0100;
static bool _follow_pc = true;
// Editing memory: the byte at the cursor, its first hex digit if typed (else
// -1), and whether typing text rather than hex
static bool _editing = false;
static uint16_t _cur;
static int _nib = -1;
static bool _text = false;
// The rows of code shown, and of memory, leaving room for the rest
static uint32_t clamp(int32_t v, int32_t lo, int32_t hi) {
    return (uint32_t)(v < lo ? lo : v > hi ? hi : v);
}
static uint32_t code_rows() {
    return clamp((int32_t)_emu->term.h - 23, 4, 12);
}
static uint32_t mem_rows() {
    return clamp((int32_t)_emu->term.h - (int32_t)code_rows() - 17, 4, 16);
}

// A line of code: the address, the bytes and the instruction, with a note of
// what the emulator does itself
static void code_line(uint16_t a, const char* mark, const char* attr) {
    char ins[32];
    const uint32_t n = z80_disasm(_mem, a, ins, sizeof(ins));
    char bytes[16] = {0};
    for (uint32_t i = 0, k = 0; i < n; ++i) {
        k += snprintf(bytes + k, sizeof(bytes) - k, i ? " %02X" : "%02X", _mem[(uint16_t)(a + i)]);
    }
    putf(" %s%s%04X  %-11s  %-20s\x1b[0m", attr, mark, a, bytes, ins);
    if (a == 0x0005 || a == (uint16_t)(_bdos + 6)) putf("; BDOS function %u (emulated)", Z80_C(*_cpu));
    else if (a >= _bios) put("; BIOS (emulated)");
}

static void draw(const char* prompt = nullptr) {
    Z80& c = *_cpu;
    const uint8_t* m = _mem;
    const uint16_t pc = Z80_PC(c), sp = Z80_SP(c);
    const uint32_t w = _emu->term.w;
    if (_follow_pc) _addr = (pc & 0xfff0) - 32;

    row(1);
    putf("\x1b[7m %-*s", (int)(w - 1), "CP/M settings                           Ctrl-\\ or Esc: back to CP/M");

    row(3);
    put(" Z80 speed   \x1b[1m");
    if (_speeds[_speed].khz) put_speed(_speeds[_speed].khz);
    else put("unlimited");
    putf("\x1b[0m  %s", _speeds[_speed].note);
    put("\x1b[3;48H");
    if (_measured) {
        put("measured ");
        put_speed(_measured);
    }
    put("\x1b[3;68H+ - change");

    row(4);
    put(" State       ");
    if (_in_monitor) putf("%s %04X", _stepped ? "stepped to" : "paused at", pc);
    else if (conctrl_waiting) put(_step ? "waiting for input, then stops" : "waiting for input (S: stop after it)");
    else put("in a BDOS/BIOS call");
    putf("\x1b[4;62H%llu cycles", (unsigned long long)_cycles);

    row(5);
    putf(" Terminal    \x1b[1m%s\x1b[0m", console_term_name(console_term));
    put("\x1b[5;48Hthe codes programs use  T change");

    row(7);
    putf(" PC \x1b[1m%04X\x1b[0m  SP %04X  AF %04X  BC %04X  DE %04X  HL %04X  IX %04X  IY %04X",
        pc, sp, Z80_AF(c), Z80_BC(c), Z80_DE(c), Z80_HL(c), Z80_IX(c), Z80_IY(c));
    row(8);
    putf("                   AF'%04X  BC'%04X  DE'%04X  HL'%04X  I %02X    R %02X",
        Z80_AF_(c), Z80_BC_(c), Z80_DE_(c), Z80_HL_(c), c.i, (c.r & 0x7f) | (c.r7 & 0x80));
    row(9);
    const uint8_t f = Z80_F(c);
    putf(" Flags  S%u Z%u H%u P%u N%u C%u      IM %u  IFF %u",
        f >> 7 & 1, f >> 6 & 1, f >> 4 & 1, f >> 2 & 1, f >> 1 & 1, f & 1, c.im, c.iff1);
    put("\x1b[9;48HStack ");
    for (uint32_t i = 0; i < 4; ++i) {
        const uint16_t a = sp + i * 2;
        putf(" %04X", m[a] | m[(uint16_t)(a + 1)] << 8);
    }

    // The code: the last steps, dim, then from PC on
    const uint32_t code = code_rows();
    const uint32_t hist = _history_n < code / 3 ? _history_n : code / 3;
    uint32_t r = 11;
    for (uint32_t i = hist; i-- > 0;) {
        row(r++);
        code_line(_history[i], "   ", "\x1b[2m");
    }
    uint16_t a = pc;
    for (bool first = true; r < 11 + code; first = false) {
        row(r++);
        code_line(a, first ? "-> " : "   ", first ? "\x1b[1m" : "");
        char ins[32];
        a += z80_disasm(m, a, ins, sizeof(ins));
    }

    const uint32_t mtop = 11 + code + 1;
    row(mtop);
    put("\x1b[1m Memory  +0 +1 +2 +3 +4 +5 +6 +7 +8 +9 +A +B +C +D +E +F  0123456789ABCDEF");
    const uint32_t rows = mem_rows();
    for (uint32_t r = 0; r < rows; ++r) {
        const uint16_t a = _addr + r * 16;
        row(mtop + 1 + r);
        putf("   %04X ", a);
        for (uint32_t i = 0; i < 16; ++i) {
            const uint16_t b = a + i;
            if (_editing && b == _cur) {
                // The cursor, bright, with the first digit if typed
                if (_nib >= 0) putf(" \x1b[1;7m%X_\x1b[0m", _nib);
                else putf(_text ? " \x1b[1;4m%02X\x1b[0m" : " \x1b[1;7m%02X\x1b[0m", m[b]);
            }
            // The byte at PC in reverse (bold when editing), at SP underlined
            else if (b == pc) putf(_editing ? " \x1b[1m%02X\x1b[0m" : " \x1b[7m%02X\x1b[0m", m[b]);
            else if (b == sp) putf(" \x1b[4m%02X\x1b[0m", m[b]);
            else putf(" %02X", m[b]);
        }
        put("  ");
        for (uint32_t i = 0; i < 16; ++i) {
            const uint16_t b = a + i;
            const uint8_t ch = m[b] & 0x7f;
            const char t = ch >= 32 && ch < 127 ? ch : '.';
            if (_editing && b == _cur) putf(_text ? "\x1b[1;7m%c\x1b[0m" : "\x1b[1;4m%c\x1b[0m", t);
            else putf("%c", t);
        }
    }

    const uint32_t h = mtop + 1 + rows + 1;
    if (_editing) {
        row(h);
        putf(" \x1b[1mEditing %04X\x1b[0m: type %s to change it  \x1b[1mTab\x1b[0m %s",
            _cur, _text ? "text" : "hex", _text ? "hex instead" : "text instead");
        row(h + 1);
        put(" \x1b[1mArrows\x1b[0m move  \x1b[1mPgUp PgDn\x1b[0m a page  \x1b[1mEnter\x1b[0m or \x1b[1mEsc\x1b[0m done");
        row(h + 3);
        if (prompt) putf(" %s", prompt);
        put("\x1b[J");
        return;
    }
    row(h);
    put(" \x1b[1mS\x1b[0m/\x1b[1mSpace\x1b[0m step  \x1b[1mC\x1b[0m/\x1b[1mEsc\x1b[0m back to CP/M  "
        "\x1b[1mR\x1b[0m restart CP/M  \x1b[1m+ -\x1b[0m speed");
    row(h + 1);
    put(" \x1b[1mM\x1b[0m address  \x1b[1mE\x1b[0m edit  \x1b[1mP\x1b[0m follow PC  \x1b[1mK\x1b[0m stack  "
        "\x1b[1mUp Down , .\x1b[0m line  \x1b[1mPgUp PgDn < >\x1b[0m page");
    row(h + 3);
    if (prompt) putf(" %s", prompt);
    put("\x1b[J");
}

// ---- Keys

enum {
    KEY_UP = 0x100,
    KEY_DOWN,
    KEY_PAGE_UP,
    KEY_PAGE_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
};

static int key_wait(uint32_t us) {
    const uint64_t until = time_us_64() + us;
    int c;
    while ((c = conctrl_poll_raw()) < 0 && time_us_64() < until) {}
    return c;
}

// A key, with a serial terminal's arrows and pages (ESC [ A...) as one
static int key() {
    int c;
    while ((c = conctrl_poll_raw()) < 0) {}
    if (c != 0x1b) return c;
    // Escape on its own, or the start of a sequence
    int n = key_wait(20000);
    if (n != '[' && n != 'O') return 0x1b;
    int p = 0;
    while ((n = key_wait(20000)) >= '0' && n <= '9') p = p * 10 + n - '0';
    switch (n) {
        case 'A': return KEY_UP;
        case 'B': return KEY_DOWN;
        case 'C': return KEY_RIGHT;
        case 'D': return KEY_LEFT;
        case '~': return p == 5 ? KEY_PAGE_UP : p == 6 ? KEY_PAGE_DOWN : -1;
        default: return -1;
    }
}

// Asks for an address, in hex: false if none given
static bool ask_addr(uint16_t& a) {
    char s[5] = {0};
    uint32_t n = 0;
    while (true) {
        char p[64];
        snprintf(p, sizeof(p), "Memory at (hex, Enter): \x1b[1m%s_", s);
        draw(p);
        const int c = key();
        if (c == '\r' || c == '\n') break;
        if (c == 0x1b) return false;
        if ((c == 8 || c == 0x7f) && n) s[--n] = 0;
        else if (n < 4 && isxdigit(c)) s[n++] = toupper(c);
    }
    if (!n) return false;
    a = (uint16_t)strtoul(s, nullptr, 16);
    return true;
}

// Edits memory from the first byte shown: hex, or text after Tab, the arrows
// moving, until Enter or Esc
static void edit_memory() {
    _editing = true;
    _follow_pc = false;
    _cur = _addr;
    _nib = -1;
    while (true) {
        draw();
        const uint32_t rows = mem_rows();
        const int c = key();
        int move = 0;
        if (c == '\r' || c == '\n' || c == 0x1b) break;
        switch (c) {
            case '\t': _text = !_text; _nib = -1; break;
            case KEY_LEFT: case 'S' & 0x1f: case 8: case 0x7f: move = -1; break;
            case KEY_RIGHT: case 'D' & 0x1f: move = 1; break;
            case KEY_UP: case 'E' & 0x1f: move = -16; break;
            case KEY_DOWN: case 'X' & 0x1f: move = 16; break;
            case KEY_PAGE_UP: case 'R' & 0x1f: move = -16 * (int)rows; break;
            case KEY_PAGE_DOWN: case 'C' & 0x1f: move = 16 * (int)rows; break;
            default:
                if (_text) {
                    if (c >= 32 && c < 127) {
                        _mem[_cur] = (uint8_t)c;
                        move = 1;
                    }
                }
                else if (c >= 0 && c < 0x100 && isxdigit(c)) {
                    const int v = isdigit(c) ? c - '0' : toupper(c) - 'A' + 10;
                    if (_nib < 0) _nib = v;
                    else {
                        _mem[_cur] = (uint8_t)(_nib << 4 | v);
                        move = 1;
                    }
                }
                break;
        }
        if (move) {
            _cur += move;
            _nib = -1;
            // Keep the cursor in view
            const uint16_t line = _cur & 0xfff0;
            if ((uint16_t)(line - _addr) >= rows * 16) {
                _addr = move < 0 ? line : (uint16_t)(line - (rows - 1) * 16);
            }
        }
    }
    _editing = false;
    _nib = -1;
}

void cpm_settings_open() {
    if (!_cpu) return;
    enter_screen();
    if (!conctrl_waiting) _step = false;
    while (true) {
        draw();
        const uint16_t page = mem_rows() * 16;
        switch (int c = key()) {
            case 's': case 'S': case ' ':
                if (conctrl_waiting) {
                    // The Z80 can't step until CP/M has its input: stop at
                    // the first instruction after (or not, pressed again)
                    _step = !_step;
                    _check_at = 0;
                    break;
                }
                // Remember where it was, for the code shown
                for (uint32_t i = HISTORY - 1; i > 0; --i) _history[i] = _history[i - 1];
                _history[0] = Z80_PC(*_cpu);
                if (_history_n < HISTORY) ++_history_n;
                _step = true;
                _check_at = 0;
                leave_screen();
                return;
            case 'c': case 'C': case 0x1b: case CONCTRL_SETTINGS_KEY:
                // Going on, unless stopping after CP/M's input
                if (!_step) _history_n = 0;
                leave_screen();
                return;
            case 'r': case 'R':
                leave_screen();
                conctrl_waiting = false;
                conctrl_key = -1;
                // The cycles start again from 0
                _check_at = 0;
                rebase(0);
                throw CpmRestart();
            case '+': case '=':
                if (_speed + 1 < SPEEDS) ++_speed;
                break;
            case '-': case '_':
                if (_speed > 0) --_speed;
                break;
            case 'm': case 'M':
                if (ask_addr(_addr)) {
                    _addr &= 0xfff0;
                    _follow_pc = false;
                }
                break;
            case 'e': case 'E':
                edit_memory();
                break;
            case 'p': case 'P':
                _follow_pc = true;
                break;
            case 't': case 'T':
                console_term = (ConsoleTerm)((console_term + 1) % CONSOLE_TERMS);
                console_reset();
                break;
            case 'k': case 'K':
                _addr = Z80_SP(*_cpu) & 0xfff0;
                _follow_pc = false;
                break;
            case ',': case KEY_UP: case 'E' & 0x1f:
                _addr -= 16;
                _follow_pc = false;
                break;
            case '.': case KEY_DOWN: case 'X' & 0x1f:
                _addr += 16;
                _follow_pc = false;
                break;
            case '<': case KEY_PAGE_UP: case 'R' & 0x1f:
                _addr -= page;
                _follow_pc = false;
                break;
            case '>': case KEY_PAGE_DOWN: case 'C' & 0x1f:
                _addr += page;
                _follow_pc = false;
                break;
            default:
                (void)c;
                break;
        }
    }
}

// ---- The monitor

// The slow path: stepping, keeping to the speed, measuring it, and keys
static void __attribute__((noinline)) monitor_check(uint64_t cycles) {
    _in_monitor = true;
    if (_step) {
        // Stopped at the next step
        _step = false;
        _stepped = true;
        _measured = 0;
        cpm_settings_open();
        _stepped = false;
    }
    const uint32_t khz = _speeds[_speed].khz;
    uint64_t now = time_us_64();

    if (khz) {
        // Where it should be by now, waiting until then if ahead, or starting
        // afresh if far behind (it was paused, or waiting for a key)
        const uint64_t due = _t0 + (cycles - _c0) * 1000 / khz;
        if (due > now) {
            busy_wait_until(from_us_since_boot(due));
            now = due;
        }
        else if (now - due > 20000) {
            _t0 = now;
            _c0 = cycles;
        }
    }
    // Measure the speed over a quarter of a second or more, waiting to keep
    // to the speed included, but leaving out whatever time the Z80 wasn't
    // running (e.g. paused, or CP/M waiting for a key)
    if (now - _last_t < 10000) {
        _mt += now - _last_t;
        _mc += cycles - _last_c;
        if (_mt >= 250000) {
            _measured = (uint32_t)(_mc * 1000 / _mt);
            _mt = _mc = 0;
        }
    }

    _last_t = now;
    _last_c = cycles;

    // The settings key, or a key for CP/M read ahead
    conctrl_status();

    _in_monitor = false;
    // Look again at the next step if stepping, else after a millisecond at
    // the speed set
    _check_at = _step ? 0 : cycles + (khz ? khz : CHECK_CYCLES_FAST);
}

void cpm_settings_monitor(void*, Z80& cpu, uint8_t* memory, uint64_t cycles) {
    _cycles = cycles;
    if (cycles < _check_at) return;
    _cpu = &cpu;
    _mem = memory;
    monitor_check(cycles);
}
