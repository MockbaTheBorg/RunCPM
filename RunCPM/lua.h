#ifndef LUA_H
#define LUA_H

extern int32 PCX; /* external view of PC                          */
extern int32 AF;  /* AF register                                  */
extern int32 BC;  /* BC register                                  */
extern int32 DE;  /* DE register                                  */
extern int32 HL;  /* HL register                                  */
extern int32 IX;  /* IX register                                  */
extern int32 IY;  /* IY register                                  */
extern int32 PC;  /* program counter                              */
extern int32 SP;  /* SP register                                  */
extern int32 AF1; /* alternate AF register                        */
extern int32 BC1; /* alternate BC register                        */
extern int32 DE1; /* alternate DE register                        */
extern int32 HL1; /* alternate HL register                        */
extern int32 IFF; /* Interrupt Flip Flop                          */
extern int32 IR;  /* Interrupt (upper) / Refresh (lower) register */

// Lua "Trampoline" functions
int luaBdosCall(lua_State* L) {
	uint8 function = (uint8)luaL_checkinteger(L, 1);
	uint16 de = (uint16)luaL_checkinteger(L, 2);

	SET_LOW_REGISTER(BC, function);
	DE = de;
	_Bdos();
	uint16 result = HL & 0xffff;

	lua_pushinteger(L, result);
	return(1);
}

int luaRamRead(lua_State* L) {
	uint16 addr = (uint16)luaL_checkinteger(L, 1);

	uint8 result = _RamRead(addr);
	lua_pushinteger(L, result);
	return(1);
}

int luaRamWrite(lua_State* L) {
	uint16 addr = (uint16)luaL_checkinteger(L, 1);
	uint8 value = (uint8)luaL_checkinteger(L, 2);

	_RamWrite(addr, value);
	return(0);
}

int luaRamRead16(lua_State* L) {
	uint16 addr = (uint16)luaL_checkinteger(L, 1);

	uint16 result = _RamRead16(addr);
	lua_pushinteger(L, result);
	return(1);
}

int luaRamWrite16(lua_State* L) {
	uint16 addr = (uint16)luaL_checkinteger(L, 1);
	uint16 value = (uint8)luaL_checkinteger(L, 2);

	_RamWrite16(addr, value);
	return(0);
}

int luaReadReg(lua_State* L) {
	uint8 reg = (uint8)luaL_checkinteger(L, 1);
	uint16 result;

	switch (reg) {
	case 0:
		result = PCX & 0xffff;	break;	/* external view of PC                          */
	case 1:
		result = AF & 0xffff;	break;	/* AF register                                  */
	case 2:
		result = BC & 0xffff;	break;	/* BC register                                  */
	case 3:
		result = DE & 0xffff;	break;	/* DE register                                  */
	case 4:
		result = HL & 0xffff;	break;	/* HL register                                  */
	case 5:
		result = IX & 0xffff;	break;	/* IX register                                  */
	case 6:
		result = IY & 0xffff;	break;	/* IY register                                  */
	case 7:
		result = PC & 0xffff;	break;	/* program counter                              */
	case 8:
		result = SP & 0xffff;	break;	/* SP register                                  */
	case 9:
		result = AF1 & 0xffff;	break;	/* alternate AF register                        */
	case 10:
		result = BC1 & 0xffff;	break;	/* alternate BC register                        */
	case 11:
		result = DE1 & 0xffff;	break;	/* alternate DE register                        */
	case 12:
		result = HL1 & 0xffff;	break;	/* alternate HL register                        */
	case 13:
		result = IFF & 0xffff;	break;	/* Interrupt Flip Flop                          */
	case 14:
		result = IR & 0xffff;	break;	/* Interrupt (upper) / Refresh (lower) register */
	default:
		result = -1;	break;
	}

	lua_pushinteger(L, result);
	return(1);
}

int luaWriteReg(lua_State* L) {
	uint8 reg = (uint8)luaL_checkinteger(L, 1);
	uint16 value = (uint8)luaL_checkinteger(L, 2);

	switch (reg) {
	case 0:
		PCX = value;	break;	/* external view of PC                          */
	case 1:
		AF = value;		break;	/* AF register                                  */
	case 2:
		BC = value;		break;	/* BC register                                  */
	case 3:
		DE = value;		break;	/* DE register                                  */
	case 4:
		HL = value;		break;	/* HL register                                  */
	case 5:
		IX = value;		break;	/* IX register                                  */
	case 6:
		IY = value;		break;	/* IY register                                  */
	case 7:
		PC = value;		break;	/* program counter                              */
	case 8:
		SP = value;		break;	/* SP register                                  */
	case 9:
		AF1 = value;	break;	/* alternate AF register                        */
	case 10:
		BC1 = value;	break;	/* alternate BC register                        */
	case 11:
		DE1 = value;	break;	/* alternate DE register                        */
	case 12:
		HL1 = value;	break;	/* alternate HL register                        */
	case 13:
		IFF = value;	break;	/* Interrupt Flip Flop                          */
	case 14:
		IR = value;		break;	/* Interrupt (upper) / Refresh (lower) register */
	default:
		break;
	}

	return(0);
}

#endif
