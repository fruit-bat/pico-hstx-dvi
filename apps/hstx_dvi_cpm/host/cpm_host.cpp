// Copyright (c) 2026 fruit-bat

// hstx_dvi_cpm on the PC, for screenshots: the same CP/M (CPM-Emulator's
// Computer), settings screen and terminal (pico-vt, drawn by hstx_dvi_vt.c)
// as on the Pico, with the drives in the current directory, the keys from a
// script, and the screen saved as a PPM when the script says.
//
// cpm_host <script> [out dir]
//
// The script is the keys typed, newlines ignored, with these in braces:
//   {enter} {esc} {tab} {up} {down} {left} {right} {pgup} {pgdn}
//   {ctrl-x}     a control key, e.g. {ctrl-\} for the settings screen
//   {wait n}     n ms more before the next key
//   {gap n}      n ms between keys from here on (30 to start with)
//   {shot name}  saves the screen as <out dir>/name.ppm
//   {quit}       stops
// A key is only taken when CP/M (or the settings screen) looks for one, so
// the screen has caught up by then. The time is the Z80's: its cycles at
// HOST_MHZ (the Pico's speed, so the settings screen shows much the same)
// plus waiting.

#include "hstx_dvi_vt.h"
extern "C" {
#include "hstx_dvi_row_fifo.h"
#include "hstx_dvi_row_buf.h"
#include "pico/multicore.h"
}
#include "computer.h"
#include "cpm_settings.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <streambuf>
#include <string>
#include <vector>

#define HOST_MHZ 23.6

// ---- Time: the Z80's cycles, and waiting

static uint64_t _cycles, _cycles_seen;
static double _clock_us;

extern "C" uint64_t time_us_64(void) {
    _clock_us += (double)(_cycles - _cycles_seen) / HOST_MHZ;
    _cycles_seen = _cycles;
    return (uint64_t)_clock_us;
}

extern "C" void busy_wait_until(absolute_time_t t) {
    if ((double)t > _clock_us) _clock_us = (double)t;
}

static uint32_t monitor(void* ctx, Z80& cpu, uint8_t* memory, uint64_t cycles) {
    _cycles = cycles;
    return cpm_settings_monitor(ctx, cpu, memory, cycles);
}

// ---- The screen

static vt_emu_t* _emu;
static uint8_t _frame[MODE_V_ACTIVE_LINES][MODE_H_ACTIVE_PIXELS];
static uint32_t _y;
static hstx_dvi_row_t _rows[4];
static uint32_t _row_i;
static std::string _out = ".";

extern "C" {
void hstx_dvi_row_buf_init(void) {}
hstx_dvi_row_t* hstx_dvi_row_buf_get(void) { return &_rows[(_row_i++) & 3]; }
void hstx_dvi_row_fifo_init1(PIO pio) { (void)pio; }
hstx_dvi_pixel_row_fetcher hstx_dvi_row_fifo_get_row_fetcher(void) { return NULL; }
void hstx_dvi_init(hstx_dvi_pixel_row_fetcher f) { (void)f; }
void multicore_launch_core1(void (*entry)(void)) { (void)entry; }
void hstx_dvi_row_fifo_put_blocking(hstx_dvi_row_t* r) {
    memcpy(_frame[_y], r->b, MODE_H_ACTIVE_PIXELS);
    _y = (_y + 1) % MODE_V_ACTIVE_LINES;
}

// All of stdio is the screen here
int stdio_put_string(const char* s, int len, bool newline, bool cr_translation) {
    (void)newline;
    for (int i = 0; i < len; ++i) {
        if (cr_translation && s[i] == '\n') vt_emu_put_ch(_emu, '\r');
        vt_emu_put_ch(_emu, s[i]);
    }
    return len;
}
int putchar_raw(int c) { const char ch = (char)c; stdio_put_string(&ch, 1, false, false); return c; }
}

// The Pico's printf, with its CR LF
static void say(const char* s) { stdio_put_string(s, (int)strlen(s), false, true); }

// std::cout, as the emulator's banner goes there
struct ScreenBuf : std::streambuf {
    int overflow(int c) override { if (c != EOF) { const char ch = (char)c; stdio_put_string(&ch, 1, false, true); } return c; }
};

static void shot(const std::string& name) {
    // One frame, with the cursor showing
    hstx_dvi_vt_render_frame(32);
    const std::string path = _out + "/" + name + ".ppm";
    FILE* f = fopen(path.c_str(), "wb");
    if (!f) { perror(path.c_str()); exit(1); }
    fprintf(f, "P6 %d %d 255\n", MODE_H_ACTIVE_PIXELS, MODE_V_ACTIVE_LINES);
    for (int y = 0; y < MODE_V_ACTIVE_LINES; ++y) {
        for (int x = 0; x < MODE_H_ACTIVE_PIXELS; ++x) {
            const uint8_t p = _frame[y][x];
            // RGB332, each part stretched to the full range
            const uint8_t rgb[3] = { (uint8_t)(((p >> 5) & 7) * 255 / 7), (uint8_t)(((p >> 2) & 7) * 255 / 7), (uint8_t)((p & 3) * 255 / 3) };
            fwrite(rgb, 1, 3, f);
        }
    }
    fclose(f);
    fprintf(stderr, "saved %s\n", path.c_str());
}

