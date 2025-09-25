#ifndef CCP_H
#define CCP_H

// CP/M BDOS calls
#include "cpm.h"

#define CmdFCB	(BatchFCB + 48)			// FCB for use by internal commands
#define ParFCB	0x005C					// FCB for use by line parameters
#define SecFCB	0x006C					// Secondary part of FCB for renaming files
#define Trampoline (CmdFCB + 36)		// Trampoline for running external commands

#define inBuf	(BDOSjmppage - 256)     // Input buffer location
#define cmdLen	125						// Maximum size of a command line (sz+rd+cmd+\0)

#define defDMA	0x0080					// Default DMA address
#define defLoad	0x0100					// Default load address

#define Internals                       // Define to have internal commands

// CCP global variables
uint8 pgSize = 22;              // for TYPE
uint8 curDrive = 0;             // 0 -> 15 = A -> P	.. Current drive for the CCP (same as RAM[DSKByte])
uint8 parDrive = 0;             // 0 -> 15 = A -> P .. Drive for the first file parameter
uint8 curUser = 0;              // 0 -> 15			.. Current user area to access
bool sFlag = FALSE;             // Submit Flag
uint8 sRecs = 0;                // Number of records on the Submit file
uint8 prompt[8] = "\r\n  >";
uint16 pbuf, perr;
uint8 blen = 0;                 // Actual size of the typed command line (size of the buffer)

typedef struct {
    const char* name;
    uint8 (*handler)(void);
} Command;

// Used to call BIOS from inside the CCP
void _ccp_bios(uint8 function) {
    SET_LOW_REGISTER(PCX, function);
    _Bios();
} // _ccp_bios

// Used to call BDOS from inside the CCP
uint16 _ccp_bdos(uint8 function, uint16 de) {
    SET_LOW_REGISTER(BC, function);
    DE = de;
    _Bdos();
    
    return (HL & 0xffff);
} // _ccp_bdos

// Compares two strings (Atmel doesn't like strcmp)
uint8 _ccp_strcmp(const char *stra, const char *strb) {
    while (*stra && *strb && (*stra == *strb)) {
        ++stra;
        ++strb;
    }
    return (*stra == *strb);
} // _ccp_strcmp

// Returns true if character is a delimiter
uint8 _ccp_delim(uint8 ch) {
    return (ch == 0 || ch == ' ' || ch == '=' || ch == '.' || ch == ':' || ch == ';' || ch == '<' || ch == '>');
}

// Prints the FCB filename
void _ccp_printfcb(uint16 fcb, uint8 compact) {
    uint8 i, ch;
    
    ch = _RamRead(fcb);
    if (ch && compact) {
        _ccp_bdos(C_WRITE,	ch + '@');
        _ccp_bdos(C_WRITE,	':');
    }
    
    for (i = 1; i < 12; ++i) {
        ch = _RamRead(fcb + i);
        if ((ch == ' ') && compact)
            continue;
        if (i == 9)
            _ccp_bdos(C_WRITE, compact ? '.' : ' ');
        _ccp_bdos(C_WRITE, ch);
    }
} // _ccp_printfcb

// Initializes the FCB
void _ccp_initFCB(uint16 address, uint8 size) {
    uint8 i;
    
    for (i = 0; i < size; ++i)
        _RamWrite(address + i, 0x00);
    
    for (i = 0; i < 11; ++i)
        _RamWrite(address + 1 + i, 0x20);
} // _ccp_initFCB

// Name to FCB
uint8 _ccp_nameToFCB(uint16 fcb) {
    uint8 pad, plen, ch, n = 0;
    
    // Checks for a drive and places it on the Command FCB
    if (_RamRead(pbuf + 1) == ':') {
        ch = toupper(_RamRead(pbuf++));
        _RamWrite(fcb, ch - '@');   // Makes the drive 0x1-0xF for A-P
        ++pbuf;                     // Points pbuf past the :
        blen -= 2;
    }
    if (blen) {
        ++fcb;
        
        plen = 8;
        pad = ' ';
        ch = toupper(_RamRead(pbuf));
        
        while (blen && plen) {
            if (_ccp_delim(ch))
                break;
            ++pbuf;
            --blen;
            if (ch == '*')
                pad = '?';
            if (pad == '?') {
                ch = pad;
                n = n | 0x80;       // Name is not unique
            }
            --plen;
            ++n;
            _RamWrite(fcb++, ch);
            ch = toupper(_RamRead(pbuf));
        }
        
        while (plen--)
            _RamWrite(fcb++, pad);
        plen = 3;
        pad = ' ';
        if (ch == '.') {
            ++pbuf;
            --blen;
        }
        
        while (blen && plen) {
            ch = toupper(_RamRead(pbuf));
            if (_ccp_delim(ch))
                break;
            ++pbuf;
            --blen;
            if (ch == '*')
                pad = '?';
            if (pad == '?') {
                ch = pad;
                n = n | 0x80;       // Name is not unique
            }
            --plen;
            ++n;
            _RamWrite(fcb++, ch);
        }
        
        while (plen--)
            _RamWrite(fcb++, pad);
    }
    return (n);
} // _ccp_nameToFCB

