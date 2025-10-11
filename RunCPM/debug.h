#ifndef DEBUG_H
#define DEBUG_H

void watchprint(uint16 pos) {
    uint8 I, J;
    _puts("\r\n");
    _puts("  Watch : ");
    _puthex16(Watch);
    _puts(" = ");
    _puthex8(_RamRead(Watch));
    _putcon(':');
    _puthex8(_RamRead(Watch + 1));
    _puts(" / ");
    for (J = 0, I = _RamRead(Watch); J < 8; ++J, I <<= 1)
        _putcon(I & 0x80 ? '1' : '0');
    _putcon(':');
    for (J = 0, I = _RamRead(Watch + 1); J < 8; ++J, I <<= 1)
        _putcon(I & 0x80 ? '1' : '0');
}

void memdump(uint16 pos) {
    uint16 h = pos;
    uint16 c = pos;
    uint8 l, i;
    uint8 ch = pos & 0xff;

    _puts("       ");
    for (i = 0; i < 16; ++i) {
        _puthex8(ch++ & 0x0f);
        _puts(" ");
    }
    _puts("\r\n");
    _puts("       ");
    for (i = 0; i < 16; ++i)
        _puts("---");
    _puts("\r\n");
    for (l = 0; l < 16; ++l) {
        _puthex16(h);
        _puts(" : ");
        for (i = 0; i < 16; ++i) {
            _puthex8(_RamRead(h++));
            _puts(" ");
        }
        for (i = 0; i < 16; ++i) {
            ch = _RamRead(c++);
            _putcon(ch > 31 && ch < 127 ? ch : '.');
        }
        _puts("\r\n");
    }
}

static int read_hex8(uint8 *out) {
    unsigned int v = 0;
    int count = 0;
    int ch;

    /* Read characters until newline/carriage return */
    while (1) {
        ch = _getcon();
        /* End on CR or LF */
        if (ch == '\r' || ch == '\n')
            break;

        /* Backspace handling (BS=8, DEL=127) */
        if (ch == 8 || ch == 127) {
            if (count > 0) {
                /* erase last hex digit visually (basic backspace handling) */
                _putch('\b');
                _putch(' ');
                _putch('\b');
                v >>= 4;
                --count;
            }
            continue;
        }

        /* Accept 0-9 */
        if (ch >= '0' && ch <= '9') {
            if (count < 2) {
                v = (v << 4) | (unsigned int)(ch - '0');
                ++count;
                _putch((char)ch);
            }
            continue;
        }

        /* Accept a-f */
        if (ch >= 'a' && ch <= 'f') {
            if (count < 2) {
                v = (v << 4) | (unsigned int)(10 + ch - 'a');
                ++count;
                _putch((char)ch);
            }
            continue;
        }

        /* Accept A-F */
        if (ch >= 'A' && ch <= 'F') {
            if (count < 2) {
                v = (v << 4) | (unsigned int)(10 + ch - 'A');
                ++count;
                _putch((char)ch);
            }
            continue;
        }

        /* Ignore 'x'/'X' to allow typing "0x..." */
        if (ch == 'x' || ch == 'X')
            continue;

        /* Ignore any other characters */
    }

    /* move to next line visually */
    _putch('\r');
    _putch('\n');

    if (count == 0)
        return 0; /* no digits entered */

    *out = (uint8)(v & 0xFFu);
    return 1;
}

