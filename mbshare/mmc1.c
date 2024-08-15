/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 1998 BERO
 *  Copyright (C) 2002 Xodnizel
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "mapinc.h"

#define MMC1_reg mapbyte1
#define MMC1_buf mapbyte2[0]
#define MMC1_sft mapbyte3[0]
#define EXPREGS	mapbyte4

static int mmc1opts;

static void (*MMC1CHRHook4)(uint32 A, uint8 V);
static void (*MMC1PRGHook16)(uint32 A, uint8 V);

uint8 MMC1WRAMsize;	/* For use in iNES.c */

static DECLFW(MBWRAM)
{
 if(!(MMC1_reg[3]&0x10))
  Page[A>>11][A]=V;     // WRAM is enabled.
}

static DECLFR(MAWRAM)
{
 if(MMC1_reg[3]&0x10)   
  return X.DB;          // WRAM is disabled
 return(Page[A>>11][A]);
}

static void MMC1CHR(void)
{
    if(mmc1opts&4)
    {
     if(MMC1_reg[0]&0x10)
      setprg8r(0x10,0x6000,(MMC1_reg[1]>>4)&1);
     else
      setprg8r(0x10,0x6000,(MMC1_reg[1]>>3)&1);
    }

    if(MMC1CHRHook4)
    {
     if(MMC1_reg[0]&0x10)
     {
      MMC1CHRHook4(0x0000,MMC1_reg[1]);
      MMC1CHRHook4(0x1000,MMC1_reg[2]);
     }   
     else
     {
      MMC1CHRHook4(0x0000,(MMC1_reg[1]&0xFE));
      MMC1CHRHook4(0x1000,MMC1_reg[1]|1);
     }
    }
    else
    {
     if(MMC1_reg[0]&0x10)
     {
      setchr4(0x0000,MMC1_reg[1]);
      setchr4(0x1000,MMC1_reg[2]);
     }
     else
      setchr8(MMC1_reg[1]>>1);
    }
}

static void MMC1PRG(void)
{
        uint8 offs;

        offs=MMC1_reg[1]&0x10;
	if(MMC1PRGHook16)
	{
         switch(MMC1_reg[0]&0xC)
         {
          case 0xC: MMC1PRGHook16(0x8000,(MMC1_reg[3]+offs));
                    MMC1PRGHook16(0xC000,0xF+offs);
                    break;
          case 0x8: MMC1PRGHook16(0xC000,(MMC1_reg[3]+offs));
                    MMC1PRGHook16(0x8000,offs);
                    break;
          case 0x0:
          case 0x4:
                    MMC1PRGHook16(0x8000,((MMC1_reg[3]&~1)+offs));
                    MMC1PRGHook16(0xc000,((MMC1_reg[3]&~1)+offs+1));
                    break;
         }
	}
	else
         switch(MMC1_reg[0]&0xC)
         {
          case 0xC: setprg16(0x8000,(MMC1_reg[3]+offs));
                    setprg16(0xC000,0xF+offs);
                    break;
          case 0x8: setprg16(0xC000,(MMC1_reg[3]+offs));
                    setprg16(0x8000,offs);
                    break;
          case 0x0:
          case 0x4:
                    setprg16(0x8000,((MMC1_reg[3]&~1)+offs));
                    setprg16(0xc000,((MMC1_reg[3]&~1)+offs+1));
                    break;
         }
}
static void MMC1MIRROR(void)
{
                switch(MMC1_reg[0]&3)
                {
                 case 2: setmirror(MI_V);break;
                 case 3: setmirror(MI_H);break;
                 case 0: setmirror(MI_0);break;
                 case 1: setmirror(MI_1);break;
                }
}