// Converts the ParFCB name to a number
uint16 _ccp_fcbtonum() {
    uint8 ch;
    uint16 n = 0;
    uint8 pos = ParFCB + 1;
    
    while (TRUE) {
        ch = _RamRead(pos++);
        if ((ch < '0') || (ch > '9')) {
            break;
        }
        n = (n * 10) + (ch - '0');
    }
    return (n);
} // _ccp_fcbtonum

#ifdef Internals
// DIR command
uint8 _ccp_dir(void) {
    uint8 i;
    uint8 dirHead[6] = "A: ";
    uint8 dirSep[6] = "  |  ";
    uint32 ccount = 0;  // Number of columns printed
    
    if (_RamRead(ParFCB + 1) == ' ')
        for (i = 1; i < 12; ++i)
            _RamWrite(ParFCB + i, '?');
    dirHead[0] = _RamRead(ParFCB) ? _RamRead(ParFCB) + '@' : prompt[2];
    
    _puts("\r\n");
    if (!_SearchFirst(ParFCB, TRUE)) {
        _puts((char *)dirHead);
        _ccp_printfcb(tmpFCB, FALSE);
        ++ccount;
        
        while (!_SearchNext(ParFCB, TRUE)) {
            if (!ccount) {
                _puts(	"\r\n");
                _puts(	(char *)dirHead);
            } else {
                _puts((char *)dirSep);
            }
            _ccp_printfcb(tmpFCB, FALSE);
            ++ccount;
            if (ccount > 3)
                ccount = 0;
        }
    } else {
        _puts("No file");
    }
    return 0;
} // _ccp_dir

// LDIR command (Long DIR)
uint8 _ccp_ldir(void) {
    uint8 checksumOption = 0;
    uint8 l = 0;
    
    // Check for /C option in ParFCB or SecFCB
    if ((_RamRead(ParFCB + 1) == '/' && _RamRead(ParFCB + 2) == 'C') ||
        (_RamRead(SecFCB + 1) == '/' && _RamRead(SecFCB + 2) == 'C')) {
        checksumOption = 1;
    }
    
    // If ParFCB has /C, set it to list all files
    if (_RamRead(ParFCB + 1) == '/' && _RamRead(ParFCB + 2) == 'C') {
        for (uint8 i = 1; i < 12; ++i)
            _RamWrite(ParFCB + i, '?');
    } else if (_RamRead(ParFCB + 1) == ' ') {
        // If no pattern specified, list all files
        for (uint8 i = 1; i < 12; ++i)
            _RamWrite(ParFCB + i, '?');
    }
    
    _puts("\r\n");
    if (!_SearchFirst(ParFCB, TRUE)) {
        do {
            _ccp_printfcb(tmpFCB, TRUE);
            // Calculate length of printed filename for alignment
            uint8 len = 2; // drive:
            for (uint8 i = 1; i <= 8; ++i) {
                uint8 ch = _RamRead(tmpFCB + i);
                if (ch != ' ') len++;
                else break;
            }
            len++; // .
            for (uint8 i = 9; i <= 11; ++i) {
                uint8 ch = _RamRead(tmpFCB + i);
                if (ch != ' ') len++;
                else break;
            }
            // Align to column 20
            uint8 target = 20;
            while (len < target) {
                _ccp_bdos(C_WRITE, ' ');
                len++;
            }
            // Get file size
            long fsize = _FileSize(tmpFCB);
            uint32 size = (fsize == -1) ? 0 : (uint32)fsize;
            // Print size in bytes, padded to 7 digits
            if (size == 0) {
                _puts("      0");
            } else {
                uint32 temp = size;
                char buf[12];
                uint8 idx = 0;
                while (temp > 0) {
                    buf[idx++] = '0' + (temp % 10);
                    temp /= 10;
                }
                // Pad with spaces to 7 digits
                uint8 digits = idx;
                uint8 padding = 7 - digits;
                while (padding > 0) {
                    _ccp_bdos(C_WRITE, ' ');
                    padding--;
                }
                // Print the digits
                while (idx > 0) {
                    _ccp_bdos(C_WRITE, buf[--idx]);
                }
            }
            _puts(" bytes");
            
            if (checksumOption) {
                // Compute checksum
                uint16 checksum = 0;
                CPM_FCB* F = (CPM_FCB*)_RamSysAddr(tmpFCB);
                F->ex = 0;
                F->cr = 0;
                if (!_ccp_bdos(F_OPEN, tmpFCB)) {
                    while (!_ccp_bdos(F_READ, tmpFCB)) {
                        for (uint8 i = 0; i < 128; ++i) {
                            checksum += _RamRead(dmaAddr + i);
                        }
                    }
                    _ccp_bdos(F_CLOSE, tmpFCB);
                }
                // Print checksum as 4 hex digits
                char cbuf[8];
                sprintf(cbuf, "  %04Xh", checksum);
                _puts(cbuf);
            }
            
            _puts("\r\n");
            l++;
            if (pgSize && (l == pgSize)) {
                l = 0;
                _ccp_bios(B_CONIN);
                if (HIGH_REGISTER(AF) == 3)
                    break;
            }
        } while (!_SearchNext(ParFCB, TRUE));
    } else {
        _puts("No file");
    }
    return 0;
} // _ccp_ldir

