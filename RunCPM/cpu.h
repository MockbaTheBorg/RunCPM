/* see main.c for definition */

int32 PCX;  /* external view of PC                          */
int32 AF;  /* AF register                                  */
int32 BC;  /* BC register                                  */
int32 DE;  /* DE register                                  */
int32 HL;  /* HL register                                  */
int32 IX;  /* IX register                                  */
int32 IY;  /* IY register                                  */
int32 PC;  /* program counter                              */
int32 SP;  /* SP register                                  */
int32 AF1; /* alternate AF register                        */
int32 BC1; /* alternate BC register                        */
int32 DE1; /* alternate DE register                        */
int32 HL1; /* alternate HL register                        */
int32 IFF; /* Interrupt Flip Flop                          */
int32 IR;  /* Interrupt (upper) / Refresh (lower) register */
int32 Status = 0; /* Status of the CPU 0=running 1=end request 2=back to CCP */
int32 Debug = 0;
int32 Break = -1;

/*
	Functions needed by the soft CPU implementation
*/
void cpu_out(const uint32 Port, const uint32 Value)
{
	_Bios();
}

uint32 cpu_in(const uint32 Port)
{
	_Bdos();
	return(HIGH_REGISTER(AF));
}
