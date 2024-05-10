#ifndef ABSTRACT_H
#define ABSTRACT_H

#ifdef PROFILE
#define printf(a, b) Serial.println(b)
#endif

#if defined ARDUINO_SAM_DUE || defined ADAFRUIT_GRAND_CENTRAL_M4
#define HostOS 0x01
#endif
#if defined CORE_TEENSY
#define HostOS 0x04
#endif
#if defined ESP32
#define HostOS 0x05
#endif
#if defined _STM32_DEF_
#define HostOS 0x06
#endif

/* Memory abstraction functions */
/*===============================================================================*/
bool _RamLoad(char* filename, uint16 address) {
	File f;
	bool result = false;

	if (f = SD.open(filename, FILE_READ)) {
		while (f.available())
			_RamWrite(address++, f.read());
		f.close();
		result = true;
	}
	return(result);
}

/* Filesystem (disk) abstraction functions */
/*===============================================================================*/
File rootdir, userdir;
#define FOLDERCHAR '/'

typedef struct {
	uint8 dr;
	uint8 fn[8];
	uint8 tp[3];
	uint8 ex, s1, s2, rc;
	uint8 al[16];
	uint8 cr, r0, r1, r2;
} CPM_FCB;

typedef struct {
	uint8 dr;
	uint8 fn[8];
	uint8 tp[3];
	uint8 ex, s1, s2, rc;
	uint8 al[16];
} CPM_DIRENTRY;

static DirFat_t fileDirEntry;

bool _sys_exists(uint8* filename) {
	return(SD.exists((const char *)filename));
}

File _sys_fopen_w(uint8* filename) {
	return(SD.open((char*)filename, O_CREAT | O_WRITE));
}

int _sys_fputc(uint8 ch, File& f) {
	return(f.write(ch));
}

void _sys_fflush(File& f) {
	f.flush();
}

void _sys_fclose(File& f) {
	f.close();
}

int _sys_select(uint8* disk) {
	uint8 result = FALSE;
	File f;

	digitalWrite(LED, HIGH ^ LEDinv);
	if (f = SD.open((char*)disk, O_READ)) {
		if (f.isDirectory())
			result = TRUE;
		f.close();
	}
	digitalWrite(LED, LOW ^ LEDinv);
	return(result);
}

long _sys_filesize(uint8* filename) {
	long l = -1;
	File f;

	digitalWrite(LED, HIGH ^ LEDinv);
	if (f = SD.open((char*)filename, O_RDONLY)) {
		l = f.size();
		f.close();
	}
	digitalWrite(LED, LOW ^ LEDinv);
	return(l);
}

int _sys_openfile(uint8* filename) {
	File f;
	int result = 0;

	digitalWrite(LED, HIGH ^ LEDinv);
	f = SD.open((char*)filename, O_READ);
	if (f) {
		f.dirEntry(&fileDirEntry);
		f.close();
		result = 1;
	}
	digitalWrite(LED, LOW ^ LEDinv);
	return(result);
}

int _sys_makefile(uint8* filename) {
	File f;
	int result = 0;

	digitalWrite(LED, HIGH ^ LEDinv);
	f = SD.open((char*)filename, O_CREAT | O_WRITE);
	if (f) {
		f.close();
		result = 1;
	}
	digitalWrite(LED, LOW ^ LEDinv);
	return(result);
}

int _sys_deletefile(uint8* filename) {
	digitalWrite(LED, HIGH ^ LEDinv);
	return(SD.remove((char*)filename));
	digitalWrite(LED, LOW ^ LEDinv);
}

int _sys_renamefile(uint8* filename, uint8* newname) {
	File f;
	int result = 0;

	digitalWrite(LED, HIGH ^ LEDinv);
	f = SD.open((char*)filename, O_WRITE | O_APPEND);
	if (f) {
    if (f.rename((char*)newname)) {
			f.close();
			result = 1;
		}
	}
	digitalWrite(LED, LOW ^ LEDinv);
	return(result);
}

#ifdef DEBUGLOG
void _sys_logbuffer(uint8* buffer) {
#ifdef CONSOLELOG
	puts((char*)buffer);
#else
	File f;
	uint8 s = 0;
	while (*(buffer + s))	// Computes buffer size
		++s;
	if (f = SD.open(LogName, O_CREAT | O_APPEND | O_WRITE)) {
		f.write(buffer, s);
		f.flush();
		f.close();
	}
#endif
}
#endif