// ERA command
uint8 _ccp_era(void) {
    if (_ccp_bdos(F_DELETE, ParFCB))
        _puts("\r\nNo file");
    return 0;
} // _ccp_era

// TYPE command
uint8 _ccp_type(void) {
    uint8 i, c, l = 0, p = 0;
    uint16 a = 0;
    
    _puts("\r\n");
    if (!_ccp_bdos(F_OPEN, ParFCB)) {
        
        while (!_ccp_bdos(F_READ, ParFCB)) {
            i = 128;
            a = dmaAddr;
            
            while (i) {
                c = _RamRead(a);
                if (c == 0x1a)
                    break;
                _ccp_bdos(C_WRITE, c);
                if (c == 0x0a) {
                    ++l;
                    if (pgSize && (l == pgSize)) {
                        l = 0;
                        _ccp_bios(B_CONIN);
                        p = HIGH_REGISTER(AF);
                        if (p == 3)
                            break;
                    }
                }
                --i;
                ++a;
            }
            if (p == 3)
                break;
        }
    } else {
        _puts("No file");
    }
    return 0;
} // _ccp_type

// SAVE command
uint8 _ccp_save(void) {
    uint8 error = TRUE;
    uint16 pages = _ccp_fcbtonum();
    uint16 i, dma;
    
    if (pages > 0 && pages < 256) {
        error = FALSE;
        
        while (_RamRead(pbuf) == ' ' && blen) {     // Skips any leading spaces
            ++pbuf;
            --blen;
        }
        _ccp_nameToFCB(SecFCB);                     // Loads file name onto the ParFCB
        _puts("\r\n");
        if (_ccp_bdos(F_MAKE, SecFCB)) {
            _puts("Err: create");
        } else {
            if (_ccp_bdos(F_OPEN, SecFCB)) {
                _puts("Err: open");
            } else {
                pages *= 2;                         // Calculates the number of CP/M blocks to write
                dma = defLoad;
                
                for (i = 0; i < pages; i++) {
                    _ccp_bdos(F_DMAOFF,	dma);
                    _ccp_bdos(F_WRITE, SecFCB);
                    dma += 128;
                    if (i % 2)
                        _ccp_bdos(C_WRITE, '.');
                }
                _ccp_bdos(F_CLOSE, SecFCB);
            }
        }
    }
    return (error);
} // _ccp_save

// REN command
uint8 _ccp_ren(void) {
    uint8 ch, i;
    
    ++pbuf;
    --blen;
    
    _ccp_nameToFCB(SecFCB);
    
    for (i = 0; i < 12; ++i) {  // Swap the filenames on the fcb
        ch = _RamRead(ParFCB + i);
        _RamWrite(	ParFCB + i, _RamRead(SecFCB + i));
        _RamWrite(	SecFCB + i, ch);
    }
    if (_ccp_bdos(F_RENAME, ParFCB))
        _puts("\r\nNo file");
    return 0;
} // _ccp_ren

// USER command
uint8 _ccp_user(void) {
    uint8 error = TRUE;
    
    curUser = (uint8)_ccp_fcbtonum();
    if (curUser < 16) {
        _ccp_bdos(F_USERNUM, curUser);
        error = FALSE;
    }
    return (error);
} // _ccp_user

// CLS command
uint8 _ccp_cls(void) {
    _clrscr();
    return (FALSE);
} // _ccp_cls

// EXIT command
uint8 _ccp_exit(void) {
    _puts("\r\nTerminating RunCPM.");
    _puts("\r\nCPU Halted.");
    Status = STATUS_EXIT;
    return (FALSE);
} // _ccp_exit

// PAGE command
uint8 _ccp_page(void) {
    uint8 error = TRUE;
    uint16 r = _ccp_fcbtonum();
    char pbuf[6];
    
    if (r < 256) {
        pgSize = (uint8)r;
        sprintf(pbuf, "%d", pgSize);
        _puts("\r\nPage size set to ");
        _puts(pbuf);
        _puts(" lines");
        _puts("\r\n");
        error = FALSE;
    }
    return (error);
} // _ccp_page
#endif // Internals

