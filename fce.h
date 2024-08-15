#ifndef _FCEH
extern int GameLoaded;
extern int fceuindbg;
void ResetGameLoaded(void);

#define DECLFR(x) uint8 FP_FASTAPASS(1) x (uint32 A)
#define DECLFW(x) void FP_FASTAPASS(2) x (uint32 A, uint8 V)

void FCEU_MemoryRand(uint8 *ptr, uint32 size);
void FASTAPASS(3) SetReadHandler(int32 start, int32 end, readfunc func);
void FASTAPASS(3) SetWriteHandler(int32 start, int32 end, writefunc func);
writefunc FASTAPASS(1) GetWriteHandler(int32 a);
readfunc FASTAPASS(1) GetReadHandler(int32 a);

int AllocGenieRW(void);
void FlushGenieRW(void);

void FCEU_ResetVidSys(void);

void ResetMapping(void);
void ResetNES(void);
void PowerNES(void);


extern uint64 timestampbase;
extern uint32 MMC5HackVROMMask;
extern uint8 *MMC5HackExNTARAMPtr;
extern int MMC5Hack;
extern uint8 *MMC5HackVROMPTR;
extern uint8 MMC5HackCHRMode;
extern uint8 MMC5HackSPMode;
extern uint8 MMC5HackSPScroll;
extern uint8 MMC5HackSPPage;

extern uint8 RAM[0x800];
extern uint8 GameMemBlock[131072];

extern readfunc ARead[0x10000];
extern writefunc BWrite[0x10000];

extern void (*GameInterface)(int h);
extern void (*GameHBIRQHook)(void);
extern void (*GameStateRestore)(int version);

#define GI_RESETM2	1
#define GI_POWER	2
#define GI_CLOSE	3

#include "git.h"
extern FCEUGI FCEUGameInfo;
extern int GameAttributes;

extern uint8 PAL;
#endif

#define _FCEH