bool _sys_extendfile(char* fn, unsigned long fpos)
{
	uint8 result = true;
	File f;
	unsigned long i;

	digitalWrite(LED, HIGH ^ LEDinv);
	if (f = SD.open(fn, O_WRITE | O_APPEND)) {
		if (fpos > f.size()) {
			for (i = 0; i < f.size() - fpos; ++i) {
				if (f.write((uint8)0) != 1) {
					result = false;
					break;
				}
			}
		}
		f.close();
	} else {
		result = false;
	}
	digitalWrite(LED, LOW ^ LEDinv);
	return(result);
}

uint8 _sys_readseq(uint8* filename, long fpos) {
	uint8 result = 0xff;
	File f;
	uint8 bytesread;
	uint8 dmabuf[BlkSZ];
	uint8 i;

	digitalWrite(LED, HIGH ^ LEDinv);
	f = SD.open((char*)filename, O_READ);
	if (f) {
		if (f.seek(fpos)) {
			for (i = 0; i < BlkSZ; ++i)
				dmabuf[i] = 0x1a;
			bytesread = f.read(&dmabuf[0], BlkSZ);
			if (bytesread) {
				for (i = 0; i < BlkSZ; ++i)
					_RamWrite(dmaAddr + i, dmabuf[i]);
			}
			result = bytesread ? 0x00 : 0x01;
		} else {
			result = 0x01;
		}
		f.close();
	} else {
		result = 0x10;
	}
	digitalWrite(LED, LOW ^ LEDinv);
	return(result);
}

uint8 _sys_writeseq(uint8* filename, long fpos) {
	uint8 result = 0xff;
	File f;

	digitalWrite(LED, HIGH ^ LEDinv);
	if (_sys_extendfile((char*)filename, fpos))
		f = SD.open((char*)filename, O_RDWR);
	if (f) {
		if (f.seek(fpos)) {
			if (f.write(_RamSysAddr(dmaAddr), BlkSZ))
				result = 0x00;
		} else {
			result = 0x01;
		}
		f.close();
	} else {
		result = 0x10;
	}
	digitalWrite(LED, LOW ^ LEDinv);
	return(result);
}

uint8 _sys_readrand(uint8* filename, long fpos) {
	uint8 result = 0xff;
	File f;
	uint8 bytesread;
	uint8 dmabuf[BlkSZ];
	uint8 i;
	long extSize;

	digitalWrite(LED, HIGH ^ LEDinv);
	f = SD.open((char*)filename, O_READ);
	if (f) {
		if (f.seek(fpos)) {
			for (i = 0; i < BlkSZ; ++i)
				dmabuf[i] = 0x1a;
			bytesread = f.read(&dmabuf[0], BlkSZ);
			if (bytesread) {
				for (i = 0; i < BlkSZ; ++i)
					_RamWrite(dmaAddr + i, dmabuf[i]);
			}
			result = bytesread ? 0x00 : 0x01;
		} else {
			if (fpos >= 65536L * BlkSZ) {
				result = 0x06;	// seek past 8MB (largest file size in CP/M)
			} else {
				extSize = f.size();
				// round file size up to next full logical extent
				extSize = ExtSZ * ((extSize / ExtSZ) + ((extSize % ExtSZ) ? 1 : 0));
				if (fpos < extSize)
					result = 0x01;	// reading unwritten data
				else
					result = 0x04; // seek to unwritten extent
			}
		}
		f.close();
	} else {
		result = 0x10;
	}
	digitalWrite(LED, LOW ^ LEDinv);
	return(result);
}

uint8 _sys_writerand(uint8* filename, long fpos) {
	uint8 result = 0xff;
	File f;

	digitalWrite(LED, HIGH ^ LEDinv);
	if (_sys_extendfile((char*)filename, fpos)) {
		f = SD.open((char*)filename, O_RDWR);
	}
	if (f) {
		if (f.seek(fpos)) {
			if (f.write(_RamSysAddr(dmaAddr), BlkSZ))
				result = 0x00;
		} else {
			result = 0x06;
		}
		f.close();
	} else {
		result = 0x10;
	}
	digitalWrite(LED, LOW ^ LEDinv);
	return(result);
}

static uint8 findNextDirName[13];
static uint16 fileRecords = 0;
static uint16 fileExtents = 0;
static uint16 fileExtentsUsed = 0;
static uint16 firstFreeAllocBlock;