// VER command
uint8 _ccp_ver(void) {
    _puts(CCPHEAD);
    return(FALSE);
}

// DUMP command: dump memory or file in hex+ASCII, 128 bytes per screen, stop on ESC
uint8 _ccp_dump(void) {
    uint8 param[17];
    uint8 i = 0, c;
    uint16 addr = 0;
    uint8 isHex = 1;
    uint8 done = 0;

    // Extract parameter from ParFCB (filename or address)
    for (i = 1; i < 13; ++i) {
        c = _RamRead(ParFCB + i);
        if (c == ' ' || c == 0) break;
        param[i - 1] = c;
    }
    param[i - 1] = 0;

    // Check if parameter is exactly 4 hex digits
    if (i == 5) {
        for (uint8 j = 0; j < 4; ++j) {
            c = param[j];
            if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) {
                isHex = 0;
                break;
            }
        }
    } else {
        isHex = 0;
    }

    if (isHex) {
        // Parse hex address
        addr = 0;
        for (uint8 j = 0; j < 4; ++j) {
            c = param[j];
            addr <<= 4;
            if (c >= '0' && c <= '9') addr += c - '0';
            else if (c >= 'A' && c <= 'F') addr += c - 'A' + 10;
            else if (c >= 'a' && c <= 'f') addr += c - 'a' + 10;
        }
        // Dump memory
        _puts("\r\n");
        while (!done) {
            // Print address
            char abuf[8];
            sprintf(abuf, "%04X: ", addr);
            _puts(abuf);

            // Print hex bytes
            for (i = 0; i < 16; ++i) {
                uint8 b = _RamRead(addr + i);
                char hbuf[4];
                sprintf(hbuf, "%02X ", b);
                _puts(hbuf);
            }
            _puts(" ");
            // Print ASCII
            for (i = 0; i < 16; ++i) {
                uint8 b = _RamRead(addr + i);
                _ccp_bdos(C_WRITE, (b >= 32 && b < 127) ? b : '.');
            }
            _puts("\r\n");
            addr += 16;
            if ((addr & 0x7F) == 0) { // Every 128 bytes, pause
                _puts("-- Press any key, ^C to quit --");
                if (_ccp_bdos(C_READ, 0) == 3) // ^C
                    done = 1;
                _puts("\r\n");
            }
        }
        return 0;
    } else {
        // Assume file, try to open
        if (_ccp_bdos(F_OPEN, ParFCB)) {
            _puts("\r\nNo file");
            return 0;
        }
        _puts("\r\n");
        uint32 faddr = 0;
        done = 0;
        while (!done) {
            // Read 128 bytes
            if (_ccp_bdos(F_READ, ParFCB)) break;
            // Print 8 lines of 16 bytes
            for (uint8 l = 0; l < 8; ++l) {
                // Print file offset
                char abuf[12];
                sprintf(abuf, "%06lX: ", (unsigned long)faddr);
                _puts(abuf);
                // Print hex
                for (i = 0; i < 16; ++i) {
                    uint8 b = _RamRead(dmaAddr + l * 16 + i);
                    char hbuf[4];
                    sprintf(hbuf, "%02X ", b);
                    _puts(hbuf);
                }
                _puts(" ");
                // Print ASCII
                for (i = 0; i < 16; ++i) {
                    uint8 b = _RamRead(dmaAddr + l * 16 + i);
                    _ccp_bdos(C_WRITE, (b >= 32 && b < 127) ? b : '.');
                }
                _puts("\r\n");
                faddr += 16;
            }
            // Pause every 128 bytes
            _puts("-- Press any key, ^C to quit --");
            if (_ccp_bdos(C_READ, 0) == 3) // ^C
                done = 1;
            _puts("\r\n");
        }
        return 0;
    }
} // _ccp_dump

// VOL command
uint8 _ccp_vol(void) {
    uint8 error = FALSE;
    uint8 letter = _RamRead(ParFCB) ? '@' + _RamRead(ParFCB) : 'A' + curDrive;
    uint8 folder[5] = {letter, FOLDERCHAR, '0', FOLDERCHAR, 0};
    uint8 filename[13] = {letter, FOLDERCHAR, '0', FOLDERCHAR, 'I', 'N', 'F', 'O', '.', 'T', 'X', 'T', 0};
    uint8 bytesread;
    uint8 i, j;
    
    _puts("\r\nVolumes on ");
    _putcon(folder[0]);
    _puts(":\r\n");
    
    for (i = 0; i < 16; ++i) {
        folder[2] = i < 10 ? i + 48 : i + 55;
        if (_sys_exists(folder)) {
            _putcon(i < 10 ? ' ' : '1');
            _putcon(i < 10 ? folder[2] : 38 + i);
            _puts(": ");
            filename[2] = i < 10 ? i + 48 : i + 55;
            bytesread = (uint8)_sys_readseq(filename, 0);
            if (!bytesread) {
                for (j = 0; j < 128; ++j) {
                    if ((_RamRead(dmaAddr + j) < 32) || (_RamRead(dmaAddr + j) > 126))
                        break;
                    _putcon(_RamRead(dmaAddr + j));
                }
            }
            _puts("\r\n");
        }
    }
    return (error);
} // _ccp_vol