static uint64 lreset;
static DECLFW(MMC1_write)
{
        int n=(A>>13)-4;
	//FCEU_DispMessage("%016x",timestampbase+timestamp);
	//printf("$%04x:$%02x, $%04x\n",A,V,X.PC);
	//DumpMem("out",0xe000,0xffff);

	/* The MMC1 is busy so ignore the write. */
	/* As of version FCE Ultra 0.81, the timestamp is only
	   increased before each instruction is executed(in other words
	   precision isn't that great), but this should still work to
	   deal with 2 writes in a row from a single RMW instruction.
	*/
	if( (timestampbase+timestamp)<(lreset+2))
	 return;
        if (V&0x80)
        {
	 MMC1_reg[0]|=0xC;
         MMC1_sft=MMC1_buf=0;
	 MMC1PRG();
	 lreset=timestampbase+timestamp;
         return;
        }

        MMC1_buf|=(V&1)<<(MMC1_sft++);

  if (MMC1_sft==5) {
        MMC1_reg[n]=MMC1_buf;
        MMC1_sft = MMC1_buf=0;

        switch(n){
        case 0:
		MMC1MIRROR();
                MMC1CHR();
                MMC1PRG();
                break;
        case 1:
                MMC1CHR();
                MMC1PRG();
                break;
        case 2:
                MMC1CHR();
                break;
        case 3:
                MMC1PRG();
                break;
        }
  }
}

static void MMC1_Restore(int version)
{
 MMC1MIRROR();
 MMC1CHR();
 MMC1PRG();
 lreset=0;	/* timestamp(base) is not stored in save states. */
}

static void MMC1CMReset(void)
{
        int i;

        for(i=0;i<4;i++)
         MMC1_reg[i]=0;
        MMC1_sft = MMC1_buf =0;
        MMC1_reg[0]=0x1F;

        MMC1_reg[1]=0;
        MMC1_reg[2]=0;                  // Should this be something other than 0?
        MMC1_reg[3]=0;

        MMC1MIRROR();
        MMC1CHR();
        MMC1PRG();
}

void DetectMMC1WRAMSize(void)
{
        switch(iNESGameCRC32)
        {
         default:MMC1WRAMsize=1;break;
	 case 0xc6182024:	/* Romance of the 3 Kingdoms */
         case 0x2225c20f:       /* Genghis Khan */
         case 0x4642dda6:       /* Nobunaga's Ambition */
	 case 0x29449ba9:	/* ""	"" (J) */
	 case 0x2b11e0b0:	/* ""	"" (J) */
                MMC1WRAMsize=2;
                break;
        }
}

void GenMap1_init(void (*ph)(uint32 A, uint8 V),void (*ch)(uint32 A, uint8 V))
{
	MMC1PRGHook16=ph;
        MMC1CHRHook4=ch;
	lreset=0;
	mmc1opts=0;
        MMC1CMReset();        
	SetWriteHandler(0x8000,0xFFFF,MMC1_write);
	MapStateRestore=MMC1_Restore;
	AddExState(&lreset, 8, 1, "LRST");

	if(!VROM_size)
	 SetupCartCHRMapping(0, CHRRAM, 8192, 1);

	if(MMC1WRAMsize==2)
 	 mmc1opts|=4;

	SetupCartPRGMapping(0x10,WRAM,MMC1WRAMsize*8192,1);
	SetReadHandler(0x6000,0x7FFF,MAWRAM);
	SetWriteHandler(0x6000,0x7FFF,MBWRAM);
	setprg8r(0x10,0x6000,0);
}

void Mapper1_init(void)
{ 
 GenMap1_init(0,0);
}

static DECLFW(BWRAM)
{
 Page[A>>11][A]=V;
}

static DECLFR(AWRAM)
{
 return(Page[A>>11][A]);
}

/* Same as mapper 1, without respect for WRAM enable bit. */
void Mapper155_init(void)
{
 Mapper1_init();
 SetReadHandler(0x6000,0x7FFF,AWRAM);
 SetWriteHandler(0x6000,0x7FFF,BWRAM);
}

#define NWCDIP 0xE
 
static void FP_FASTAPASS(1) NWCIRQHook(int a)
{ 
        if(!(MMC1_reg[1]&0x10))
        {
         IRQCount+=a;
         if((IRQCount|(NWCDIP<<25))>=0x3e000000)
          {
           IRQCount=0;
           X6502_IRQBegin(FCEU_IQEXT);
          }
        }
} 

static void NWCCHRHook(uint32 A, uint8 V)
{
 if((V&0x10)) // && !(EXPREGS[0]&0x10))
 {
  IRQCount=0;
  X6502_IRQEnd(FCEU_IQEXT);
 }

 EXPREGS[0]=V;
 if(V&0x08)
  MMC1PRG();
 else
  setprg32(0x8000,(V>>1)&3);
}

static void NWCPRGHook(uint32 A, uint8 V)
{
 if(EXPREGS[0]&0x8)
  setprg16(A,8|(V&0x7));
 else
  setprg32(0x8000,(EXPREGS[0]>>1)&3);
}