// ---- The keys, from the script

struct Step {
    int key;          // or -1
    double gap_ms;    // before it
    std::string shot; // or saves the screen
    bool quit;
};
static std::vector<Step> _script;
static size_t _next;
static double _due_us;

static void load(const char* path) {
    std::ifstream in(path);
    if (!in) { perror(path); exit(1); }
    std::stringstream ss;
    ss << in.rdbuf();
    const std::string s = ss.str();
    double gap = 30.0, extra = 0.0;
    auto add = [&](int k) { _script.push_back({ k, gap + extra, "", false }); extra = 0.0; };
    auto seq = [&](const char* q) { add(q[0]); for (const char* p = q + 1; *p; ++p) _script.push_back({ *p, 0.0, "", false }); };
    for (size_t i = 0; i < s.size(); ++i) {
        const char c = s[i];
        if (c == '\n' || c == '\r') continue;
        if (c != '{') { add((uint8_t)c); continue; }
        const size_t e = s.find('}', i + 1);
        if (e == std::string::npos) { fprintf(stderr, "no } after %zu\n", i); exit(1); }
        const std::string t = s.substr(i + 1, e - i - 1);
        i = e;
        if (t == "enter") add('\r');
        else if (t == "esc") add(0x1b);
        else if (t == "tab") add('\t');
        else if (t == "up") seq("\x1b[A");
        else if (t == "down") seq("\x1b[B");
        else if (t == "right") seq("\x1b[C");
        else if (t == "left") seq("\x1b[D");
        else if (t == "pgup") seq("\x1b[5~");
        else if (t == "pgdn") seq("\x1b[6~");
        else if (t == "{") add('{');
        else if (t.rfind("ctrl-", 0) == 0 && t.size() == 6) add(toupper((uint8_t)t[5]) & 0x1f);
        else if (t.rfind("wait ", 0) == 0) extra += atof(t.c_str() + 5);
        else if (t.rfind("gap ", 0) == 0) gap = atof(t.c_str() + 4);
        else if (t.rfind("shot ", 0) == 0) { _script.push_back({ -1, gap + extra, t.substr(5), false }); extra = 0.0; }
        else if (t == "quit") { _script.push_back({ -1, gap + extra, "", true }); extra = 0.0; }
        else { fprintf(stderr, "unknown {%s}\n", t.c_str()); exit(1); }
    }
}

extern "C" int getchar_timeout_us(uint32_t us) {
    (void)us;
    // Each look for a key takes a little time
    time_us_64();
    _clock_us += 10.0;
    while (_next < _script.size()) {
        const Step& st = _script[_next];
        if (_clock_us < _due_us + st.gap_ms * 1000.0) return PICO_ERROR_TIMEOUT;
        _due_us = _clock_us;
        ++_next;
        if (st.quit) exit(0);
        if (!st.shot.empty()) { shot(st.shot); continue; }
        return st.key;
    }
    fprintf(stderr, "end of the script\n");
    exit(0);
}

int main(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "cpm_host <script> [out dir]\n"); return 1; }
    load(argv[1]);
    if (argc > 2) _out = argv[2];

    // As main.cpp, less the hardware
    hstx_dvi_vt_init();
    hstx_dvi_vt_set_font(hstx_dvi_font_find(VT_FONT), 1, 1);
    hstx_dvi_vt_enable_alt_screen();
    _emu = hstx_dvi_vt_emu_get();
    vt_emu_set_reply(_emu, NULL, NULL);
    static ScreenBuf screen;
    std::cout.rdbuf(&screen);
    std::cerr.rdbuf(&screen);

    say("HSTX DVI CP/M\n");
    say("Ctrl-\\ (or F12) opens the settings screen\n");

    static Computer<64, 0xFC00, 0xFE00> computer("CCP-DR.64K", 0xF400);
    console_term = CPM_TERMINAL;
    cpm_settings_init(CPM_Z80_KHZ, 0xFC00, 0xFE00);
    computer.setMonitor(monitor, nullptr);
    conctrl_settings = cpm_settings_open;

    while (true) {
        try {
            computer.start();
        }
        catch (const CpmRestart&) {
            conctrl_flush();
            say("\nRestarting CP/M\n");
            continue;
        }
        catch (const std::exception& e) {
            conctrl_flush();
            say("\n"); say(e.what()); say("\n");
        }
        say("Press a key to restart CP/M\n");
        conctrl_getch_wait();
    }
}