// ?/Help command
uint8 _ccp_hlp(void) {
    _puts("\r\nCCP Commands:\r\n");
    _puts(" ?                  - Shows this list of commands\r\n");
    _puts(" CLS                - Clears the screen\r\n");
    _puts(" DEL [<patt>]       - Alias to ERA\r\n");
    _puts(" DIR [<patt>]       - Lists file directory\r\n");
    _puts(" DUMP <addr|file>   - Hex+ASCII dump of memory or file\r\n");
    _puts("                      addr = 4 hex digits\r\n");
    _puts(" ERA [<patt>]       - Erases files\r\n");
    _puts(" EXIT               - Terminates RunCPM\r\n");
    _puts(" LDIR [<patt>] [/C] - Lists file directory with sizes\r\n");
    _puts("                      /C option includes 16 bit checksum\r\n");
    _puts(" PAGE [<n>]         - Sets the paging size for TYPE and LDIR\r\n");
    _puts("                      n = 0 to 255, 0 disables paging\r\n");
    _puts(" REN <new>=<old>    - Renames files\r\n");
    _puts(" SAVE <n> <file>    - Saves memory pages (256 bytes) to file\r\n");
    _puts(" TYPE <file>        - Displays file contents\r\n");
    _puts(" USER <n>           - Changes user area\r\n");
    _puts(" VER                - Displays the current CCP version\r\n");
    _puts(" VOL [<drv>]        - Shows the volume INFO.TXT information\r\n");
    return(FALSE);
}

// List of CCP commands
static const Command Commands[] = {
#ifdef Internals
    // Standard CP/M commands
    {"DIR", _ccp_dir},
    {"ERA", _ccp_era},
    {"TYPE", _ccp_type},
    {"SAVE", _ccp_save},
    {"REN", _ccp_ren},
    {"USER", _ccp_user},
    
    // Extra CCP commands
    {"CLS", _ccp_cls},
    {"LDIR", _ccp_ldir},
    {"DEL", _ccp_era},
    {"EXIT", _ccp_exit},
    {"PAGE", _ccp_page},
    {"VER", _ccp_ver},
    {"DUMP", _ccp_dump}, // <-- Added here
#endif
    {"VOL", _ccp_vol},
    {"?", _ccp_hlp},
    {NULL, NULL}  // Sentinel
};

// Gets the command pointer
const Command* _ccp_cnum(void) {
    uint8 command[9];
    uint8 i = 0;

    if (!_RamRead(CmdFCB)) {    // If a drive was set, then the command is external
        while (i < 8 && _RamRead(CmdFCB + i + 1) != ' ') {
            command[i] = _RamRead(CmdFCB + i + 1);
            ++i;
        }
        command[i] = 0;
        i = 0;
        while (Commands[i].name) {
            if (_ccp_strcmp((char *)command, Commands[i].name)) {
                perr = defDMA + 2;
                return &Commands[i];
            }
            ++i;
        }
    }
    return NULL;  // External command
} // _ccp_cnum