uint8 _findnext(uint8 isdir) {
	File f;
	uint8 result = 0xff;
	bool isfile;
	uint32 bytes;

	digitalWrite(LED, HIGH ^ LEDinv);
	if (allExtents && fileRecords) {
		_mockupDirEntry();
		result = 0;
	} else {
		while (f = userdir.openNextFile()) {
			f.getName((char*)&findNextDirName[0], 13);
			isfile = !f.isDirectory();
			bytes = f.size();
			f.dirEntry(&fileDirEntry);
			f.close();
			if (!isfile)
				continue;
			_HostnameToFCBname(findNextDirName, fcbname);
			if (match(fcbname, pattern)) {
				if (isdir) {
					// account for host files that aren't multiples of the block size
					// by rounding their bytes up to the next multiple of blocks
					if (bytes & (BlkSZ - 1)) {
						bytes = (bytes & ~(BlkSZ - 1)) + BlkSZ;
					}
					fileRecords = bytes / BlkSZ;
					fileExtents = fileRecords / BlkEX + ((fileRecords & (BlkEX - 1)) ? 1 : 0);
					fileExtentsUsed = 0;
					firstFreeAllocBlock = firstBlockAfterDir;
					_mockupDirEntry();
				} else {
					fileRecords = 0;
					fileExtents = 0;
					fileExtentsUsed = 0;
					firstFreeAllocBlock = firstBlockAfterDir;
				}
				_RamWrite(tmpFCB, filename[0] - '@');
				_HostnameToFCB(tmpFCB, findNextDirName);
				result = 0x00;
				break;
			}
		}
	}
	digitalWrite(LED, LOW ^ LEDinv);
	return(result);
}

uint8 _findfirst(uint8 isdir) {
	uint8 path[4] = { '?', FOLDERCHAR, '?', 0 };
	path[0] = filename[0];
	path[2] = filename[2];
	if (userdir)
		userdir.close();
	userdir = SD.open((char*)path); // Set directory search to start from the first position
	_HostnameToFCBname(filename, pattern);
	fileRecords = 0;
	fileExtents = 0;
	fileExtentsUsed = 0;
	return(_findnext(isdir));
}

uint8 _findnextallusers(uint8 isdir) {
	uint8 result = 0xFF;
	char dirname[13];
	bool done = false;

	while (!done) {
		while (!userdir) {
			userdir = rootdir.openNextFile();
			if (!userdir) {
				done = true;
				break;
			}
			userdir.getName(dirname, sizeof dirname);
			if (userdir.isDirectory() && strlen(dirname) == 1 && isxdigit(dirname[0])) {
				currFindUser = dirname[0] <= '9' ? dirname[0] - '0' : toupper(dirname[0]) - 'A' + 10;
				break;
			}
			userdir.close();
		}
		if (userdir) {
			result = _findnext(isdir);
			if (result) {
				userdir.close();
			} else {
				done = true;
			}
		} else {
			result = 0xFF;
			done = true;
		}
	}
	return result;
}

uint8 _findfirstallusers(uint8 isdir) {
	uint8 path[2] = { '?', 0 };

	path[0] = filename[0];
	if (rootdir)
		rootdir.close();
	if (userdir)
		userdir.close();
	rootdir = SD.open((char*)path); // Set directory search to start from the first position
	strcpy((char*)pattern, "???????????");
	if (!rootdir)
		return 0xFF;
	fileRecords = 0;
	fileExtents = 0;
	fileExtentsUsed = 0;
	return(_findnextallusers(isdir));
}

uint8 _Truncate(char* filename, uint8 rc) {
	File f;
	int result = 0;

	digitalWrite(LED, HIGH ^ LEDinv);
	f = SD.open((char*)filename, O_WRITE | O_APPEND);
	if (f) {
		if (f.truncate(rc * BlkSZ)) {
			f.close();
			result = 1;
		}
	}
	digitalWrite(LED, LOW ^ LEDinv);
	return(result);
}

void _MakeUserDir() {
	uint8 dFolder = cDrive + 'A';
	uint8 uFolder = toupper(tohex(userCode));

	uint8 path[4] = { dFolder, FOLDERCHAR, uFolder, 0 };

	digitalWrite(LED, HIGH ^ LEDinv);
	SD.mkdir((char*)path);
	digitalWrite(LED, LOW ^ LEDinv);
}

uint8 _sys_makedisk(uint8 drive) {
	uint8 result = 0;
	if (drive < 1 || drive>16) {
		result = 0xff;
	} else {
		uint8 dFolder = drive + '@';
		uint8 disk[2] = { dFolder, 0 };
		digitalWrite(LED, HIGH ^ LEDinv);
		if (!SD.mkdir((char*)disk)) {
			result = 0xfe;
		} else {
			uint8 path[4] = { dFolder, FOLDERCHAR, '0', 0 };
			SD.mkdir((char*)path);
		}
		digitalWrite(LED, LOW ^ LEDinv);
	}

	return(result);
}