static int read_hex16(uint16 *out) {
    unsigned int v = 0;
    int count = 0;
    int ch;

    /* Read characters until newline/carriage return */
    while (1) {
        ch = _getcon();
        /* End on CR or LF */
        if (ch == '\r' || ch == '\n')
            break;

        /* Backspace handling (BS=8, DEL=127) */
        if (ch == 8 || ch == 127) {
            if (count > 0) {
                /* erase last hex digit visually (basic backspace handling) */
                _putch('\b');
                _putch(' ');
                _putch('\b');
                v >>= 4;
                --count;
            }
            continue;
        }

        /* Accept 0-9 */
        if (ch >= '0' && ch <= '9') {
            if (count < 4) {
                v = (v << 4) | (unsigned int)(ch - '0');
                ++count;
                _putch((char)ch);
            }
            continue;
        }

        /* Accept a-f */
        if (ch >= 'a' && ch <= 'f') {
            if (count < 4) {
                v = (v << 4) | (unsigned int)(10 + ch - 'a');
                ++count;
                _putch((char)ch);
            }
            continue;
        }

        /* Accept A-F */
        if (ch >= 'A' && ch <= 'F') {
            if (count < 4) {
                v = (v << 4) | (unsigned int)(10 + ch - 'A');
                ++count;
                _putch((char)ch);
            }
            continue;
        }

        /* Ignore 'x'/'X' to allow typing "0x..." */
        if (ch == 'x' || ch == 'X')
            continue;

        /* Ignore any other characters */
    }

    /* move to next line visually */
    _putch('\r');
    _putch('\n');

    if (count == 0)
        return 0; /* no digits entered */

    *out = (uint16)(v & 0xFFFFu);
    return 1;
}

/* Read opcode prefixes from memory at 'pos', advance pos to the
   first operand byte and return the mnemonic pointer. It also returns the
   initial byte-count (prefixes + opcode bytes consumed so far) via *countp
   and an optional prefix character ('X'/'Y' or 0) via *prefixp.

   Inputs:
         posp  - pointer to the position (will be advanced to operand start)
   Outputs:
         returns const char* mnemonic string (one of Mnemonics*, MnemonicsCB, ...)
         *countp set to initial consumed bytes (1 or more)
         *prefixp set to 'X' or 'Y' or 0 if applicable
*/
static const char *GetMnemonicAt(uint16 *posp, uint8 *countp, char *prefixp) {
    uint16 pos = *posp;
    uint8 ch = _RamRead(pos);
    uint8 count = 1;
    char C = 0;
    const char *txt;

    switch (ch) {
    case 0xCB:
        ++pos;
        txt = MnemonicsCB[_RamRead(pos++)];
        count++;
        break;
    case 0xED:
        ++pos;
        txt = MnemonicsED[_RamRead(pos++)];
        count++;
        break;
    case 0xDD:
        ++pos;
        C = 'X';
        if (_RamRead(pos) != 0xCB) {
            txt = MnemonicsXX[_RamRead(pos++)];
            ++count;
        } else {
            ++pos;
            txt = MnemonicsXCB[_RamRead(pos++)];
            count += 2;
        }
        break;
    case 0xFD:
        ++pos;
        C = 'Y';
        if (_RamRead(pos) != 0xCB) {
            txt = MnemonicsXX[_RamRead(pos++)];
            ++count;
        } else {
            ++pos;
            txt = MnemonicsXCB[_RamRead(pos++)];
            count += 2;
        }
        break;
    default:
        /* Normal opcode */
        txt = Mnemonics[_RamRead(pos++)];
        break;
    }

    *posp = pos;     /* advanced to first operand byte (if any) */
    *countp = count; /* consumed opcode/prefix bytes so far */
    if (prefixp)
        *prefixp = C;
    return txt;
}

/* InstructionLength - compute the number of bytes used by the instruction at pos */
static uint8 InstructionLength(uint16 pos) {
    uint8 count = 0;
    uint8 initial;
    char C = 0;
    const char *txt;

    /* Get mnemonic and advance pos to the operand area. initial counts prefixes/opcode */
    txt = GetMnemonicAt(&pos, &initial, &C);
    count = initial;

    /* Walk the mnemonic-format string to count operand bytes */
    while (*txt != 0) {
        switch (*txt) {
        case '*': /* one immediate byte */
        case '^': /* one immediate byte */
            txt += 2;
            ++count;
            ++pos;
            break;
        case '#': /* two-byte immediate (word) */
            txt += 2;
            count += 2;
            pos += 2;
            break;
        case '@': /* relative displacement (one byte) */
            txt += 2;
            ++count;
            ++pos;
            break;
        case '%': /* prefix placeholder in mnemonic text, no operand */
            ++txt;
            break;
        default:
            ++txt;
        }
    }

    return count;
}