// External (.COM) command
uint8 _ccp_ext(void) {
    bool error = TRUE, found = FALSE;
    uint8 drive = 0, user = 0;
    uint16 loadAddr = defLoad;

    bool wasBlank = (_RamRead(CmdFCB + 9) == ' ');
    bool wasSUB = ((_RamRead(CmdFCB + 9) == 'S') &&
                   (_RamRead(CmdFCB + 10) == 'U') &&
                   (_RamRead(CmdFCB + 11) == 'B'));

    if (!wasSUB) {
        if (wasBlank) {
            _RamWrite(CmdFCB + 9, 'C');                     //first look for a .COM file
            _RamWrite(CmdFCB + 10, 'O');
            _RamWrite(CmdFCB + 11, 'M');
        }

        drive = _RamRead(CmdFCB);                           // Get the drive from the command FCB
        found = !_ccp_bdos(F_OPEN, CmdFCB);                 // Look for the program on the FCB drive, current or specified
        if (!found) {                                       // If not found
            if (!drive) {                                   // and the search was on the default drive
                _RamWrite(CmdFCB, 0x01);                    // Then look on drive A: user 0
                if (curUser) {
                    user = curUser;                         // Save the current user
                    _ccp_bdos(F_USERNUM, 0x0000);           // then set it to 0
                }
                found = !_ccp_bdos(F_OPEN, CmdFCB);
                if (!found) {                               // If still not found then
                    if (curUser) {                          // If current user not = 0
                        _RamWrite(CmdFCB, 0x00);            // look on current drive user 0
                        found = !_ccp_bdos(F_OPEN, CmdFCB); // and try again
                    }
                }
            }
        }
        if (!found) {
            _RamWrite(CmdFCB, drive);                       // restore previous drive
            _ccp_bdos(F_USERNUM, curUser);                  // restore to previous user
        }
    }

    //if .COM not found then look for a .SUB file
    if ((wasBlank || wasSUB) && !found && !sFlag) {         //don't auto-submit while executing a submit file
        _RamWrite(CmdFCB + 9, 'S');
        _RamWrite(CmdFCB + 10, 'U');
        _RamWrite(CmdFCB + 11, 'B');
        
        drive = _RamRead(CmdFCB);                           // Get the drive from the command FCB
        found = !_ccp_bdos(F_OPEN, CmdFCB);                 // Look for the program on the FCB drive, current or specified
        if (!found) {                                       // If not found
            if (!drive) {                                   // and the search was on the default drive
                _RamWrite(CmdFCB, 0x01);                    // Then look on drive A: user 0
                if (curUser) {
                    user = curUser;                         // Save the current user
                    _ccp_bdos(F_USERNUM, 0x0000);           // then set it to 0
                }
                found = !_ccp_bdos(F_OPEN, CmdFCB);
                if (!found) {                               // If still not found then
                    if (curUser) {                          // If current user not = 0
                        _RamWrite(CmdFCB, 0x00);            // look on current drive user 0
                        found = !_ccp_bdos(F_OPEN, CmdFCB); // and try again
                    }
                }
            }
        }
        if (!found) {
            _RamWrite(CmdFCB, drive);                       // restore previous drive
            _ccp_bdos(F_USERNUM, curUser);                  // restore to previous user
        }

        if (found) {
            //_puts(".SUB file found!\n");
            int i;

            //move FCB's (CmdFCB --> ParFCB --> SecFCB)
            for (i = 0; i < 16; i++) {
                //ParFCB to SecFCB
                _RamWrite(SecFCB + i, _RamRead(ParFCB + i));
                //CmdFCB to ParFCB
                _RamWrite(ParFCB + i, _RamRead(CmdFCB + i));
            }
            // (Re)Initialize the CmdFCB
            _ccp_initFCB(CmdFCB, 36);
            
            //put 'SUBMIT.COM' in CmdFCB
            const char *str = "SUBMIT  COM";
            int s = (int)strlen(str);
            for (i = 0; i < s; i++)
                _RamWrite(CmdFCB + i + 1, str[i]);
            
            //now try to find SUBMIT.COM file
            found = !_ccp_bdos(F_OPEN, CmdFCB);                 // Look for the program on the FCB drive, current or specified
            if (!found) {                                       // If not found
                if (!drive) {                                   // and the search was on the default drive
                    _RamWrite(CmdFCB, 0x01);                    // Then look on drive A: user 0
                    if (curUser) {
                        user = curUser;                         // Save the current user
                        _ccp_bdos(F_USERNUM, 0x0000);           // then set it to 0
                    }
                    found = !_ccp_bdos(F_OPEN, CmdFCB);
                    if (!found) {                               // If still not found then
                        if (curUser) {                          // If current user not = 0
                            _RamWrite(CmdFCB, 0x00);            // look on current drive user 0
                            found = !_ccp_bdos(F_OPEN, CmdFCB); // and try again
                        }
                    }
                }
            }
            if (found) {
                //insert "@" into command buffer
                //note: this is so the rest will be parsed correctly
                blen = _RamRead(defDMA);
                if (blen < cmdLen) {
                    blen++;
                    _RamWrite(defDMA, blen);
                }
                uint8 lc = '@';
                for (i = 0; i < blen; i++) {
                    uint8 nc = _RamRead(defDMA + 1 + i);
                    _RamWrite(defDMA + 1 + i, lc);
                    lc = nc;
                }
            }
        }
    }

    if (found) {										// Program was found somewhere
        _puts("\r\n");
        _ccp_bdos(F_DMAOFF, loadAddr);					// Sets the DMA address for the loading
        while (!_ccp_bdos(F_READ, CmdFCB)) {			// Loads the program into memory
            loadAddr += 128;
            if (loadAddr == BDOSjmppage) {				// Breaks if it reaches the end of TPA
                _puts("\r\nNo Memory");
                break;
            }
            _ccp_bdos(F_DMAOFF, loadAddr);				// Points the DMA offset to the next loadAddr
        }
        _ccp_bdos(F_DMAOFF, defDMA);					// Points the DMA offset back to the default
        
        if (user) {										// If a user was selected
            _ccp_bdos(F_USERNUM, curUser);				// Set it back
            user = 0;
        }
        _RamWrite(CmdFCB, drive);						// Set the command FCB drive back to what it was
        cDrive = oDrive;								// And restore cDrive
        
        // Place a trampoline to call the external command
        // as it may return using RET instead of JP 0000h
        loadAddr = Trampoline;
        _RamWrite(loadAddr, CALL);						// CALL 0100h
        _RamWrite16(loadAddr + 1, defLoad);
        _RamWrite(loadAddr + 3, JP);					// JP USERF
        _RamWrite16(loadAddr + 4, BIOSjmppage + B_USERF);
        
        Z80reset();										// Resets the Z80 CPU
        SET_LOW_REGISTER(BC, _RamRead(DSKByte));		// Sets C to the current drive/user
        PC = loadAddr;									// Sets CP/M application jump point
        SP = BDOSjmppage;								// Sets the stack to the top of the TPA
        
        Z80run();										// Starts Z80 simulation
        
        error = FALSE;
    }

    if (user)											// If a user was selected
        _ccp_bdos(F_USERNUM, curUser);					// Set it back
    _RamWrite(CmdFCB, drive);							// Set the command FCB drive back to what it was

    return(error);
} // _ccp_ext