/* Hardware abstraction functions */
/*===============================================================================*/
void _HardwareOut(const uint32 Port, const uint32 Value) {

}

uint32 _HardwareIn(const uint32 Port) {
	return 0;
}

// ***************************** Physical Device List **************************************
//CON: = TTY: CRT: BAT: UC1:
//RDR: = TTY: PTR: UR1: UR2:
//PUN: = TTY: PTP: UP1: UP2:
//LST: = TTY: CRT: LPT: UL1:

// TTY

void deviceTTYout(uint8_t ch)  // same as _putch 
{
  Serial1.write(ch); // same as _putch but with buffer overflow protection
  while (Serial1.availableForWrite() < 20) {
      delay(100); // at 9600 baud, ESP32 output buffer is 128 bytes, if stops when only 20 chars left in the output buffer, and pause 100ms, then have about 115 left as sending about 1 per ms.
  }
}

uint8_t deviceTTYavailable(void) {  // same as _kbhit
  return(Serial1.available());
}

uint8_t deviceTTYin(void) {  // same as _getch
  while (!Serial1.available());
  return(Serial1.read());
}

uint8_t deviceTTYinEcho(void) { // same as _getche
  uint8_t ch = deviceTTYin();
  deviceTTYout(ch);
  return(ch);
}

// CRT

void deviceCRTout(uint8_t ch)  // same as _putch 
{
  Serial.write(ch); // same as _putch but with buffer overflow protection
  while (Serial.availableForWrite() < 20) {
      delay(100); // at 9600 baud, ESP32 output buffer is 128 bytes, if stop when only 20 chars left in the output buffer, and pause 100ms, then have about 115 left as sending about 1 per ms.
  }
}

uint8_t deviceCRTavailable(void) {  // same as _kbhit
  return(Serial.available());
}

uint8_t deviceCRTin(void) {  // same as _getch
  while (!Serial.available());
  return(Serial.read());
}

uint8_t deviceCRTinEcho(void) { // same as _getche
  uint8_t ch = deviceCRTin();
  deviceCRTout(ch);
  return(ch);
}

// UR1 

uint8_t deviceUR1in(void) {
    uint8_t ch = 26;
    return ch;
}

// UP1 

void deviceUP1out(uint8_t ch) {
  // add code here
}

// BAT, console device, input and output and can see if characters waiting, input used by Kermit as the default input device

void deviceBATout(uint8_t ch) 
{
  Serial2.write(ch); // same as _putch but with buffer overflow protection
  while (Serial2.availableForWrite() < 20) {
      delay(100); // at 9600 baud, ESP32 output buffer is 128 bytes, if stop when only 20 chars left in the output buffer, and pause 100ms, then have about 115 left as sending about 1 per ms.
  }
}

uint8_t deviceBATavailable(void) {  
   return(Serial2.available());
}

uint8_t deviceBATin(void) {  
  while (!Serial2.available());
  return(Serial2.read());
}

uint8_t deviceBATinEcho(void) { 
  uint8_t ch = deviceBATin();
  deviceBATout(ch);
  return(ch);
}

// UC1, console device

void deviceUC1out(uint8_t ch) 
{
  // add code here
}

uint8_t deviceUC1available(void) {  
  return(1); // add code here
}

uint8_t deviceUC1in(void) {  
    return(26); // add code here
}

uint8_t deviceUC1inEcho(void) { 
  uint8_t ch = deviceUC1in();
  deviceUC1out(ch);
  return(ch);
}

// PTR, reader device

uint8_t devicePTRin(void) {
    return 26; // add code here
}

// UR2, reader device

uint8_t deviceUR2in(void) {
    return 26; // add code here
}

// PTP,  punch device

void devicePTPout(uint8_t ch) {
    Serial2.write(ch); // used by Kermit as the default output device
    while (Serial2.availableForWrite() < 20) {
      delay(100); // at 9600 baud, if stop when only 20 chars left, and pause 100ms, then have about 115 left, as sending about 1 per ms. Should not need to do this if using small packets with Kermit
    }
}

// UP2, punch device

void deviceUP2out(uint8_t ch) {
  // add code here - eg write to file
}

// UL1,  LST device

void deviceUL1out(uint8_t ch) {
  // add code here - eg write to file
}


// LPT,  LST device

void deviceLPTout(uint8_t ch) {
  // add code here
}

// *************************************

// redirection for CON RDR PUN and LST eg CONin, CONout, CONavailable directed to one of four places depending on IOBYTE. 

uint8_t iobyteCON()
{
  uint8_t iobyte;
  iobyte = _RamRead(0x0003);
  iobyte = iobyte & 0x03; // mask off 00000011
  return iobyte;
}