/* TextLength - compute the length of the text representation of the instruction at pos */
static uint8 TextLength(uint16 pos) {
    uint8 len = 0;
    const char *txt = GetMnemonicAt(&pos, &len, NULL);
    len = 0;
    while (*txt != 0) {
        switch (*txt) {
        case '*':
        case '^':
            txt += 2;
            len += 2; // 1 byte + 2 hex digits
            break;
        case '#':
            txt += 2;
            len += 4; // 2 bytes + 2 hex digits
            break;
        case '@':
            txt += 2;
            len += 4; // 1 byte + 2 hex digits
            break;
        case '%':
            ++txt;
            ++len;
            break;
        default:
            ++txt;
            ++len;
        }
    }
    return len;
}

/* Disassemble instruction at given address */
uint8 Disasm(uint16 pos) {
    /* New Disasm: print full opcode byte column, then mnemonic. */
    const char *txt;
    uint8 Cflag = 0;
    uint8 len = InstructionLength(pos);
    uint16 op_pos = pos;
    uint8 initial = 0;

    /* Print opcode bytes (up to len) */
    for (uint8 i = 0; i < len; ++i) {
        _puthex8(_RamRead((pos + i) & 0xffff));
        _putch(' ');
    }

    /* pad bytes area to fixed column (use 3 chars per byte, target 12 chars) */
    int bytes_width = (int)len * 3;
    int target = 12; /* enough for up to 4 bytes */
    for (int s = bytes_width; s < target; ++s)
        _putch(' ');

    /* Get mnemonic template (advances a temporary pos to operand start) */
    txt = GetMnemonicAt(&op_pos, &initial, (char *)&Cflag);

    /* Now print mnemonic with formatted operands. When we encounter operand markers
       we consume bytes from op_pos (which points to first operand byte). */
    while (*txt != 0) {
        switch (*txt) {
        case '*':
        case '^': {
            /* single byte immediate */
            txt += 2;
            uint8 v = _RamRead(op_pos++);
            _puthex8(v);
            break;
        }
        case '#': {
            /* word immediate: print as 16-bit hex (little-endian) */
            txt += 2;
            uint8 lo = _RamRead(op_pos);
            uint8 hi = _RamRead(op_pos + 1);
            uint16 w = (uint16)lo | ((uint16)hi << 8);
            op_pos += 2;
            _puthex16(w);
            break;
        }
        case '@': {
            /* relative displacement - show target address */
            char jr = (char)_RamRead(op_pos++);
            uint16 target = (op_pos + jr) & 0xffff;
            txt += 2;
            _puthex16(target);
            break;
        }
        case '%':
            _putch((char)Cflag);
            ++txt;
            break;
        default:
            _putch(*txt);
            ++txt;
        }
    }

    return (len);
}

/* --- Simple instruction trace buffer and exec breakpoints ---
   Implemented inline here to avoid changing platform Makefiles.
   Trace records last N executed instructions (pc, bytes, len, reg snapshot).
*/
#define TRACE_CAPACITY 512
typedef struct {
    uint16 pc;
    uint8 len;
    uint8 bytes[8];
    int32 AF, BC, DE, HL, IX, IY, SP;
} trace_entry_t;

static trace_entry_t trace_buf[TRACE_CAPACITY];
static uint32 trace_pos = 0; /* next write index */
static int trace_enabled = 1;

static void z80_trace_push(uint16 pc) {
    if (!trace_enabled)
        return;
    trace_entry_t *e = &trace_buf[trace_pos++ % TRACE_CAPACITY];
    uint8 len = InstructionLength(pc);
    if (len > 8)
        len = 8;
    e->pc = pc;
    e->len = len;
    for (uint8 i = 0; i < len; ++i) {
        e->bytes[i] = _RamRead((pc + i) & 0xffff);
    }
    e->AF = AF;
    e->BC = BC;
    e->DE = DE;
    e->HL = HL;
    e->IX = IX;
    e->IY = IY;
    e->SP = SP;
}

