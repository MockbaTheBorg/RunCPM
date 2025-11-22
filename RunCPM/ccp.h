#ifndef CCP_H
#define CCP_H

// CP/M BDOS calls
#include "cpm.h"

// Memory Layout Definitions
#define CmdFCB      (BatchFCB + 48)     // FCB for use by internal commands
#define ParFCB      0x005C              // FCB for use by line parameters
#define SecFCB      0x006C              // Secondary part of FCB for renaming files
#define Trampoline  (CmdFCB + 36)       // Trampoline for running external commands

#define inBuf       (BDOSjmppage - 256) // Input buffer location
#define cmdLen      125                 // Maximum size of a command line (sz+rd+cmd+\0)

#define defDMA      0x0080              // Default DMA address
#define defLoad     0x0100              // Default load address

#define Internals                       // Define to have internal commands

// CCP Configuration and State
#define DEFAULT_PAGE_SIZE 22
#define PROMPT_SIZE       8
#define FCB_SIZE          36
#define SEC_SIZE          128
#define MAX_USER          15

// CCP global variables
uint8 pageSize = DEFAULT_PAGE_SIZE;     // for TYPE
uint8 currentDrive = 0;                 // 0 -> 15 = A -> P (Current drive for the CCP)
uint8 paramDrive = 0;                   // 0 -> 15 = A -> P (Drive for the first file parameter)
uint8 currentUser = 0;                  // 0 -> 15 (Current user area to access)
bool submitFlag = FALSE;                // Submit Flag
uint8 submitRecords = 0;                // Number of records on the Submit file
uint8 prompt[PROMPT_SIZE] = "\r\n  >";  // Command prompt
uint16 cmdBufferPtr, errorPtr;          // Pointer to the command buffer, and error position
uint8 bufferLen = 0;                    // Actual size of the typed command line