// Prints a command error
void _ccp_cmdError() {
    uint8 ch;
    
    _puts("\r\n");    
    while ((ch = _RamRead(perr++))) {
        if (ch == ' ')
            break;
        _ccp_bdos(C_WRITE, toupper(ch));
    }
    _puts("?\r\n");
} // _ccp_cmdError

// Reads input, either from the $$$.SUB or console
void _ccp_readInput(void) {
    uint8 i;
    uint8 chars;
    
    if (sFlag) {                                // Are we running a submit?
        if (!sRecs) {                           // Are we already counting?
            _ccp_bdos(F_OPEN, BatchFCB);        // Open the batch file
            sRecs = _RamRead(BatchFCB + 15);    // Gets its record count
        }
        --sRecs;                                // Counts one less
        _RamWrite(BatchFCB + 32, sRecs);        // And sets to be the next read
        _ccp_bdos(	F_DMAOFF,	defDMA);        // Reset current DMA
        _ccp_bdos(	F_READ,		BatchFCB);      // And reads the last sector
        chars = _RamRead(defDMA);               // Then moves it to the input buffer
        
        for (i = 0; i <= chars; ++i)
            _RamWrite(inBuf + i + 1, _RamRead(defDMA + i));
        _RamWrite(inBuf + i + 1, 0);
        _puts((char *)_RamSysAddr(inBuf + 2));
        if (!sRecs) {
            _ccp_bdos(F_DELETE, BatchFCB);      // Deletes the submit file
            sFlag = FALSE;                      // and clears the submit flag
        }
    } else {
        _ccp_bdos(C_READSTR, inBuf);            // Reads the command line from console
        if (Debug)
            Z80run();
    }
} // _ccp_readInput