void Mapper105_init(void)
{
 //Mapper1_init();
 MMC1CHRHook4=NWCCHRHook;
 MMC1PRGHook16=NWCPRGHook;
 GenMap1_init(NWCPRGHook,NWCCHRHook);
 // MMC1PRG();
 // MMC1CHR();
 setchr8r(0,0);
 MapIRQHook=NWCIRQHook;
}

static void GenMMC1Power(void)
{
 lreset=0;
 if(mmc1opts&1)
 {
  FCEU_CheatAddRAM(8,0x6000,WRAM);
  if(mmc1opts&4)
   FCEU_dwmemset(WRAM,0,8192)
  else if(!(mmc1opts&2))
   FCEU_dwmemset(WRAM,0,8192);
 }
 SetWriteHandler(0x8000,0xFFFF,MMC1_write);
 SetReadHandler(0x8000,0xFFFF,CartBR);

 if(mmc1opts&1)
 {
  SetReadHandler(0x6000,0x7FFF,MAWRAM);
  SetWriteHandler(0x6000,0x7FFF,MBWRAM);
  setprg8r(0x10,0x6000,0);
 }

 MMC1CMReset();
}

static void GenMMC1Init(CartInfo *info, int prg, int chr, int wram, int battery)
{
 MMC1PRGHook16=MMC1CHRHook4=0;
 mmc1opts=0;
 PRGmask16[0]&=(prg>>14)-1;
 CHRmask4[0]&=(chr>>12)-1;
 CHRmask8[0]&=(chr>>13)-1;

 if(wram) 
 { 
  mmc1opts|=1;
  if(wram>8) mmc1opts|=4;
  SetupCartPRGMapping(0x10,WRAM,wram*1024,1);
  AddExState(WRAM, wram*1024, 0, "WRAM");
 }

 if(battery)
 {
  mmc1opts|=2;

  info->SaveGame[0]=WRAM+((mmc1opts&4)?8192:0);
  info->SaveGameLen[0]=8192;
 }

 if(!chr)
 {
  CHRmask4[0]=1;
  SetupCartCHRMapping(0, CHRRAM, 8192, 1);
  AddExState(CHRRAM, 8192, 0, "CHRR");
 }
 AddExState(mapbyte1, 32, 0, "MPBY");
 info->Power=GenMMC1Power;

 GameStateRestore=MMC1_Restore;
 AddExState(&lreset, 8, 1, "LRST");
}

//static void GenMMC1Init(int prg, int chr, int wram, int battery)
void SAROM_Init(CartInfo *info)
{
 GenMMC1Init(info, 128, 64, 8, info->battery); 
}

void SBROM_Init(CartInfo *info)
{
 GenMMC1Init(info, 128, 64, 0, 0);
}

void SCROM_Init(CartInfo *info)	
{
 GenMMC1Init(info, 128, 128, 0, 0);
}

void SEROM_Init(CartInfo *info)
{
 GenMMC1Init(info, 32, 64, 0, 0);
}

void SGROM_Init(CartInfo *info)
{
 GenMMC1Init(info, 256, 0, 0, 0);
}

void SKROM_Init(CartInfo *info)
{
 GenMMC1Init(info, 256, 64, 8, info->battery);
}

void SLROM_Init(CartInfo *info)
{
 GenMMC1Init(info, 256, 128, 0, 0);
}

void SL1ROM_Init(CartInfo *info)
{
 GenMMC1Init(info, 128, 128, 0, 0);
}

/* Begin unknown - may be wrong - perhaps they use different MMC1s from the
   similarly functioning boards?
*/

void SL2ROM_Init(CartInfo *info)
{
 GenMMC1Init(info, 256, 256, 0, 0);
}

void SFROM_Init(CartInfo *info)
{
 GenMMC1Init(info, 256, 256, 0, 0);
}

void SHROM_Init(CartInfo *info)
{
 GenMMC1Init(info, 256, 256, 0, 0);
}

/* End unknown  */
/*              */
/*              */

void SNROM_Init(CartInfo *info)
{
 GenMMC1Init(info, 256, 0, 8, info->battery);
}

void SOROM_Init(CartInfo *info)
{
 GenMMC1Init(info, 256, 0, 16, info->battery);
}