void z80_print_flags(uint16 AF) {
    static const char Flags[9] = "SZ5H3PNC";
    uint8 J, I;
    _puts(" [");
    for (J = 0, I = LOW_REGISTER(AF); J < 8; ++J, I <<= 1)
        _putcon(I & 0x80 ? Flags[J] : '.');
    _puts("]");
}

static void z80_trace_dump(void) {
    uint32 start = trace_pos;
    uint32 i;
    uint8 len;
    _puts("\r\n--- Trace dump (most recent last) ---\r\n");
    for (i = 0; i < TRACE_CAPACITY; ++i) {
        trace_entry_t *e = &trace_buf[(start + i) % TRACE_CAPACITY];
        /* skip empty entries (pc==0 and len==0) */
        if (e->len == 0 && e->pc == 0)
            continue;
        _puthex16(e->pc);
        _puts(": ");
        /* print disassembly for this address */
        Disasm(e->pc);
        len = TextLength(e->pc);
        /* pad to fixed column (target 16 chars) */
        int target = 16;
        for (int s = len; s < target; ++s)
            _putch(' ');
        _puts("BC:");
        _puthex16(e->BC);
        _puts(" DE:");
        _puthex16(e->DE);
        _puts(" HL:");
        _puthex16(e->HL);
        _puts(" AF:");
        _puthex16(e->AF);
        z80_print_flags(e->AF);
        _puts(" SP:");
        _puthex16(e->SP);
        _puts("\r\n");
    }
    _puts("--- end trace ---\r\n");
}

/* Exec breakpoints: small fixed-size list. Only the bp_addrs[] list is used. */
#define MAX_BREAKPOINTS 32
static uint16 bp_addrs[MAX_BREAKPOINTS];
static int bp_count = 0;

static int z80_add_breakpoint(uint16 addr) {
    /* prevent duplicates */
    for (int i = 0; i < bp_count; ++i)
        if (bp_addrs[i] == addr)
            return -2;
    if (bp_count >= MAX_BREAKPOINTS)
        return -1;
    bp_addrs[bp_count++] = addr;
    return 0;
}

static void z80_clear_breakpoints(void) {
    bp_count = 0;
}

static int z80_remove_breakpoint(uint16 addr) {
    for (int i = 0; i < bp_count; ++i) {
        if (bp_addrs[i] == addr) {
            /* shift remaining */
            for (int j = i; j + 1 < bp_count; ++j)
                bp_addrs[j] = bp_addrs[j + 1];
            --bp_count;
            return 0;
        }
    }
    return -1; /* not found */
}

static int z80_check_breakpoints_on_exec(uint16 pc) {
    for (int i = 0; i < bp_count; ++i)
        if (bp_addrs[i] == pc)
            return 1;
    return 0;
}