typedef struct {
    const char *name;
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

// --- New Helper Functions for Printing ---

// Helper to print a byte in Hex
void _ccp_printHex8(uint8 v) {
    uint8 nibble = v >> 4;
    _ccp_bdos(C_WRITE, nibble > 9 ? nibble + 55 : nibble + 48);
    nibble = v & 0x0F;
    _ccp_bdos(C_WRITE, nibble > 9 ? nibble + 55 : nibble + 48);
}

// Helper to print a word in Hex
void _ccp_printHex16(uint16 v) {
    _ccp_printHex8(v >> 8);
    _ccp_printHex8(v & 0xFF);
}

// Helper to print a number in Decimal
void _ccp_printDec(uint32 v) {
    char buf[10];
    uint8 i = 0;
    if (v == 0) {
        _ccp_bdos(C_WRITE, '0');
        return;
    }
    while (v) {
        buf[i++] = (v % 10) + '0';
        v /= 10;
    }
    while (i) {
        _ccp_bdos(C_WRITE, buf[--i]);
    }
}

// Compares two strings (Atmel doesn't like strcmp)
uint8 _ccp_strEqual(const char *stra, const char *strb) {
    while (*stra && *strb && (*stra == *strb)) {
        ++stra;
        ++strb;
    }
    return (*stra == *strb);
} // _ccp_strEqual

// Returns true if character is a delimiter
uint8 _ccp_delim(uint8 ch) {
    return (ch == 0 || ch == ' ' || ch == '=' || ch == '.' || ch == ':' ||
            ch == ';' || ch == '<' || ch == '>');
}

// Prints the FCB filename
void _ccp_printfcb(uint16 fcb, uint8 compact) {
    uint8 i, ch;

    ch = _RamRead(fcb);
    if (ch && compact) {
        _ccp_bdos(C_WRITE, ch + '@');
        _ccp_bdos(C_WRITE, ':');
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
// Parses a filename from the command buffer into an FCB
// Handles drive specifiers (A:), wildcards (*, ?), and extensions
uint8 _ccp_nameToFCB(uint16 fcb) {
    uint8 pad, plen, ch, n = 0;

    // Checks for a drive and places it on the Command FCB
    if (_RamRead(cmdBufferPtr + 1) == ':') {
        ch = toupper(_RamRead(cmdBufferPtr++));
        _RamWrite(fcb, ch - '@'); // Makes the drive 0x1-0xF for A-P
        ++cmdBufferPtr;           // Points cmdBufferPtr past the :
        bufferLen -= 2;
    }
    if (bufferLen) {
        ++fcb;

        // Parse filename (up to 8 chars)
        plen = 8;
        pad = ' ';
        ch = toupper(_RamRead(cmdBufferPtr));

        while (bufferLen && plen) {
            if (_ccp_delim(ch))
                break;
            ++cmdBufferPtr;
            --bufferLen;
            if (ch == '*')
                pad = '?';
            if (pad == '?') {
                ch = pad;
                n = n | 0x80; // Name is not unique
            }
            --plen;
            ++n;
            _RamWrite(fcb++, ch);
            ch = toupper(_RamRead(cmdBufferPtr));
        }

        // Pad remaining filename with spaces
        while (plen--)
            _RamWrite(fcb++, pad);
        
        // Parse extension (up to 3 chars)
        plen = 3;
        pad = ' ';
        if (ch == '.') {
            ++cmdBufferPtr;
            --bufferLen;
        }

        while (bufferLen && plen) {
            ch = toupper(_RamRead(cmdBufferPtr));
            if (_ccp_delim(ch))
                break;
            ++cmdBufferPtr;
            --bufferLen;
            if (ch == '*')
                pad = '?';
            if (pad == '?') {
                ch = pad;
                n = n | 0x80; // Name is not unique
            }
            --plen;
            ++n;
            _RamWrite(fcb++, ch);
        }

        // Pad remaining extension with spaces
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

// Asks for a key to continue, used by TYPE and LDIR
void _ccp_askForKey(void) {
    _puts("-- Press any key, ^C to quit --");
    _ccp_bios(B_CONIN);
    _puts("\r");
    _puts("                               \r");
}

#ifdef Internals
// DIR command - standard directory listing
uint8 _ccp_dir(void) {
    uint8 i;
    uint8 dirHead[6] = "A: ";
    uint8 dirSep[6] = "  |  ";
    uint32 ccount = 0; // Number of columns printed

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
                _puts("\r\n");
                _puts((char *)dirHead);
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

// LDIR command (Long DIR) - directory listing with file sizes and optional
// checksum
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
                if (ch != ' ')
                    len++;
                else
                    break;
            }
            len++; // .
            for (uint8 i = 9; i <= 11; ++i) {
                uint8 ch = _RamRead(tmpFCB + i);
                if (ch != ' ')
                    len++;
                else
                    break;
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
            // Optimized to remove sprintf
            uint32 temp = size;
            uint8 digits = 0;
            if (temp == 0) digits = 1;
            else {
                while (temp > 0) { temp /= 10; digits++; }
            }
            
            uint8 padding = 7 - digits;
            while (padding > 0) {
                _ccp_bdos(C_WRITE, ' ');
                padding--;
            }
            _ccp_printDec(size);
            _puts(" bytes");

            if (checksumOption) {
                // Compute checksum
                uint16 checksum = 0;
                CPM_FCB *F = (CPM_FCB *)_RamSysAddr(tmpFCB);
                F->ex = 0;
                F->cr = 0;
                if (!_ccp_bdos(F_OPEN, tmpFCB)) {
                    // Optimization: Use direct pointer if available for checksum
                    uint8* dmaPtr = (uint8*)_RamSysAddr(dmaAddr);
                    while (!_ccp_bdos(F_READ, tmpFCB)) {
                        for (uint8 i = 0; i < 128; ++i) {
                            checksum += dmaPtr[i];
                        }
                    }
                    _ccp_bdos(F_CLOSE, tmpFCB);
                }
                // Print checksum
                _puts("  ");
                _ccp_printHex16(checksum);
                _puts("h");
            }

            _puts("\r\n");
            l++;
            if (pageSize && (l == pageSize)) {
                l = 0;
                _ccp_askForKey();
                if (HIGH_REGISTER(AF) == 3)
                    break;
            }
        } while (!_SearchNext(ParFCB, TRUE));
    } else {
        _puts("No file");
    }
    return 0;
} // _ccp_ldir

// ERA command - erases files
uint8 _ccp_era(void) {
    if (_ccp_bdos(F_DELETE, ParFCB))
        _puts("\r\nNo file");
    return 0;
} // _ccp_era

// TYPE command - types a file to the console
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
                    if (pageSize && (l == pageSize)) {
                        l = 0;
                        _ccp_askForKey();
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

// SAVE command - saves memory pages to a file
uint8 _ccp_save(void) {
    uint8 error = TRUE;
    uint16 pages = _ccp_fcbtonum();
    uint16 i, dma;

    if (pages > 0 && pages < 256) {
        error = FALSE;

        while (_RamRead(cmdBufferPtr) == ' ' && bufferLen) { // Skips any leading spaces
            ++cmdBufferPtr;
            --bufferLen;
        }
        _ccp_nameToFCB(SecFCB); // Loads file name onto the ParFCB
        _puts("\r\n");
        if (_ccp_bdos(F_MAKE, SecFCB)) {
            _puts("Err: create");
        } else {
            if (_ccp_bdos(F_OPEN, SecFCB)) {
                _puts("Err: open");
            } else {
                pages *= 2; // Calculates the number of CP/M blocks to write
                dma = defLoad;

                for (i = 0; i < pages; i++) {
                    _ccp_bdos(F_DMAOFF, dma);
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

// REN command - renames a file
uint8 _ccp_ren(void) {
    uint8 ch, i;

    ++cmdBufferPtr;
    --bufferLen;

    _ccp_nameToFCB(SecFCB);

    for (i = 0; i < 12; ++i) { // Swap the filenames on the fcb
        ch = _RamRead(ParFCB + i);
        _RamWrite(ParFCB + i, _RamRead(SecFCB + i));
        _RamWrite(SecFCB + i, ch);
    }
    if (_ccp_bdos(F_RENAME, ParFCB))
        _puts("\r\nNo file");
    return 0;
} // _ccp_ren

// USER command - changes user area
uint8 _ccp_user(void) {
    uint8 error = TRUE;

    currentUser = (uint8)_ccp_fcbtonum();
    if (currentUser < 16) {
        _ccp_bdos(F_USERNUM, currentUser);
        error = FALSE;
    }
    return (error);
} // _ccp_user

// CLS command - clears the screen
uint8 _ccp_cls(void) {
    _clrscr();
    return (FALSE);
} // _ccp_cls

// EXIT command - terminates RunCPM
uint8 _ccp_exit(void) {
    _puts("\r\nTerminating RunCPM.");
    _puts("\r\nCPU Halted.");
    Status = STATUS_EXIT;
    return (FALSE);
} // _ccp_exit

// PAGE command - sets the paging size for TYPE and LDIR
uint8 _ccp_page(void) {
    uint8 error = TRUE;
    uint16 r = _ccp_fcbtonum();

    if (r < 256) {
        pageSize = (uint8)r;
        _puts("\r\nPage size set to ");
        _ccp_printDec(pageSize);
        _puts(" lines");
        _puts("\r\n");
        error = FALSE;
    }
    return (error);
} // _ccp_page

// VER command - displays the CCP version
uint8 _ccp_ver(void) {
    _puts(CCPHEAD);
    return (FALSE);
}

// DUMP command - dump memory or file in hex+ASCII, 128 bytes per screen, stop
// on ESC
uint8 _ccp_dump(void) {
    uint8 param[17];
    uint8 i = 0, c;
    uint16 addr = 0;
    uint8 isHex = 1;
    uint8 done = 0;

    // Extract parameter from ParFCB (filename or address)
    for (i = 1; i < 13; ++i) {
        c = _RamRead(ParFCB + i);
        if (c == ' ' || c == 0)
            break;
        param[i - 1] = c;
    }
    param[i - 1] = 0;

    // Check if parameter is exactly 4 hex digits
    if (i == 5) {
        for (uint8 j = 0; j < 4; ++j) {
            c = param[j];
            if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') ||
                  (c >= 'a' && c <= 'f'))) {
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
            if (c >= '0' && c <= '9')
                addr += c - '0';
            else if (c >= 'A' && c <= 'F')
                addr += c - 'A' + 10;
            else if (c >= 'a' && c <= 'f')
                addr += c - 'a' + 10;
        }
        // Dump memory
        _puts("\r\n");
        while (!done) {
            // Print address
            _ccp_printHex16(addr);
            _puts(": ");

            // Print hex bytes
            // Optimization: Use direct pointer if available
            uint8* ptr = (uint8*)_RamSysAddr(addr);
            for (i = 0; i < 16; ++i) {
                _ccp_printHex8(ptr[i]);
                _ccp_bdos(C_WRITE, ' ');
            }
            _puts(" ");
            // Print ASCII
            for (i = 0; i < 16; ++i) {
                uint8 b = ptr[i];
                _ccp_bdos(C_WRITE, (b >= 32 && b < 127) ? b : '.');
            }
            _puts("\r\n");
            addr += 16;
            if ((addr & 0x7F) == 0) { // Every 128 bytes, pause
                _ccp_askForKey();
                if (HIGH_REGISTER(AF) == 3) // ^C
                    done = 1;
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
            if (_ccp_bdos(F_READ, ParFCB))
                break;
            // Print 8 lines of 16 bytes
            // Optimization: Use direct pointer to DMA buffer
            uint8* dmaPtr = (uint8*)_RamSysAddr(dmaAddr);
            for (uint8 l = 0; l < 8; ++l) {
                // Print file offset (24-bit)
                _ccp_printHex8((faddr >> 16) & 0xFF);
                _ccp_printHex16(faddr & 0xFFFF);
                _puts(": ");
                
                // Print hex
                for (i = 0; i < 16; ++i) {
                    _ccp_printHex8(dmaPtr[l * 16 + i]);
                    _ccp_bdos(C_WRITE, ' ');
                }
                _puts(" ");
                // Print ASCII
                for (i = 0; i < 16; ++i) {
                    uint8 b = dmaPtr[l * 16 + i];
                    _ccp_bdos(C_WRITE, (b >= 32 && b < 127) ? b : '.');
                }
                _puts("\r\n");
                faddr += 16;
            }
            // Pause every 128 bytes
            _ccp_askForKey();
            if (HIGH_REGISTER(AF) == 3) // ^C
                done = 1;
        }
        return 0;
    }
} // _ccp_dump

// VOL command - shows the volume INFO.TXT information
uint8 _ccp_vol(void) {
    uint8 error = FALSE;
    uint8 letter = _RamRead(ParFCB) ? '@' + _RamRead(ParFCB) : 'A' + currentDrive;
    uint8 folder[5] = {letter, FOLDERCHAR, '0', FOLDERCHAR, 0};
    uint8 filename[13] = {letter, FOLDERCHAR, '0', FOLDERCHAR, 'I', 'N', 'F',
                          'O', '.', 'T', 'X', 'T', 0};
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
                    if ((_RamRead(dmaAddr + j) < 32) ||
                        (_RamRead(dmaAddr + j) > 126))
                        break;
                    _putcon(_RamRead(dmaAddr + j));
                }
            }
            _puts("\r\n");
        }
    }
    return (error);
} // _ccp_vol

// COPY command - copies a file
// Usage: COPY <src> <dest>
uint8 _ccp_copy(void) {
    if (_RamRead(ParFCB + 1) == ' ') {
        _puts("\r\nNo source");
        return 0;
    }
    if (_RamRead(SecFCB + 1) == ' ') {
        _puts("\r\nNo dest");
        return 0;
    }

    // Move SecFCB to CmdFCB to avoid corruption when opening ParFCB
    // ParFCB (0x5C) overlaps SecFCB (0x6C) when used as a full FCB
    for (uint8 i = 0; i < 16; ++i) {
        _RamWrite(CmdFCB + i, _RamRead(SecFCB + i));
    }
    // Initialize the rest of CmdFCB
    for (uint8 i = 16; i < 36; ++i) {
        _RamWrite(CmdFCB + i, 0);
    }

    if (_ccp_bdos(F_OPEN, ParFCB) == 255) {
        _puts("\r\nSource not found");
        return 0;
    }
    
    _ccp_bdos(F_DELETE, CmdFCB); // Delete dest if exists
    
    if (_ccp_bdos(F_MAKE, CmdFCB) == 255) {
        _puts("\r\nDir full");
        return 0;
    }
    
    if (_ccp_bdos(F_OPEN, CmdFCB) == 255) {
        _puts("\r\nErr open dest");
        return 0;
    }
    
    _puts("\r\nCopying...");
    
    // Use 128 byte buffer at defDMA
    while(TRUE) {
        _ccp_bdos(F_DMAOFF, defDMA);
        if (_ccp_bdos(F_READ, ParFCB) != 0) break; // EOF (or error, assumed EOF for now)
        if (_ccp_bdos(F_WRITE, CmdFCB) != 0) {
            _puts("\r\nDisk full");
            break;
        }
    }
    
    _ccp_bdos(F_CLOSE, CmdFCB);
    _puts(" Done.");
    return 0;
} // _ccp_copy

// ECHO command - prints text to console
// Usage: ECHO <text>
uint8 _ccp_echo(void) {
    // Find the start of the arguments in the input buffer
    // inBuf + 2 is the start of the command line
    uint16 ptr = inBuf + 2;
    
    // Skip leading spaces
    while (_RamRead(ptr) == ' ') ptr++;
    
    // Skip the command itself (ECHO)
    while (_RamRead(ptr) != ' ' && _RamRead(ptr) != 0) ptr++;
    
    // Skip spaces after command
    while (_RamRead(ptr) == ' ') ptr++;
    
    _puts("\r\n");
    
    // Print the rest
    while (_RamRead(ptr) != 0) {
        _ccp_bdos(C_WRITE, _RamRead(ptr++));
    }
    
    return 0;
} // _ccp_echo

// POKE command - writes a byte to memory
// Usage: POKE <addr> <val> (hex)
uint8 _ccp_poke(void) {
    uint16 addr = 0;
    uint16 val = 0;
    uint8 c, i;
    
    // Parse Address from ParFCB (first argument)
    for (i = 1; i <= 4; ++i) {
        c = _RamRead(ParFCB + i);
        if (c == ' ') break;
        addr <<= 4;
        if (c >= '0' && c <= '9') addr += (c - '0');
        else if (c >= 'A' && c <= 'F') addr += (c - 'A' + 10);
        else if (c >= 'a' && c <= 'f') addr += (c - 'a' + 10);
    }
    
    // Parse Value from SecFCB (second argument)
    for (i = 1; i <= 2; ++i) {
        c = _RamRead(SecFCB + i);
        if (c == ' ') break;
        val <<= 4;
        if (c >= '0' && c <= '9') val += (c - '0');
        else if (c >= 'A' && c <= 'F') val += (c - 'A' + 10);
        else if (c >= 'a' && c <= 'f') val += (c - 'a' + 10);
    }
    
    _RamWrite(addr, (uint8)val);
    _puts("\r\nOK");
    return 0;
} // _ccp_poke

#endif // Internals

// ?/Help command
uint8 _ccp_hlp(void) {
    _puts("\r\nCCP Commands:\r\n");
    _puts(" ?                  - Shows this list of commands\r\n");
    _puts(" CLS                - Clears the screen\r\n");
    _puts(" COPY <src> <dst>   - Copies a file\r\n");
    _puts(" DEL [<patt>]       - Alias to ERA\r\n");
    _puts(" DIR [<patt>]       - Lists file directory\r\n");
    _puts(" DUMP <addr|file>   - Hex+ASCII dump of memory or file\r\n");
    _puts("                      addr = 4 hex digits\r\n");
    _puts(" ECHO <text>        - Prints text to console\r\n");
    _puts(" ERA [<patt>]       - Erases files\r\n");
    _puts(" EXIT               - Terminates RunCPM\r\n");
    _puts(" LDIR [<patt>] [/C] - Lists file directory with sizes\r\n");
    _puts("                      /C option includes 16 bit checksum\r\n");
    _puts(" PAGE [<n>]         - Sets the paging size for TYPE and LDIR\r\n");
    _puts("                      n = 0 to 255, 0 disables paging\r\n");
    _puts(" POKE <addr> <val>  - Writes a byte to memory (hex)\r\n");
    _puts(" REN <new>=<old>    - Renames files\r\n");
    _puts(" SAVE <n> <file>    - Saves memory pages (256 bytes) to file\r\n");
    _puts(" TYPE <file>        - Displays file contents\r\n");
    _puts(" USER <n>           - Changes user area\r\n");
    _puts(" VER                - Displays the current CCP version\r\n");
    _puts(" VOL [<drv>]        - Shows the volume INFO.TXT information\r\n");
    return (FALSE);
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
    {"COPY", _ccp_copy},
    {"LDIR", _ccp_ldir},
    {"DEL", _ccp_era},
    {"ECHO", _ccp_echo},
    {"EXIT", _ccp_exit},
    {"PAGE", _ccp_page},
    {"POKE", _ccp_poke},
    {"VER", _ccp_ver},
    {"DUMP", _ccp_dump},
    {"VOL", _ccp_vol},
#endif
    {"?", _ccp_hlp},
    {NULL, NULL} // Sentinel
};

// Gets the command pointer
const Command *_ccp_cnum(void) {
    uint8 command[9];
    uint8 i = 0;

    if (!_RamRead(CmdFCB)) { // If a drive was set, then the command is external
        while (i < 8 && _RamRead(CmdFCB + i + 1) != ' ') {
            command[i] = _RamRead(CmdFCB + i + 1);
            ++i;
        }
        command[i] = 0;
        i = 0;
        while (Commands[i].name) {
            if (_ccp_strEqual((char *)command, Commands[i].name)) {
                errorPtr = defDMA + 2;
                return &Commands[i];
            }
            ++i;
        }
    }
    return NULL; // External command
} // _ccp_cnum

// External (.COM) command
uint8 _ccp_ext(void) {
    bool error = TRUE, found = FALSE;
    uint8 drive = 0, user = 0;
    uint16 loadAddr = defLoad;

    bool wasBlank = (_RamRead(CmdFCB + 9) == ' ');
    bool wasSUB =
        ((_RamRead(CmdFCB + 9) == 'S') && (_RamRead(CmdFCB + 10) == 'U') &&
         (_RamRead(CmdFCB + 11) == 'B'));

    if (!wasSUB) {
        if (wasBlank) {
            _RamWrite(CmdFCB + 9, 'C'); // first look for a .COM file
            _RamWrite(CmdFCB + 10, 'O');
            _RamWrite(CmdFCB + 11, 'M');
        }

        drive = _RamRead(CmdFCB);           // Get the drive from the command FCB
        found = !_ccp_bdos(F_OPEN, CmdFCB); // Look for the program on the FCB
                                            // drive, current or specified
        if (!found) {                       // If not found
            if (!drive) {                   // and the search was on the default drive
                _RamWrite(CmdFCB, 0x01);    // Then look on drive A: user 0
                if (currentUser) {
                    user = currentUser;           // Save the current user
                    _ccp_bdos(F_USERNUM, 0x0000); // then set it to 0
                }
                found = !_ccp_bdos(F_OPEN, CmdFCB);
                if (!found) {                               // If still not found then
                    if (currentUser) {                      // If current user not = 0
                        _RamWrite(CmdFCB, 0x00);            // look on current drive user 0
                        found = !_ccp_bdos(F_OPEN, CmdFCB); // and try again
                    }
                }
            }
        }
        if (!found) {
            _RamWrite(CmdFCB, drive);      // restore previous drive
            _ccp_bdos(F_USERNUM, currentUser); // restore to previous user
        }
    }

    // if .COM not found then look for a .SUB file
    if ((wasBlank || wasSUB) && !found &&
        !submitFlag) { // don't auto-submit while executing a submit file
        _RamWrite(CmdFCB + 9, 'S');
        _RamWrite(CmdFCB + 10, 'U');
        _RamWrite(CmdFCB + 11, 'B');

        drive = _RamRead(CmdFCB);           // Get the drive from the command FCB
        found = !_ccp_bdos(F_OPEN, CmdFCB); // Look for the program on the FCB
                                            // drive, current or specified
        if (!found) {                       // If not found
            if (!drive) {                   // and the search was on the default drive
                _RamWrite(CmdFCB, 0x01);    // Then look on drive A: user 0
                if (currentUser) {
                    user = currentUser;           // Save the current user
                    _ccp_bdos(F_USERNUM, 0x0000); // then set it to 0
                }
                found = !_ccp_bdos(F_OPEN, CmdFCB);
                if (!found) {                               // If still not found then
                    if (currentUser) {                      // If current user not = 0
                        _RamWrite(CmdFCB, 0x00);            // look on current drive user 0
                        found = !_ccp_bdos(F_OPEN, CmdFCB); // and try again
                    }
                }
            }
        }
        if (!found) {
            _RamWrite(CmdFCB, drive);      // restore previous drive
            _ccp_bdos(F_USERNUM, currentUser); // restore to previous user
        }

        if (found) {
            //_puts(".SUB file found!\n");
            int i;

            // move FCB's (CmdFCB --> ParFCB --> SecFCB)
            // Optimization: Use direct pointers for FCB copying
            uint8* secPtr = (uint8*)_RamSysAddr(SecFCB);
            uint8* parPtr = (uint8*)_RamSysAddr(ParFCB);
            uint8* cmdPtr = (uint8*)_RamSysAddr(CmdFCB);
            
            for (i = 0; i < 16; i++) {
                // ParFCB to SecFCB
                secPtr[i] = parPtr[i];
                // CmdFCB to ParFCB
                parPtr[i] = cmdPtr[i];
            }
            // (Re)Initialize the CmdFCB
            _ccp_initFCB(CmdFCB, 36);

            // put 'SUBMIT.COM' in CmdFCB
            const char *str = "SUBMIT  COM";
            int s = (int)strlen(str);
            for (i = 0; i < s; i++)
                _RamWrite(CmdFCB + i + 1, str[i]);

            // now try to find SUBMIT.COM file
            found =
                !_ccp_bdos(F_OPEN, CmdFCB);  // Look for the program on the FCB
                                             // drive, current or specified
            if (!found) {                    // If not found
                if (!drive) {                // and the search was on the default drive
                    _RamWrite(CmdFCB, 0x01); // Then look on drive A: user 0
                    if (currentUser) {
                        user = currentUser;           // Save the current user
                        _ccp_bdos(F_USERNUM, 0x0000); // then set it to 0
                    }
                    found = !_ccp_bdos(F_OPEN, CmdFCB);
                    if (!found) {      // If still not found then
                        if (currentUser) { // If current user not = 0
                            _RamWrite(CmdFCB,
                                      0x00);                    // look on current drive user 0
                            found = !_ccp_bdos(F_OPEN, CmdFCB); // and try again
                        }
                    }
                }
            }
            if (found) {
                // insert "@" into command buffer
                // note: this is so the rest will be parsed correctly
                bufferLen = _RamRead(defDMA);
                if (bufferLen < cmdLen) {
                    bufferLen++;
                    _RamWrite(defDMA, bufferLen);
                }
                uint8 lc = '@';
                for (i = 0; i < bufferLen; i++) {
                    uint8 nc = _RamRead(defDMA + 1 + i);
                    _RamWrite(defDMA + 1 + i, lc);
                    lc = nc;
                }
            }
        }
    }

    if (found) { // Program was found somewhere
        _puts("\r\n");
        _ccp_bdos(F_DMAOFF, loadAddr);       // Sets the DMA address for the loading
        while (!_ccp_bdos(F_READ, CmdFCB)) { // Loads the program into memory
            loadAddr += 128;
            if (loadAddr ==
                BDOSjmppage) { // Breaks if it reaches the end of TPA
                _puts("\r\nNo Memory");
                break;
            }
            _ccp_bdos(F_DMAOFF,
                      loadAddr); // Points the DMA offset to the next loadAddr
        }
        _ccp_bdos(F_DMAOFF,
                  defDMA); // Points the DMA offset back to the default

        if (user) {                        // If a user was selected
            _ccp_bdos(F_USERNUM, currentUser); // Set it back
            user = 0;
        }
        _RamWrite(CmdFCB,
                  drive); // Set the command FCB drive back to what it was
        cDrive = oDrive;  // And restore cDrive

        // Place a trampoline to call the external command
        // as it may return using RET instead of JP 0000h
        loadAddr = Trampoline;
        _RamWrite(loadAddr, CALL); // CALL 0100h
        _RamWrite16(loadAddr + 1, defLoad);
        _RamWrite(loadAddr + 3, JP); // JP USERF
        _RamWrite16(loadAddr + 4, BIOSjmppage + B_USERF);

        Z80reset(); // Resets the Z80 CPU
        SET_LOW_REGISTER(BC,
                         _RamRead(DSKByte)); // Sets C to the current drive/user
        PC = loadAddr;                       // Sets CP/M application jump point
        SP = BDOSjmppage;                    // Sets the stack to the top of the TPA

        Z80run(cpuDelayInstructions); // Starts Z80 simulation
        PC = 0;   // Resets the PC/SP after command execution
        SP = 0;

        error = FALSE;
    }

    if (user)                          // If a user was selected
        _ccp_bdos(F_USERNUM, currentUser); // Set it back
    _RamWrite(CmdFCB, drive);          // Set the command FCB drive back to what it was

    return (error);
} // _ccp_ext

// Prints a command error
void _ccp_cmdError() {
    uint8 ch;

    _puts("\r\n");
    while ((ch = _RamRead(errorPtr++))) {
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

    if (submitFlag) {                             // Are we running a submit?
        if (!submitRecords) {                        // Are we already counting?
            _ccp_bdos(F_OPEN, BatchFCB);     // Open the batch file
            submitRecords = _RamRead(BatchFCB + 15); // Gets its record count
        }
        --submitRecords;                         // Counts one less
        _RamWrite(BatchFCB + 32, submitRecords); // And sets to be the next read
        _ccp_bdos(F_DMAOFF, defDMA);     // Reset current DMA
        _ccp_bdos(F_READ, BatchFCB);     // And reads the last sector
        chars = _RamRead(defDMA);        // Then moves it to the input buffer

        for (i = 0; i <= chars; ++i)
            _RamWrite(inBuf + i + 1, _RamRead(defDMA + i));
        _RamWrite(inBuf + i + 1, 0);
        _puts((char *)_RamSysAddr(inBuf + 2));
        if (!submitRecords) {
            _ccp_bdos(F_DELETE, BatchFCB); // Deletes the submit file
            submitFlag = FALSE;                 // and clears the submit flag
        }
    } else {
        _ccp_bdos(C_READSTR, inBuf); // Reads the command line from console
        if (Debug)
            Z80run(cpuDelayInstructions);
    }
} // _ccp_readInput

// Parses the command line for drive/user changes (e.g., A:, 0:, A0:)
// Returns TRUE if a drive/user change was processed, FALSE otherwise
// Sets errorFlag if an invalid user was specified
bool _ccp_parseDriveUser(bool *errorFlag) {
    uint8 i;
    uint8 ch, tDrive = 0, tUser = currentUser, u = 0;
    bool isDriveUserCmd = FALSE;

    *errorFlag = FALSE;

    for (i = 0; i < bufferLen; i++) {
        ch = toupper(_RamRead(cmdBufferPtr + i));
        if ((ch >= 'A') && (ch <= 'P')) {
            if (tDrive) { // if we've already specified a new drive
                return FALSE; // not a DU: command
            } else {
                tDrive = ch - '@';
            }
        } else if ((ch >= '0') && (ch <= '9')) {
            tUser = u = (u * 10) + (ch - '0');
        } else if (ch == ':') {
            if (i == bufferLen - 1) {   // if we at the end of the command line
                if (tUser >= 16) { // if invalid user
                    *errorFlag = TRUE;
                    return FALSE;
                }
                if (tDrive != 0) {
                    cDrive = oDrive = tDrive - 1;
                    _RamWrite(DSKByte,
                              (_RamRead(DSKByte) & 0xf0) | cDrive);
                    _ccp_bdos(DRV_SET, cDrive);
                    if (Status)
                        currentDrive = 0;
                }
                if (tUser != currentUser) {
                    currentUser = tUser;
                    _ccp_bdos(F_USERNUM, currentUser);
                }
                return TRUE;
            }
            return FALSE;
        } else {   // invalid character
            return FALSE; // don't error; may be valid (non-DU:) command
        }
    }
    return FALSE;
}

// Main CCP code
void _ccp(void) {
    uint8 i;

    submitFlag = (bool)_ccp_bdos(DRV_ALLRESET, 0x0000);
    _ccp_bdos(DRV_SET, currentDrive);

    for (i = 0; i < 36; ++i) {
        _RamWrite(BatchFCB + i, _RamRead(tmpFCB + i));
    }

    // Loads an autoexec file if it exists and this is the first boot
    // The file contents are loaded at ccpAddr+8 up to 126 bytes then the size
    // loaded is stored at ccpAddr+7
    if (firstBoot && !submitFlag) {
        if (_sys_exists((uint8 *)AUTOEXEC)) {
            uint16 cmd = inBuf + 2;
            uint8 bytesread = (uint8)_RamLoad((uint8 *)AUTOEXEC, cmd, 125);
            bufferLen = 0;
            while (bufferLen < bytesread && _RamRead(cmd + bufferLen) > 31)
                bufferLen++;
            _RamWrite(cmd + bufferLen, 0x00);
            _RamWrite(--cmd, bufferLen);
        } else {
            bufferLen = 0;
        }
        if (BOOTONLY)
            firstBoot = FALSE;
    } else {
        _RamWrite(inBuf, 0);     // Clears the buffer
        _RamWrite(inBuf + 1, 0); // Clears the buffer
        bufferLen = 0;
    }

    while (TRUE) {
        currentDrive = (uint8)_ccp_bdos(DRV_GET, 0x0000);  // Get current drive
        currentUser = (uint8)_ccp_bdos(F_USERNUM, 0x00FF); // Get current user
        _RamWrite(DSKByte,
                  (currentUser << 4) + currentDrive); // Set user/drive on addr DSKByte

        paramDrive = currentDrive; // Initially the parameter drive is the same as the
                             // current drive

        sprintf((char *)prompt, "\r\n%c%u%c", 'A' + currentDrive, currentUser,
                submitFlag ? '$' : '>');
        if (!bufferLen) {
            _puts((char *)prompt);

            _RamWrite(inBuf,
                      cmdLen); // Sets the buffer size to read the command line
            _ccp_readInput();
            if (Status == STATUS_RETURN)
                Status = STATUS_RUNNING;
            bufferLen = _RamRead(inBuf + 1); // Obtains the number of bytes read
        }

        _ccp_bdos(F_DMAOFF, defDMA); // Reset current DMA
        if (bufferLen) {
            _RamWrite(inBuf + 2 + bufferLen,
                      0);     // "Closes" the read buffer with a \0
            cmdBufferPtr = inBuf + 2; // Points cmdBufferPtr to the first command character

            while (_RamRead(cmdBufferPtr) == ' ' && bufferLen) { // Skips any leading spaces
                ++cmdBufferPtr;
                --bufferLen;
            }
            if (!bufferLen) // There were only spaces
                continue;
            if (_RamRead(cmdBufferPtr) == ';') { // Found a comment line
                bufferLen = 0;                // Ignore the rest of the line
                continue;
            }

            // parse for DU: command line shortcut
            bool errorFlag = FALSE;
            if (_ccp_parseDriveUser(&errorFlag)) {
                bufferLen = 0; // ignore the rest of the line
                continue;
            }
            if (errorFlag) {
                _ccp_cmdError(); // print command error
                bufferLen = 0;        // ignore the rest of the line
                continue;
            }

            _ccp_initFCB(CmdFCB, 36); // Initializes the command FCB

            errorPtr = cmdBufferPtr; // Saves the pointer in case there's an error
            if (_ccp_nameToFCB(CmdFCB) >
                8) {             // Extracts the command from the buffer
                _ccp_cmdError(); // Command name cannot be non-unique or have an
                                 // extension
                bufferLen = 0;        // ignore the rest of the line
                continue;
            }
            _RamWrite(defDMA,
                      bufferLen); // Move the command line at this point to 0x0080

            for (i = 0; i < bufferLen; ++i)
                _RamWrite(defDMA + i + 1, toupper(_RamRead(cmdBufferPtr + i)));
            while (i++ < 127) // "Zero" the rest of the DMA buffer
                _RamWrite(defDMA + i, 0);
            _ccp_initFCB(ParFCB, 18); // Initializes the parameter FCB
            _ccp_initFCB(SecFCB, 18); // Initializes the secondary FCB

            while (_RamRead(cmdBufferPtr) == ' ' && bufferLen) { // Skips any leading spaces
                ++cmdBufferPtr;
                --bufferLen;
            }
            _ccp_nameToFCB(
                ParFCB); // Loads the next file parameter onto the parameter FCB

            while (_RamRead(cmdBufferPtr) == ' ' && bufferLen) { // Skips any leading spaces
                ++cmdBufferPtr;
                --bufferLen;
            }
            _ccp_nameToFCB(
                SecFCB); // Loads the next file parameter onto the secondary FCB

            i = FALSE; // Checks if the command is valid and executes

            const Command *cmd = _ccp_cnum();
            if (cmd) {
                i = cmd->handler();
            } else {
                i = _ccp_ext();
            }

            cDrive = oDrive = currentDrive; // Restore cDrive and oDrive
            if (i)
                _ccp_cmdError();
        }
        bufferLen = 0;
        if ((Status == STATUS_EXIT) || (Status == STATUS_RESTART))
            break;
    }
    _puts("\r\n");
} // _ccp

#endif // ifndef CCP_H
