#ifndef POSIX_H
#define POSIX_H

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

/* Externals for abstracted functions need to go here */
FILE* _sys_fopen_r(uint8* filename);
int _sys_fseek(FILE* file, long delta, int origin);
long _sys_ftell(FILE* file);
long _sys_fread(void* buffer, long size, long count, FILE* file);
int _sys_fflush(FILE* file);
int _sys_fclose(FILE* file);

/* Memory abstraction functions */
/*===============================================================================*/
void _RamLoad(uint8* filename, uint16 address) {
	long l;
	FILE* file = _sys_fopen_r(filename);
	_sys_fseek(file, 0, SEEK_END);
	l = _sys_ftell(file);

	_sys_fseek(file, 0, SEEK_SET);
	_sys_fread(_RamSysAddr(address), 1, l, file); // (todo) This can overwrite past RAM space

	_sys_fclose(file);
}

/* Filesystem (disk) abstraction fuctions */
/*===============================================================================*/
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

uint8 _sys_exists(uint8* filename) {
	return(!access((const char*)filename, F_OK));
}

FILE* _sys_fopen_r(uint8* filename) {
	return(fopen((const char*)filename, "rb"));
}

FILE* _sys_fopen_w(uint8* filename) {
	return(fopen((const char*)filename, "wb"));
}

FILE* _sys_fopen_rw(uint8* filename) {
	return(fopen((const char*)filename, "r+b"));
}

FILE* _sys_fopen_a(uint8* filename) {
	return(fopen((const char*)filename, "a"));
}

int _sys_fseek(FILE* file, long delta, int origin) {
	return(fseek(file, delta, origin));
}

long _sys_ftell(FILE* file) {
	return(ftell(file));
}

long _sys_fread(void* buffer, long size, long count, FILE* file) {
	return(fread(buffer, size, count, file));
}

long _sys_fwrite(const void* buffer, long size, long count, FILE* file) {
	return(fwrite(buffer, size, count, file));
}

int _sys_fputc(int ch, FILE* file) {
	return(fputc(ch, file));
}

int _sys_feof(FILE* file) {
	return(feof(file));
}

int _sys_fflush(FILE* file) {
	return(fflush(file));
}

int _sys_fclose(FILE* file) {
	return(fclose(file));
}

int _sys_remove(uint8* filename) {
	return(remove((const char*)filename));
}

int _sys_rename(uint8* name1, uint8* name2) {
	return(rename((const char*)name1, (const char*)name2));
}

int _sys_select(uint8* disk) {
	struct stat st;
	return((stat((char*)disk, &st) == 0) && ((st.st_mode & S_IFDIR) != 0));
}

long _sys_filesize(uint8* filename) {
	long l = -1;
	FILE* file = _sys_fopen_r(filename);
	if (file != NULL) {
		_sys_fseek(file, 0, SEEK_END);
		l = _sys_ftell(file);
		_sys_fclose(file);
	}
	return(l);
}

int _sys_openfile(uint8* filename) {
	FILE* file = _sys_fopen_r(filename);
	if (file != NULL)
		_sys_fclose(file);
	return(file != NULL);
}

int _sys_makefile(uint8* filename) {
	FILE* file = _sys_fopen_a(filename);
	if (file != NULL)
		_sys_fclose(file);
	return(file != NULL);
}

int _sys_deletefile(uint8* filename) {
	return(!_sys_remove(filename));
}

int _sys_renamefile(uint8* filename, uint8* newname) {
	return(!_sys_rename(&filename[0], &newname[0]));
}

#ifdef DEBUGLOG
void _sys_logbuffer(uint8* buffer) {
	FILE* file;
#ifdef CONSOLELOG
	puts((char*)buffer);
#else
	uint8 s = 0;
	while (*(buffer + s))	// Computes buffer size
		++s;
	file = _sys_fopen_a((uint8*)LogName);
	_sys_fwrite(buffer, 1, s, file);
	_sys_fclose(file);
#endif
}
#endif

uint8 _sys_readseq(uint8* filename, long fpos) {
	uint8 result = 0xff;
	uint8 bytesread;
	uint8 dmabuf[128];
	uint8 i;

	FILE* file = _sys_fopen_r(&filename[0]);
	if (file != NULL) {
		if (!_sys_fseek(file, fpos, 0)) {
			for (i = 0; i < 128; ++i)
				dmabuf[i] = 0x1a;
			bytesread = (uint8)_sys_fread(&dmabuf[0], 1, 128, file);
			if (bytesread) {
				for (i = 0; i < 128; ++i)
					_RamWrite(dmaAddr + i, dmabuf[i]);
			}
			result = bytesread ? 0x00 : 0x01;
		} else {
			result = 0x01;
		}
		_sys_fclose(file);
	} else {
		result = 0x10;
	}

	return(result);
}

