
/* Type attributes, you can OR them together. */
#define BPOINT_READ		1
#define BPOINT_WRITE		2
#define BPOINT_PC		4


void FCEUI_DumpMem(char *fname, uint32 start, uint32 end);
void FCEUI_LoadMem(char *fname, uint32 start);
void FCEUI_SetCPUCallback(void (*callb)(X6502 *X));
int FCEUI_DeleteBreakPoint(uint32 w);
int FCEUI_ListBreakPoints(int (*callb)(int type, unsigned int A1, unsigned int A2,
                void (*Handler)(X6502 *, int type, unsigned int A) ));
int FCEUI_GetBreakPoint(uint32 w, int *type, unsigned int *A1, unsigned int *A2,
                void (**Handler)(X6502 *, int type, unsigned int A));
int FCEUI_SetBreakPoint(uint32 w, int type, unsigned int A1, unsigned int A2,
                void (*Handler)(X6502 *, int type, unsigned int A));
int FCEUI_AddBreakPoint(int type, unsigned int A1, unsigned int A2, 
                void (*Handler)(X6502 *, int type, unsigned int A));