// Main CCP code
void _ccp(void) {
    uint8 i;
    
    sFlag = (bool)_ccp_bdos(DRV_ALLRESET, 0x0000);
    _ccp_bdos(DRV_SET, curDrive);
    
    for (i = 0; i < 36; ++i) {
        _RamWrite(BatchFCB + i, _RamRead(tmpFCB + i));
    }

	// Loads an autoexec file if it exists and this is the first boot
	// The file contents are loaded at ccpAddr+8 up to 126 bytes then the size loaded is stored at ccpAddr+7
	if (firstBoot && !sFlag) {
        if (_sys_exists((uint8*)AUTOEXEC)) {
            uint16 cmd = inBuf + 2;
            uint8 bytesread = (uint8)_RamLoad((uint8*)AUTOEXEC, cmd, 125);
            blen = 0;
            while (blen < bytesread && _RamRead(cmd + blen) > 31)
                blen++;
            _RamWrite(cmd + blen, 0x00);
            _RamWrite(--cmd, blen);
        } else {
            blen = 0;
        }
        if (BOOTONLY)
            firstBoot = FALSE;
    } else {
        _RamWrite(inBuf, 0);                        // Clears the buffer
        _RamWrite(inBuf + 1, 0);                    // Clears the buffer
        blen = 0;
    }

    while (TRUE) {
        curDrive = (uint8)_ccp_bdos(DRV_GET, 0x0000);   // Get current drive
        curUser = (uint8)_ccp_bdos(F_USERNUM, 0x00FF);  // Get current user
        _RamWrite(DSKByte, (curUser << 4) + curDrive);  // Set user/drive on addr DSKByte
        
        parDrive = curDrive;                            // Initially the parameter drive is the same as the current drive

        sprintf((char *) prompt, "\r\n%c%u%c", 'A' + curDrive, curUser, sFlag ? '$' : '>');
        if(!blen){
            _puts((char *)prompt);

            _RamWrite(inBuf, cmdLen);                   // Sets the buffer size to read the command line
            _ccp_readInput();
            if (Status == STATUS_RETURN)
                Status = STATUS_RUNNING;
            blen = _RamRead(inBuf + 1);                 // Obtains the number of bytes read
        }

        _ccp_bdos(F_DMAOFF, defDMA);                    // Reset current DMA
        if (blen) {
            _RamWrite(inBuf + 2 + blen, 0);             // "Closes" the read buffer with a \0
            pbuf = inBuf + 2;                           // Points pbuf to the first command character
            
            while (_RamRead(pbuf) == ' ' && blen) {     // Skips any leading spaces
                ++pbuf;
                --blen;
            }
            if (!blen)                                  // There were only spaces
                continue;
            if (_RamRead(pbuf) == ';') {                // Found a comment line
                blen = 0;                               // Ignore the rest of the line
                continue;
            }
            
            // parse for DU: command line shortcut
            bool errorFlag = FALSE, continueFlag = FALSE;
            uint8 ch, tDrive = 0, tUser = curUser, u = 0;
            
            for (i = 0; i < blen; i++) {
                ch = toupper(_RamRead(pbuf + i));
                if ((ch >= 'A') && (ch <= 'P')) {
                    if (tDrive) {                       // if we've already specified a new drive
                        break;                          // not a DU: command
                    } else {
                        tDrive = ch - '@';
                    }
                } else if ((ch >= '0') && (ch <= '9')) {
                    tUser = u = (u * 10) + (ch - '0');
                } else if (ch == ':') {
                    if (i == blen - 1) {                // if we at the end of the command line
                        if (tUser >= 16) {              // if invalid user
                            errorFlag = TRUE;
                            break;
                        }
                        if (tDrive != 0) {
                            cDrive = oDrive = tDrive - 1;
                            _RamWrite(DSKByte, (_RamRead(DSKByte) & 0xf0) | cDrive);
                            _ccp_bdos(DRV_SET, cDrive);
                            if (Status)
                                curDrive = 0;
                        }
                        if (tUser != curUser) {
                            curUser = tUser;
                            _ccp_bdos(F_USERNUM, curUser);
                        }
                        continueFlag = TRUE;
                    }
                    break;
                } else {                                // invalid character
                    break;                              // don't error; may be valid (non-DU:) command
                }
            }
            if (errorFlag) {
                _ccp_cmdError();                        // print command error
                blen = 0;                               // ignore the rest of the line
                continue;
            }
            if (continueFlag) {
                blen = 0;                               // ignore the rest of the line
                continue;
            }
            _ccp_initFCB(CmdFCB, 36);                   // Initializes the command FCB
            
            perr = pbuf;                                // Saves the pointer in case there's an error
            if (_ccp_nameToFCB(CmdFCB) > 8) {           // Extracts the command from the buffer
                _ccp_cmdError();                        // Command name cannot be non-unique or have an extension
                blen = 0;                               // ignore the rest of the line
                continue;
            }
            _RamWrite(defDMA, blen);                    // Move the command line at this point to 0x0080
            
            for (i = 0; i < blen; ++i)
                _RamWrite(defDMA + i + 1, toupper(_RamRead(pbuf + i)));
            while (i++ < 127)                           // "Zero" the rest of the DMA buffer
                _RamWrite(defDMA + i, 0);
            _ccp_initFCB(ParFCB, 18);                   // Initializes the parameter FCB
            _ccp_initFCB(SecFCB, 18);                   // Initializes the secondary FCB
            
            while (_RamRead(pbuf) == ' ' && blen) {     // Skips any leading spaces
                ++pbuf;
                --blen;
            }
            _ccp_nameToFCB(ParFCB);                     // Loads the next file parameter onto the parameter FCB
            
            while (_RamRead(pbuf) == ' ' && blen) {     // Skips any leading spaces
                ++pbuf;
                --blen;
            }
            _ccp_nameToFCB(SecFCB);                     // Loads the next file parameter onto the secondary FCB
            
            i = FALSE;  // Checks if the command is valid and executes

const Command* cmd = _ccp_cnum();
if (cmd) {
    i = cmd->handler();
} else {
    i = _ccp_ext();
}

cDrive = oDrive = curDrive; // Restore cDrive and oDrive
if (i)
    _ccp_cmdError();
        }
        blen = 0;
        if ((Status == STATUS_EXIT) || (Status == STATUS_RESTART))
            break;
    }
    _puts("\r\n");
} // _ccp

#endif // ifndef CCP_H