uint8_t iobyteRDR()
{
  uint8_t iobyte;
  iobyte = _RamRead(0x0003);
  iobyte = iobyte & 0x0C; // mask off 00001100
  iobyte = iobyte >> 2; // bitshift so low 2 bits
  return iobyte;
}

uint8_t iobytePUN()
{
  uint8_t iobyte;
  iobyte = _RamRead(0x0003);
  iobyte = iobyte & 0x30; // mask off 00110000
  iobyte = iobyte >> 4; // bitshift so low 2 bits
  return iobyte;
}

uint8_t iobyteLST()
{
  uint8_t iobyte;
  iobyte = _RamRead(0x0003);
  iobyte = iobyte & 0xC0; // mask off 11000000
  iobyte = iobyte >> 6; // bitshift so low 2 bits
  return iobyte;
}

//CON: = TTY: CRT: BAT: UC1:
void outCON(uint8_t ch)
{
  uint8_t iobyte;
  iobyte = iobyteCON(); // 00, 01 10, 11
  switch (iobyte) {
    case 0: deviceTTYout(ch);
            break;
    case 1: deviceCRTout(ch);
            break;
    case 2: deviceBATout(ch);
            break;
    case 3: deviceUC1out(ch);
            break;                                 
  }
}

uint8_t availableCON()
{
  uint8_t iobyte;
  uint8_t dataAvailable;
  iobyte = iobyteCON(); // 00, 01 10, 11
  switch (iobyte) {
    case 0: dataAvailable = deviceTTYavailable();
            break;
    case 1: dataAvailable = deviceCRTavailable();
            break;
    case 2: dataAvailable = deviceBATavailable();
            break;
    case 3: dataAvailable = deviceUC1available();
            break;                                 
  }
  return dataAvailable;
}

uint8_t inCON(void)
{
  uint8_t iobyte;
  uint8_t dataIn;
  iobyte = iobyteCON(); // 00, 01 10, 11
  switch (iobyte) {
    case 0: dataIn = deviceTTYin();
            break;
    case 1: dataIn = deviceCRTin();
            break;
    case 2: dataIn = deviceBATin();
            break;
    case 3: dataIn = deviceUC1in();
            break;                                 
  }
  return dataIn;
}

uint8_t echoCON()
{
  uint8_t ch;
  ch = inCON();
  outCON(ch);
  return(ch);
}

//RDR: = TTY: PTR: UR1: UR2:
uint8_t inRDR(void)
{
  uint8_t iobyte;
  uint8_t dataIn;
  iobyte = iobyteRDR(); // 00, 01 10, 11
  switch (iobyte) {
    case 0: dataIn = deviceTTYin();
            break;
    case 1: dataIn = devicePTRin();
            break;
    case 2: dataIn = deviceUR1in();
            break;
    case 3: dataIn = deviceUR2in();
            break;                                 
  }
  return dataIn;
}

//PUN: = TTY: PTP: UP1: UP2:
void outPUN(uint8_t ch)
{
  uint8_t iobyte;
  iobyte = iobytePUN(); // 00, 01 10, 11
  switch (iobyte) {
    case 0: deviceTTYout(ch);
            break;
    case 1: devicePTPout(ch);
            break;
    case 2: deviceUP1out(ch);
            break;
    case 3: deviceUP2out(ch);
            break;                                 
  }
}

//LST: = TTY: CRT: LPT: UL1:
void outLST(uint8_t ch)
{
  uint8_t iobyte;
  iobyte = iobyteLST(); // 00, 01 10, 11
  switch (iobyte) {
    case 0: deviceTTYout(ch);
            break;
    case 1: deviceCRTout(ch);
            break;
    case 2: deviceLPTout(ch);
            break;
    case 3: deviceUL1out(ch);
            break;                                 
  }
}

/* Console abstraction functions */
/*===============================================================================*/

uint8_t _kbhit(void) {
  //return(Serial.available());
  return availableCON();  // redirect based on iobyte
}

uint8_t _getch(void) {
  //while (!Serial.available());
  //return(Serial.read());
  return inCON(); // redirect based on iobyte
}

uint8_t _getche(void) {
  //uint8_t ch = _getch();
  //Serial.write(ch);
  //return(ch);
  return echoCON(); // redirect based on iobyte
}

void _putch(uint8_t ch) {
  //Serial.write(ch);
  outCON(ch); // redirect based on iobyte
}

void _clrscr(void) {
  Serial.println("\e[H\e[J"); // done once at startup so left as is
}

#endif