void Z80debug(void) {
    uint8 ch = 0;
    uint16 pos, l;
    uint8 I;
    uint16 bpoint; /* changed from unsigned int to 16-bit */
    uint8 loop = TRUE;
    int res = 0; /* use a signed int for result checks */

    _puts("\r\nDebug Mode - Press '?' for help");

    while (loop && Debug) {
        pos = PC;
        _puts("\r\n");
        _puts("BC:");
        _puthex16(BC);
        _puts(" DE:");
        _puthex16(DE);
        _puts(" HL:");
        _puthex16(HL);
        _puts(" AF:");
        _puthex16(AF);
        _puts(" :");
        z80_print_flags(AF);
        _puts("\r\n");
        _puts("IX:");
        _puthex16(IX);
        _puts(" IY:");
        _puthex16(IY);
        _puts(" SP:");
        _puthex16(SP);
        _puts(" PC:");
        _puthex16(PC);
        _puts(" : ");

        Disasm(pos);

        if (PC == 0x0005) {
            if (LOW_REGISTER(BC) > 40) {
                _puts(" (Unknown)");
            } else {
                _puts(" (");
                _puts(CPMCalls[LOW_REGISTER(BC)]);
                _puts(")");
            }
        }

        if (Watch != -1) {
            watchprint(Watch);
        }

        _puts("\r\n");
        _puts("Command|? : ");
        ch = _getcon();
        if (ch > 21 && ch < 127)
            _putch(ch);
        switch (ch) {
        case 't':
            /* Trace to next instruction */
            loop = FALSE;
            break;
        case 'c':
            /* Continue execution */
            loop = FALSE;
            _puts("\r\n");
            Debug = 0;
            break;
        case 'b':
            /* Dump memory pointed by (BC) */
            _puts("\r\n");
            memdump(BC);
            break;
        case 'd':
            /* Dump memory pointed by (DE) */
            _puts("\r\n");
            memdump(DE);
            break;
        case 'h':
            /* Dump memory pointed by (HL) */
            _puts("\r\n");
            memdump(HL);
            break;
        case 'p':
            /* Dump memory page pointed by (PC) */
            _puts("\r\n");
            memdump(PC & 0xFF00);
            break;
        case 's':
            /* Dump memory page pointed by (SP) */
            _puts("\r\n");
            memdump(SP & 0xFF00);
            break;
        case 'x':
            /* Dump memory page pointed by (IX) */
            _puts("\r\n");
            memdump(IX & 0xFF00);
            break;
        case 'y':
            /* Dump memory page pointed by (IY) */
            _puts("\r\n");
            memdump(IY & 0xFF00);
            break;
        case 'a':
            /* Dump memory pointed by dmaAddr */
            _puts("\r\n");
            memdump(dmaAddr);
            break;
        case 'l':
            /* Disassemble from current PC */
            _puts("\r\n");
            I = 16;
            l = pos;
            while (I > 0) {
                _puthex16(l);
                _puts(" : ");
                l += Disasm(l);
                _puts("\r\n");
                --I;
            }
            break;
        case 'A':
            /* Add breakpoint at address */
            _puts(" Addr: ");
            res = read_hex16(&bpoint);
            if (res) {
                if (z80_add_breakpoint(bpoint) == 0) {
                    _puts("Breakpoint added: ");
                    _puthex16(bpoint);
                    _puts("\r\n");
                } else {
                    _puts("Breakpoint list full\r\n");
                }
            } else {
                _puts("Invalid address\r\n");
            }
            break;
        case 'B':
            /* List breakpoints set via 'A' */
            _puts(" Breakpoints:\r\n");
            if (bp_count == 0) {
                _puts("  (none)\r\n");
            } else {
                for (int i = 0; i < bp_count; ++i) {
                    uint16 a = bp_addrs[i];
                    _puthex16(a);
                    _puts(" : ");
                    Disasm(a);
                    _puts("\r\n");
                }
            }
            break;
        case 'C':
            /* Clear all breakpoints */
            z80_clear_breakpoints();
            _puts(" Breakpoints cleared\r\n");
            break;
        case 'D':
            /* Dump memory at address */
            _puts(" Addr: ");
            res = read_hex16(&bpoint);
            if (res)
                memdump(bpoint);
            else
                _puts("Invalid address\r\n");
            break;
        case 'E':
            /* Erase breakpoint at address */
            _puts(" Addr: ");
            res = read_hex16(&bpoint);
            if (res) {
                if (z80_remove_breakpoint(bpoint) == 0) {
                    _puts("Breakpoint removed: ");
                    _puthex16(bpoint);
                    _puts("\r\n");
                } else {
                    _puts("Breakpoint not found\r\n");
                }
            } else {
                _puts("Invalid address\r\n");
            }
            break;
        case 'J':
            /* Jump - set PC to address */
            _puts(" Addr: ");
            res = read_hex16(&bpoint);
            if (res) {
                PC = bpoint;
                _puts("PC set to ");
                _puthex16(PC);
                _puts("\r\n");
            } else {
                _puts("Invalid address\r\n");
            }
            break;
        case 'L':
            /* Disassemble at address */
            _puts(" Addr: ");
            res = read_hex16(&bpoint);
            if (res) {
                I = 16;
                l = bpoint;
                while (I > 0) {
                    _puthex16(l);
                    _puts(" : ");
                    l += Disasm(l);
                    _puts("\r\n");
                    --I;
                }
            } else {
                _puts("Invalid address\r\n");
            }
            break;
        case 'M':
            /* Modify memory byte at address until Enter is pressed */
            _puts("\r\n Addr: ");
            res = read_hex16(&bpoint);
            if (res) {
                uint16 addr = bpoint;
                uint8 val;
                while (1) {
                    _puthex16(addr);
                    _puts(" : ");
                    val = _RamRead(addr);
                    _puthex8(val);
                    _puts(" -> ");
                    res = read_hex8(&val);
                    if (res) {
                        _RamWrite(addr, val);
                        addr = (addr + 1) & 0xFFFF;
                    } else {
                        break; /* exit on no input */
                    }
                }
            } else {
                _puts("Invalid address\r\n");
            }
            break;
        case 'R':
            /* Dump recent trace */
            z80_trace_dump();
            break;
        case 'T':
            /* Step over a call */
            loop = FALSE;
            Step = pos + InstructionLength(pos);
            Debug = 0;
            break;
        case 'U':
            /* Unwatch - clear any byte/word watch */
            Watch = -1;
            _puts("\r\nWatch cleared\r\n");
            break;
        case 'W':
            /* Watch - set a byte/word watch */
            _puts(" Addr: ");
            res = read_hex16(&bpoint);
            if (res) {
                Watch = bpoint;
                _puts("Watch set to ");
                _puthex16(Watch);
                _puts("\r\n");
            } else {
                _puts("Invalid address\r\n");
            }
            break;
        case 'X':
            /* Exit RunCPM */
            _puts("\r\nExiting...\r\n");
            Debug = 0;
            Status = 1;
            break;
        case '?':
            /* Help */
            _puts("\r\n");
            _puts("Lowercase commands:\r\n");
            _puts("  t - traces to the next instruction\r\n");
            _puts("  c - Continue execution\r\n");
            _puts("  b - Dumps memory pointed by (BC)\r\n");
            _puts("  d - Dumps memory pointed by (DE)\r\n");
            _puts("  h - Dumps memory pointed by (HL)\r\n");
            _puts("  p - Dumps the page (PC) points to\r\n");
            _puts("  s - Dumps the page (SP) points to\r\n");
            _puts("  x - Dumps the page (IX) points to\r\n");
            _puts("  y - Dumps the page (IY) points to\r\n");
            _puts("  a - Dumps memory pointed by dmaAddr\r\n");
            _puts("  l - Disassembles from current PC\r\n");
            _puts("Uppercase commands:\r\n");
            _puts("  A - Add breakpoint at address\r\n");
            _puts("  B - List breakpoints\r\n");
            _puts("  C - Clear all breakpoints\r\n");
            _puts("  D - Dumps memory at address\r\n");
            _puts("  E - Erase breakpoint at address\r\n");
            _puts("  J - Jumps to address (sets PC)\r\n");
            _puts("  L - Disassembles at address\r\n");
            _puts("  M - Modify memory at address\r\n");
            _puts("  R - Dump recent trace\r\n");
            _puts("  T - Steps over a call\r\n");
            _puts("  U - Clears the byte/word watch\r\n");
            _puts("  W - Sets a byte/word watch\r\n");
            _puts("  X - Exit RunCPM\r\n");
            break;
        default:
            _puts(" ???\r\n");
        }
    }
}

#endif // ifndef DEBUG_H