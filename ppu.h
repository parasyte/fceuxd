void FCEUPPU_Init(void);
void FCEUPPU_Reset(void);
void FCEUPPU_Power(void);
int FCEUPPU_Loop(void);

void PPUHack(void);
void FCEUPPU_SetVideoSystem(int w);

extern void FP_FASTAPASS(1) (*PPU_hook)(uint32 A);


//extern uint8 SPRAM[0x100],NTARAM[0x800],PALRAM[0x20],SPRAM[0x100],SPRBUF[0x100];

/* For cart.c and banksw.h, mostly */
extern uint8 NTARAM[0x800],*vnapage[4];
extern uint8 PPUNTARAM;
extern uint8 PPUCHRRAM;

void FCEUPPU_SaveState(void);
void FCEUPPU_LoadState(int version);

extern int scanline;