uint8 _sys_writeseq(uint8* filename, long fpos) {
	uint8 result = 0xff;

	FILE* file = _sys_fopen_rw(&filename[0]);
	if (file != NULL) {
		if (!_sys_fseek(file, fpos, 0)) {
			if (_sys_fwrite(_RamSysAddr(dmaAddr), 1, 128, file))
				result = 0x00;
		} else {
			result = 0x01;
		}
		_sys_fclose(file);
	} else {
		result = 0x10;
	}

	return(result);
}

uint8 _sys_readrand(uint8* filename, long fpos) {
	uint8 result = 0xff;
	uint8 bytesread;
	uint8 dmabuf[128];
	uint8 i;
	long extSize;

	FILE* file = _sys_fopen_r(&filename[0]);
	if (file != NULL) {
		if (!_sys_fseek(file, fpos, 0)) {
			for (i = 0; i < 128; ++i)
				dmabuf[i] = 0x1a;
			bytesread = (uint8)_sys_fread(&dmabuf[0], 1, 128, file);
			if (bytesread) {
				for (i = 0; i < 128; ++i)
					_RamWrite(dmaAddr + i, dmabuf[i]);
			}
			result = bytesread ? 0x00 : 0x01;
		} else {
			if (fpos >= 65536L * 128) {
				result = 0x06;	// seek past 8MB (largest file size in CP/M)
			} else {
				_sys_fseek(file, 0, SEEK_END);
				extSize = _sys_ftell(file);
				// round file size up to next full logical extent
				extSize = 16384 * ((extSize / 16384) + ((extSize % 16384) ? 1 : 0));
				if (fpos < extSize)
					result = 0x01;	// reading unwritten data
				else
					result = 0x04; // seek to unwritten extent
			}
		}
		_sys_fclose(file);
	} else {
		result = 0x10;
	}

	return(result);
}

uint8 _sys_writerand(uint8* filename, long fpos) {
	uint8 result = 0xff;

	FILE* file = _sys_fopen_rw(&filename[0]);
	if (file != NULL) {
		if (!_sys_fseek(file, fpos, 0)) {
			if (_sys_fwrite(_RamSysAddr(dmaAddr), 1, 128, file))
				result = 0x00;
		} else {
			result = 0x06;
		}
		_sys_fclose(file);
	} else {
		result = 0x10;
	}

	return(result);
}

uint8 _Truncate(char* fn, uint8 rc) {
	uint8 result = 0x00;
	if (truncate(fn, rc * 128))
		result = 0xff;
	return(result);
}

void _MakeUserDir() {
	uint8 dFolder = cDrive + 'A';
	uint8 uFolder = toupper(tohex(userCode));

	uint8 path[4] = { dFolder, FOLDERCHAR, uFolder, 0 };

	mkdir((char*)path, S_IRUSR | S_IWUSR | S_IXUSR);
}

uint8 _sys_makedisk(uint8 drive) {
	uint8 result = 0;
	if (drive < 1 || drive>16) {
		result = 0xff;
	} else {
		uint8 dFolder = drive + '@';
		uint8 disk[2] = { dFolder, 0 };
		if (!mkdir((char*)disk, S_IRUSR | S_IWUSR | S_IXUSR)) {
			result = 0xfe;
		} else {
			uint8 path[4] = { dFolder, FOLDERCHAR, '0', 0 };
			mkdir((char*)path, S_IRUSR | S_IWUSR | S_IXUSR);
		}
	}

	return(result);
}

#ifdef HASLUA
uint8 _RunLuaScript(char* filename) {

	L = luaL_newstate();
	luaL_openlibs(L);

	// Register Lua functions
	lua_register(L, "BdosCall", luaBdosCall);
	lua_register(L, "RamRead", luaRamRead);
	lua_register(L, "RamWrite", luaRamWrite);
	lua_register(L, "RamRead16", luaRamRead16);
	lua_register(L, "RamWrite16", luaRamWrite16);
	lua_register(L, "ReadReg", luaReadReg);
	lua_register(L, "WriteReg", luaWriteReg);

	int result = luaL_loadfile(L, filename);
	if (result) {
		_puts(lua_tostring(L, -1));
	} else {
		result = lua_pcall(L, 0, LUA_MULTRET, 0);
		if (result)
			_puts(lua_tostring(L, -1));
	}

	lua_close(L);

	return(result);
}
#endif

#ifndef POLLRDBAND
#define POLLRDBAND 0
#endif
#ifndef POLLRDNORM
#define POLLRDNORM 0
#endif


#endif