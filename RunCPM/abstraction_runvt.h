#ifndef ABSTRACT_RUNVT_H
#define ABSTRACT_RUNVT_H

/* see main.c for definition */

/*
abstraction_runvt.h - RunCPM console I/O backed directly by RunVT's
in-process screen/vtparser/render core. No OS console, pty, pipe, or
ConPTY sits between RunCPM and the terminal - just function calls in
one process, so this works the same way on Posix and Windows.

Everything except console I/O (disk, host init, hardware ports) still
comes from abstraction_posix.h / abstraction_windows.h unchanged -
RUNVT_EMBED (defined externally with -DRUNVT_EMBED, see globals.h) just
switches off their console block, so the functions below are the ones
that link instead. console.h itself is untouched and still builds
_putcon/_getcon/_chready/etc. on top of the primitives this file
provides, exactly as it does for every other platform.

Note: this file deliberately does NOT share abstraction_posix.h /
abstraction_windows.h's own ABSTRACT_H include guard - it needs those
files' bodies to actually run when included from here, not get skipped
as "already included".

Requires RunVT's sources on the include path (e.g. `-IRunVT/src` if
RunVT was cloned into this folder) - see Makefile.runvt.
*/

#if defined(_WIN32)
    // mingw's conio.h declares _kbhit/_getch/_getche/_putch with libc
    // signatures (int-based) that don't match the uint8-based ones this
    // file defines below - conflicting types, not just a redeclaration.
    // They're only ever used from the console block abstraction_windows.h
    // itself skips under RUNVT_EMBED, so renaming them out of the way for
    // the duration of this include is safe and keeps the real conio.h
    // prototypes from colliding with ours.
    #define _kbhit  _runvt_disabled_kbhit
    #define _getch  _runvt_disabled_getch
    #define _getche _runvt_disabled_getche
    #define _putch  _runvt_disabled_putch
    #include "abstraction_windows.h"
    #undef _kbhit
    #undef _getch
    #undef _getche
    #undef _putch
#else
    #include "abstraction_posix.h"
#endif

#include "runvt_core.h"

// Defined (with an initializer) in whichever cpu*.h main.c includes -
// that happens after this file, so it needs forward declaring here.
extern int32 Status;

static RunVT g_runvt;

// Bytes RunVT hands back to us - keystrokes, pasted text, DSR/DA query
// replies - land here via input_cb. Drained by _kbhit/_getch below.
// Both run on the same thread (RunCPM's single interpreter loop calls
// straight into rt_core_pump(), which is what fills this), so no
// locking is needed despite looking like a producer/consumer queue.
#define RUNVT_INBUF_SIZE 256
static unsigned char g_runvt_inbuf[RUNVT_INBUF_SIZE];
static int g_runvt_in_head = 0, g_runvt_in_tail = 0;

static int _runvt_input_cb(void *ctx, const unsigned char *buf, int len) {
    (void)ctx;
    for (int i = 0; i < len; i++) {
        int next = (g_runvt_in_head + 1) % RUNVT_INBUF_SIZE;
        if (next == g_runvt_in_tail) break; // overflow: drop rather than block
        g_runvt_inbuf[g_runvt_in_head] = buf[i];
        g_runvt_in_head = next;
    }
    return len;
}

// Every console byte in or out passes through one of the four
// functions below - the natural pump point regardless of whether the
// running CP/M program used a BIOS or BDOS console call (cpm.h
// dispatches both straight to these), and regardless of which CPU core
// is in use, with no edit needed to cpm.h or cpu1-4.h.
// render_frame() presents with vsync on, so calling it on every single
// character (as _putch() does, e.g. when a program blasts a whole file
// to the console) caps output throughput at the monitor's refresh rate
// - a handful of chars/frame at best. Event handling stays untouched
// (needs to stay responsive and is cheap - no vsync wait), only the
// actual redraw is time-gated: the screen buffer is already correct
// the instant rt_core_feed() runs, this just batches how often that
// buffer gets presented.
#define RUNVT_RENDER_INTERVAL_MS 15
static void _runvt_pump(void) {
    static Uint32 last_render = 0;
    Uint32 now;

    // Setting Status alone isn't reliable here - CCP's idle console-poll loop can spin for ages without ever rechecking it. Just exit now.
    if (rt_core_pump(&g_runvt)) {
        rt_core_shutdown(&g_runvt);
        exit(EXIT_SUCCESS);
    }

    now = SDL_GetTicks();
    if (now - last_render < RUNVT_RENDER_INTERVAL_MS) return;
    last_render = now;
    rt_core_maybe_render(&g_runvt);
}

void _console_init(void) {
    RunVTConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.cols = 80;
    cfg.rows = 25;
    cfg.zoom = 1.0;
    cfg.title = "RunCPM v" VERSION;
    cfg.codepage = CODEPAGE_LATIN1;
    cfg.input_cb = _runvt_input_cb;
    cfg.input_ctx = NULL;
    if (rt_core_init(&g_runvt, &cfg) != 0) {
        fprintf(stderr, "RunCPM: failed to initialize RunVT/SDL - is a display available?\n");
        exit(EXIT_FAILURE);
    }
}

void _console_reset(void) {
    rt_core_shutdown(&g_runvt);
}

void _clrscr(void) {
    static const unsigned char clr[] = "\x1B[2J\x1B[H";
    rt_core_feed(&g_runvt, clr, sizeof(clr) - 1);
    _runvt_pump();
}

int _kbhit(void) {
    _runvt_pump();
    return g_runvt_in_head != g_runvt_in_tail;
}

uint8 _getch(void) {
    while (g_runvt_in_head == g_runvt_in_tail) {
        _runvt_pump();
        if (Status != STATUS_RUNNING) return 0; // window closed while waiting
        SDL_Delay(1);
    }
    uint8 ch = g_runvt_inbuf[g_runvt_in_tail];
    g_runvt_in_tail = (g_runvt_in_tail + 1) % RUNVT_INBUF_SIZE;
    return ch;
}

void _putch(uint8 ch) {
    rt_core_feed(&g_runvt, &ch, 1);
    _runvt_pump();
}

uint8 _getche(void) {
    uint8 ch = _getch();
    rt_core_feed(&g_runvt, &ch, 1); // echo into the screen
    return ch;
}

#endif
