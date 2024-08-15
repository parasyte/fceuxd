/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 1998 BERO
 *  Copyright (C) 2002 Xodnizel
 *  Mapper 12 code Copyright (C) 2003 CaH4e3
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

/*  Code for emulating iNES mappers 4, 118,119 */

#include "mapinc.h"

#define resetmode mapbyte1[0]
#define MMC3_cmd mapbyte1[1]
#define A000B mapbyte1[2]
#define A001B mapbyte1[3]
#define DRegBuf mapbyte4

#define PPUCHRBus mapbyte2[0]
#define TKSMIR mapbyte3
#define EXPREGS	mapbyte2

static void (*pwrap)(uint32 A, uint8 V);
static void (*cwrap)(uint32 A, uint8 V);
static void (*mwrap)(uint8 V);

static int mmc3opts=0;

static INLINE void FixMMC3PRG(int V);
static INLINE void FixMMC3CHR(int V);

static int latched;
static uint32 lasta=0x0000;
static uint32 counter=0; 

//static DECLFW(AltMMC3_IRQWrite)
//{
//	switch(A&0xE001)
//	{
//         case 0xc000:IRQCount=V;break;
//         case 0xc001:IRQLatch=V;break;
//         case 0xE000:X6502_IRQEnd(FCEU_IQEXT);IRQa=0;break;
//         case 0xE001:IRQa=1;break;
//	}
//} //bbit edited: commented this out to remove compiler warnings

static DECLFW(MMC3_IRQWrite)
{
 //printf("$%04x:$%02x, %d, %d\n",A,V,scanline,timestamp);
        switch(A&0xE001)
        {
         case 0xc000:IRQLatch=V;
	             latched=1;
		     if(resetmode)
		     {
		      IRQCount=V;
		      latched=0;
		      //resetmode=0;
		     }
                     break;
         case 0xc001:IRQCount=IRQLatch;
		     lasta=0;
			counter=0;
                     break;
         case 0xE000:IRQa=0;
		     X6502_IRQEnd(FCEU_IQEXT);
		     resetmode=1;
                     break;
         case 0xE001:IRQa=1;
		     if(latched) 
		      IRQCount=IRQLatch;
                     break;
        }

}

static INLINE void FixMMC3PRG(int V)
{
          if(V&0x40)
          {
           pwrap(0xC000,DRegBuf[6]);
           pwrap(0x8000,~1);
          }
          else
          {
           pwrap(0x8000,DRegBuf[6]);
           pwrap(0xC000,~1);
          }
	  pwrap(0xA000,DRegBuf[7]);
	  pwrap(0xE000,~0);
}

static INLINE void FixMMC3CHR(int V)
{
           int cbase=(V&0x80)<<5;
           cwrap((cbase^0x000),DRegBuf[0]&(~1));
           cwrap((cbase^0x400),DRegBuf[0]|1);
           cwrap((cbase^0x800),DRegBuf[1]&(~1));
           cwrap((cbase^0xC00),DRegBuf[1]|1);

           cwrap(cbase^0x1000,DRegBuf[2]);
           cwrap(cbase^0x1400,DRegBuf[3]);
           cwrap(cbase^0x1800,DRegBuf[4]);
           cwrap(cbase^0x1c00,DRegBuf[5]);
}

static void MMC3RegReset(void)
{
 IRQCount=IRQLatch=IRQa=MMC3_cmd=0;

 DRegBuf[0]=0;
 DRegBuf[1]=2;
 DRegBuf[2]=4;
 DRegBuf[3]=5;
 DRegBuf[4]=6;
 DRegBuf[5]=7;
 DRegBuf[6]=0;
 DRegBuf[7]=1;

 FixMMC3PRG(0);
 FixMMC3CHR(0);
}

static DECLFW(Mapper4_write)
{
        switch(A&0xE001)
	{
         case 0x8000:
          if((V&0x40) != (MMC3_cmd&0x40))
	   FixMMC3PRG(V);
          if((V&0x80) != (MMC3_cmd&0x80))
           FixMMC3CHR(V);
          MMC3_cmd = V;
          break;

        case 0x8001:
                {
                 int cbase=(MMC3_cmd&0x80)<<5;
                 DRegBuf[MMC3_cmd&0x7]=V;
                 switch(MMC3_cmd&0x07)
                 {
                  case 0: //EXPREGS[0]=V;
			  cwrap((cbase^0x000),V&(~1));
			  cwrap((cbase^0x400),V|1);
			  break;
                  case 1: cwrap((cbase^0x800),V&(~1));
			  cwrap((cbase^0xC00),V|1);
			  break;
                  case 2: cwrap(cbase^0x1000,V); break;
                  case 3: cwrap(cbase^0x1400,V); break;
                  case 4: cwrap(cbase^0x1800,V); break;
                  case 5: cwrap(cbase^0x1C00,V); break;
                  case 6: if (MMC3_cmd&0x40) pwrap(0xC000,V);
                          else pwrap(0x8000,V);
                          break;
                  case 7: pwrap(0xA000,V);
                          break;
                 }
                }
                break;

        case 0xA000:
	        if(mwrap) mwrap(V&1);
                break;
	case 0xA001:
		A001B=V;
		break;
 }
}

//static void AltMMC3_hb(void)
//{
// if(IRQa)
// {
//  if(IRQCount==0)
//  {
//   IRQCount=IRQLatch;
//   X6502_IRQBegin(FCEU_IQEXT);   
//  }
//  else IRQCount--;
// }
//} //bbit edited: commented this out to remove compiler warnings

static void MMC3_hb(void)
{
      resetmode=0;
	//printf("%d:%d:%d\n",IRQCount,scanline,timestamp);
      if(IRQCount>=0)
      {
        IRQCount--;
        if(IRQCount<0)
        {
	 //printf("IRQ: %d, %d\n",scanline,timestamp);
         if(IRQa)
          X6502_IRQBegin(FCEU_IQEXT);
        }
      }
}
//static int todofoo=0; //bbit edited: commented this out to remove compiler warnings

//static void FP_FASTAPASS(1) IRQHook(int a)
//{
// if(todofoo)
// {
//  if(todofoo==1)
//   MMC3_hb();
//  todofoo--;
// }
//}        //bbit edited: commented this out to remove compiler warnings

//static void FP_FASTAPASS(1) POOPPU(uint32 A)
//{
// if((A&0x2000) && !(lasta&0x2000))
// {
//  counter++;
//  if(counter==42)
//  {
//   todofoo=1;
//   counter=0;
//  }
// }
// lasta=A;
//}  //bbit edited: commented this out to remove compiler warnings

static int isines;

static void genmmc3restore(int version)
{
 mwrap(A000B&1);
 FixMMC3PRG(MMC3_cmd);
 FixMMC3CHR(MMC3_cmd);
}

static void GENCWRAP(uint32 A, uint8 V)
{
 setchr1(A,V);
}

static void GENPWRAP(uint32 A, uint8 V)
{
 setprg8(A,V&0x3F);
}

static void GENMWRAP(uint8 V)
{
 A000B=V;
 setmirror(V^1);
}

static void GENNOMWRAP(uint8 V)
{
 A000B=V;
}

static void genmmc3ii(void (*PW)(uint32 A, uint8 V), 
		      void (*CW)(uint32 A, uint8 V), 
		      void (*MW)(uint8 V))
{
 pwrap=GENPWRAP;
 cwrap=GENCWRAP;
 mwrap=GENMWRAP;
 if(PW) pwrap=PW;
 if(CW) cwrap=CW;
 if(MW) mwrap=MW;
 A000B=(Mirroring&1)^1; // For hard-wired mirroring on some MMC3 games.
                        // iNES format needs to die or be extended...
 mmc3opts=0;
 SetWriteHandler(0x8000,0xBFFF,Mapper4_write);
 SetWriteHandler(0xC000,0xFFFF,MMC3_IRQWrite);
 GameHBIRQHook=MMC3_hb;
 GameStateRestore=genmmc3restore;
 if(!VROM_size)
  SetupCartCHRMapping(0, CHRRAM, 8192, 1);
 isines=1;
 MMC3RegReset();
 MapperReset=MMC3RegReset;
 //PPU_hook=POOPPU;
 //MapIRQHook=IRQHook;
}

void Mapper4_init(void)
{
 genmmc3ii(0,0,0);
}

static DECLFW(Mapper12IRQ)
{
	//printf("$%04x:$%02x\n",A,V);
        switch(A&0xE001)
        {
         case 0xc000:IRQLatch=V;break;
         case 0xc001:IRQCount=IRQLatch;break;
         case 0xE000:X6502_IRQEnd(FCEU_IQEXT);IRQa=0;break;
         case 0xE001:IRQa=1;break;
        }
}

static void m12irq(void)
{
//  if (scanline<=240)
   {
      if(IRQCount>=0)
      {
        IRQCount--;
        if(IRQCount<0)
        {
         if(IRQa)
          X6502_IRQBegin(FCEU_IQEXT);
        }
      }
   }
}

static void M12CW(uint32 A, uint8 V)
{
 setchr1(A,(EXPREGS[(A&0x1000)>>12]<<8)+V);
}

static DECLFW(M12Write)
{
 EXPREGS[0]= V&0x01;
 EXPREGS[1]= (V&0x10)>>4;
}
 
void Mapper12_init(void)
{
 genmmc3ii(0,M12CW,0);
 SetWriteHandler(0x4100,0x5FFF,M12Write);
 SetWriteHandler(0xC000,0xFFFF,Mapper12IRQ);
 GameHBIRQHook=m12irq;
}

static void M47PW(uint32 A, uint8 V)
{
 V&=0xF;
 V|=EXPREGS[0]<<4;
 setprg8(A,V);
}

static void M47CW(uint32 A, uint8 V)
{
 uint32 NV=V;
 NV&=0x7F;
 NV|=EXPREGS[0]<<7;
 setchr1(A,NV);
}

static DECLFW(M47Write)
{
 EXPREGS[0]=V&1;
 FixMMC3PRG(MMC3_cmd);
 FixMMC3CHR(MMC3_cmd); 
}

void Mapper47_init(void)
{
 genmmc3ii(M47PW,M47CW,0);
 SetWriteHandler(0x6000,0x7FFF,M47Write);
 SetReadHandler(0x6000,0x7FFF,0);
}

static void M44PW(uint32 A, uint8 V)
{
 uint32 NV=V;
 if(EXPREGS[0]>=6) NV&=0x1F;
 else NV&=0x0F;
 NV|=EXPREGS[0]<<4;
 setprg8(A,NV);
}
static void M44CW(uint32 A, uint8 V)
{
 uint32 NV=V;
 if(EXPREGS[0]<6) NV&=0x7F;
 NV|=EXPREGS[0]<<7;
 setchr1(A,NV);
}

static DECLFW(Mapper44_write)
{
 if(A&1)
 {
  EXPREGS[0]=V&7;
  FixMMC3PRG(MMC3_cmd);
  FixMMC3CHR(MMC3_cmd);
 }
 else
  Mapper4_write(A,V);
}

void Mapper44_init(void)
{
 genmmc3ii(M44PW,M44CW,0);
 SetWriteHandler(0xA000,0xBFFF,Mapper44_write);
}

static void M52PW(uint32 A, uint8 V)
{
 uint32 NV=V;
 NV&=0x1F^((EXPREGS[0]&8)<<1);
 NV|=((EXPREGS[0]&6)|((EXPREGS[0]>>3)&EXPREGS[0]&1))<<4;
 setprg8(A,NV);
}

static void M52CW(uint32 A, uint8 V)
{
 uint32 NV=V;
 NV&=0xFF^((EXPREGS[0]&0x40)<<1);
 NV|=(((EXPREGS[0]>>3)&4)|((EXPREGS[0]>>1)&2)|((EXPREGS[0]>>6)&(EXPREGS[0]>>4)&1))<<7;
 setchr1(A,NV);
}

static DECLFW(Mapper52_write)
{
 if(EXPREGS[1]) 
 {
  (WRAM-0x6000)[A]=V;
  return;
 }
 EXPREGS[1]=1;
 EXPREGS[0]=V;
 FixMMC3PRG(MMC3_cmd);
 FixMMC3CHR(MMC3_cmd);
}

static void M52Reset(void)
{
 EXPREGS[0]=EXPREGS[1]=0;
 MMC3RegReset(); 
}

void Mapper52_init(void)
{
 genmmc3ii(M52PW,M52CW,0);
 SetWriteHandler(0x6000,0x7FFF,Mapper52_write);
 MapperReset=M52Reset;
}

static void M45CW(uint32 A, uint8 V)
{
 uint32 NV=V;
 if(EXPREGS[2]&8)
  NV&=(1<<( (EXPREGS[2]&7)+1 ))-1;
 else
  NV&=0;
 NV|=EXPREGS[0]|((EXPREGS[2]&0xF0)<<4); // &0x10(not 0xf0) is valid given the original
					// description of mapper 45 by kevtris,
					// but this fixes Super 8 in 1.
 setchr1(A,NV);
}

static void M45PW(uint32 A, uint8 V)
{
 //V=((V&(EXPREGS[3]^0xFF))&0x3f)|EXPREGS[1];
 V&=(EXPREGS[3]&0x3F)^0x3F;
 V|=EXPREGS[1];
 //printf("$%04x, $%02x\n",A,V);
 setprg8(A,V);
}

static DECLFW(Mapper45_write)
{
 //printf("$%02x, %d\n",V,EXPREGS[4]);
 if(EXPREGS[3]&0x40) 
 {
  (WRAM-0x6000)[A]=V;   
  return;
 }
 EXPREGS[EXPREGS[4]]=V;
 EXPREGS[4]=(EXPREGS[4]+1)&3;
 FixMMC3PRG(MMC3_cmd);
 FixMMC3CHR(MMC3_cmd);
}

static void M45Reset(void)
{
 FCEU_dwmemset(EXPREGS,0,5);
 MMC3RegReset();
}

void Mapper45_init(void)
{
 genmmc3ii(M45PW,M45CW,0);
 SetWriteHandler(0x6000,0x7FFF,Mapper45_write);
 //SetReadHandler(0x6000,0x7FFF,0);
 MapperReset=M45Reset;
}

static void M49PW(uint32 A, uint8 V)
{
 if(EXPREGS[0]&1)
 {
  V&=0xF;
  V|=(EXPREGS[0]&0xC0)>>2;
  setprg8(A,V);
 }
 else
  setprg32(0x8000,(EXPREGS[0]>>4)&3);
}

static void M49CW(uint32 A, uint8 V)
{
 uint32 NV=V;
 NV&=0x7F;
 NV|=(EXPREGS[0]&0xC0)<<1;
 setchr1(A,NV);
}

static DECLFW(M49Write)
{
 //printf("$%04x:$%02x\n",A,V);
 if(A001B&0x80)
 {
  EXPREGS[0]=V;
  FixMMC3PRG(MMC3_cmd);
  FixMMC3CHR(MMC3_cmd);
 }
}

static void M49Reset(void)
{
 EXPREGS[0]=0;
 MMC3RegReset();
}

void Mapper49_init(void)
{
 genmmc3ii(M49PW,M49CW,0);
 SetWriteHandler(0x6000,0x7FFF,M49Write);
 SetReadHandler(0x6000,0x7FFF,0);
 MapperReset=M49Reset;
}

static void M115PW(uint32 A, uint8 V)
{
 setprg8(A,V);
 if(EXPREGS[0]&0x80)
 {
  ROM_BANK16(0x8000,EXPREGS[0]&7);
 }
}

static void M115CW(uint32 A, uint8 V)
{
 setchr1(A,(uint32)V|((EXPREGS[1]&1)<<8));
}

static DECLFW(M115Write)
{
 if(A==0x6000)
  EXPREGS[0]=V;
 else if(A==0x6001)
  EXPREGS[1]=V;
 FixMMC3PRG(MMC3_cmd);
}

void Mapper115_init(void)
{
 genmmc3ii(M115PW,M115CW,0);
 SetWriteHandler(0x6000,0x7FFF,M115Write);
 SetReadHandler(0x6000,0x7FFF,0);
// MapperReset=M49Reset;
}

static void M116PW(uint32 A, uint8 V)
{
 setprg8(A,(V&0xF)|((EXPREGS[1]&0x2)<<3));
 // setprg8(A,(V&7)|(<<3));
}

static void M116CW(uint32 A, uint8 V)
{
 //if(EXPREGS[1]&2)
  setchr1(A,V|((EXPREGS[0]&0x4)<<6));
 //else
 //{
 // setchr1(A,(V&7)|((EXPREGS[2])<<3));
 //}
}

static DECLFW(M116Write)
{
 if(A==0x4100) {EXPREGS[0]=V;setmirror(V&1);}
 else if(A==0x4141) EXPREGS[1]=V;
 else if(A==0xa000) EXPREGS[2]=V;
 else if(A==0x4106) EXPREGS[3]=V;
 FixMMC3PRG(MMC3_cmd);
 FixMMC3CHR(MMC3_cmd);
// MIRROR_SET(0);
 //printf("$%04x:$%02x, $%04x, %d\n",A,V,X.PC,timestamp);
}

void Mapper116_init(void)
{
 EXPREGS[1]=2;
 genmmc3ii(M116PW,M116CW,0);
 SetWriteHandler(0x4020,0x7fff,M116Write);
 SetWriteHandler(0xa000,0xa000,M116Write);
}

static void M187PW(uint32 A, uint8 V)
{
 setprg8(A,V);
 if(EXPREGS[0]&0x80)
 {
  if(EXPREGS[0]&0x20)
   ROM_BANK32((EXPREGS[0]&0x1F)>>1);
  else
   ROM_BANK16(0xc000,EXPREGS[0]&0x1F);
 }

}

static void M187CW(uint32 A, uint8 V)
{
 setchr1(A,V|0x100);
}

static DECLFW(M187Write)
{
 EXPREGS[1]=V;
 if(A==0x5000)
 {
  EXPREGS[0]=V;
  FixMMC3PRG(MMC3_cmd);
 }
 //printf("$%04x:$%02x, $%04x\n",A,V,X.PC);
 //FCEUI_DumpMem("out",0x8000,0xffff);
}

static DECLFR(M187Read)
{
 //printf("Rd: $%04x, $%04x\n",A,X.PC);
 switch(EXPREGS[1]&0x3)
 {
  case 0: case 1:return(0x83);
  case 2:return(0x42);
 }
 return(0);
}

static DECLFW(mooga)
{
 //printf("$%04x:$%02x, $%04x\n",A,V,X.PC);
 A=(A&0xF001)^((A&2)>>1);
 Mapper4_write(A,V);
 //if(A==0xa000) FCEUI_DumpMem("out",0x8000,0xffff);
}

void Mapper187_init(void)
{
 genmmc3ii(M187PW,M187CW,0);
 SetWriteHandler(0x8000,0xBFFF,mooga);
 SetWriteHandler(0x5000,0x7FFF,M187Write);
 SetReadHandler(0x5000,0x7FFF,M187Read);

}

static DECLFW(Mapper250_write)
{
	Mapper4_write((A&0xE000)|((A&0x400)>>10),A&0xFF);
}

static DECLFW(M250_IRQWrite)
{
	MMC3_IRQWrite((A&0xE000)|((A&0x400)>>10),A&0xFF);
}

void Mapper250_init(void)
{
 genmmc3ii(0,0,0);
 SetWriteHandler(0x8000,0xBFFF,Mapper250_write);
 SetWriteHandler(0xC000,0xFFFF,M250_IRQWrite);
}

static void M249PW(uint32 A, uint8 V)
{
 if(EXPREGS[0]&0x2)
 {
  if(V<0x20)
   V=(V&1)|((V>>3)&2)|((V>>1)&4)|((V<<2)&8)|((V<<2)&0x10);
  else
  {
   V-=0x20;
   V=(V&3)|((V>>1)&4)|((V>>4)&8)|((V>>2)&0x10)|((V<<3)&0x20)|((V<<2)&0xC0);
  }
 }
 setprg8(A,V);
}

static void M249CW(uint32 A, uint8 V)
{
 if(EXPREGS[0]&0x2)
  V=(V&3)|((V>>1)&4)|((V>>4)&8)|((V>>2)&0x10)|((V<<3)&0x20)|((V<<2)&0xC0);
 setchr1(A,V);
}

static DECLFW(Mapper249_write)
{
 EXPREGS[0]=V;
 FixMMC3PRG(MMC3_cmd);
 FixMMC3CHR(MMC3_cmd);
}

void Mapper249_init(void)
{
 genmmc3ii(M249PW,M249CW,0);
 SetWriteHandler(0x5000,0x5000,Mapper249_write);
}

static void M245CW(uint32 A, uint8 V)
{
 //printf("$%04x:$%02x\n",A,V);
 //setchr1(A,V);
 //	
 EXPREGS[0]=V;
 FixMMC3PRG(MMC3_cmd);
}

static void M245PW(uint32 A, uint8 V)
{
 setprg8(A,(V&0x3F)|((EXPREGS[0]&2)<<5));
 //printf("%d\n",(V&0x3F)|((EXPREGS[0]&2)<<5));
}

void Mapper245_init(void)
{
 EXPREGS[0]=0;
 genmmc3ii(M245PW,M245CW,0);
// SetWriteHandler(0x4018,0x5fff,Mapper245_write);
// SetWriteHandler(0x8002,0xffff,Mapper245_write);
}

void m74p(uint32 a, uint8 v)
{
 setprg8(a,v&0x3f);
}

static void m74kie(uint32 a, uint8 v)
{
 if(v==((PRGsize[0]>>16)&0x08) || v==1+((PRGsize[0]>>16)&0x08))
  setchr1r(0x10,a,v);
 else
  setchr1r(0x0,a,v);
}

void Mapper74_init(void)
{
 genmmc3ii(m74p,m74kie,0);
 SetupCartCHRMapping(0x10, CHRRAM, 2048, 1);
 AddExState(CHRRAM, 2048, 0, "CHRR");
}

static void m148kie(uint32 a, uint8 v)
{
 if(v==128 || v==129)
  setchr1r(0x10,a,v);
 else
  setchr1r(0x0,a,v);
}

void Mapper148_init(void)
{
 genmmc3ii(m74p,m148kie,0);
 SetupCartCHRMapping(0x10, CHRRAM, 2048, 1);
 AddExState(CHRRAM, 2048, 0, "CHRR");
}

static void FP_FASTAPASS(1) TKSPPU(uint32 A)
{
 //static uint8 z;
 //if(A>=0x2000 || type<0) return;
 //if(type<0) return;
 A&=0x1FFF;
 //if(scanline>=140 && scanline<=200) {setmirror(MI_1);return;}
 //if(scanline>=140 && scanline<=200)
 // if(scanline>=190 && scanline<=200) {setmirror(MI_1);return;}
 // setmirror(MI_1); 
 //printf("$%04x\n",A);

 A>>=10;
 PPUCHRBus=A;
 setmirror(MI_0+TKSMIR[A]);
}

static void TKSWRAP(uint32 A, uint8 V)
{
 TKSMIR[A>>10]=V>>7;
 setchr1(A,V&0x7F);
 if(PPUCHRBus==(A>>10))
  setmirror(MI_0+(V>>7));
}

void Mapper118_init(void)
{
  genmmc3ii(0,TKSWRAP,GENNOMWRAP);
  PPU_hook=TKSPPU;
}

static void TQWRAP(uint32 A, uint8 V)
{
 setchr1r((V&0x40)>>2,A,V&0x3F);
}

void Mapper119_init(void)
{
  genmmc3ii(0,TQWRAP,0);
  SetupCartCHRMapping(0x10, CHRRAM, 8192, 1);
}

static int wrams;

static DECLFW(MBWRAM)
{
  (WRAM-0x6000)[A]=V;
}

static DECLFR(MAWRAM)
{
 return((WRAM-0x6000)[A]);
}

static DECLFW(MBWRAMMMC6)
{
 WRAM[A&0x3ff]=V;
}

static DECLFR(MAWRAMMMC6)
{
 return(WRAM[A&0x3ff]);
}

static void GenMMC3Power(void)
{
 SetWriteHandler(0x8000,0xBFFF,Mapper4_write);
 SetReadHandler(0x8000,0xFFFF,CartBR);
 SetWriteHandler(0xC000,0xFFFF,MMC3_IRQWrite);

 if(mmc3opts&1)
 {
  if(wrams==1024)
  {
   FCEU_CheatAddRAM(1,0x7000,WRAM);
   SetReadHandler(0x7000,0x7FFF,MAWRAMMMC6);
   SetWriteHandler(0x7000,0x7FFF,MBWRAMMMC6);
  }
  else
  {
   FCEU_CheatAddRAM(wrams/1024,0x6000,WRAM);
   SetReadHandler(0x6000,0x6000+wrams-1,MAWRAM);
   SetWriteHandler(0x6000,0x6000+wrams-1,MBWRAM);
  }
  if(!(mmc3opts&2))
   FCEU_dwmemset(WRAM,0,wrams);
 }
 MMC3RegReset();
 FCEU_dwmemset(CHRRAM,0,8192);
}

void GenMMC3_Init(CartInfo *info, int prg, int chr, int wram, int battery)
{
 pwrap=GENPWRAP;
 cwrap=GENCWRAP;
 mwrap=GENMWRAP;

 wrams=wram*1024;

 PRGmask8[0]&=(prg>>13)-1;
 CHRmask1[0]&=(chr>>10)-1;
 CHRmask2[0]&=(chr>>11)-1;

 if(wram)
 {
  mmc3opts|=1;
  AddExState(WRAM, wram*1024, 0, "WRAM");
 }

 if(battery)
 {
  mmc3opts|=2;

  info->SaveGame[0]=WRAM;
  info->SaveGameLen[0]=wram*1024;
 }

 if(!chr)
 {
  CHRmask1[0]=7;
  CHRmask2[0]=3;
  SetupCartCHRMapping(0, CHRRAM, 8192, 1);
  AddExState(CHRRAM, 8192, 0, "CHRR");
 }
 AddExState(mapbyte1, 32, 0, "MPBY");
 AddExState(&IRQa, 1, 0, "IRQA");
 AddExState(&IRQCount, 4, 1, "IRQC");
 AddExState(&IRQLatch, 4, 1, "IQL1");

 info->Power=GenMMC3Power;
 info->Reset=MMC3RegReset;

 GameHBIRQHook=MMC3_hb;
 GameStateRestore=genmmc3restore;
 isines=0;
}

void TEROM_Init(CartInfo *info)
{
 GenMMC3_Init(info, 32, 32, 0, 0);
}

void TFROM_Init(CartInfo *info)
{
 GenMMC3_Init(info, 512, 64, 0, 0);
}

void TGROM_Init(CartInfo *info)
{
 GenMMC3_Init(info, 512, 0, 0, 0);
}

void TKROM_Init(CartInfo *info)
{
 GenMMC3_Init(info, 512, 256, 8, info->battery);
}

void TLROM_Init(CartInfo *info)
{
 GenMMC3_Init(info, 512, 256, 0, 0);
}

void TSROM_Init(CartInfo *info)
{
 GenMMC3_Init(info, 512, 256, 8, 0);
}

void TLSROM_Init(CartInfo *info)
{
 GenMMC3_Init(info, 512, 256, 8, 0);
 cwrap=TKSWRAP;
 mwrap=GENNOMWRAP;
}

void TKSROM_Init(CartInfo *info)
{
 GenMMC3_Init(info, 512, 256, 8, info->battery);
 cwrap=TKSWRAP;
 mwrap=GENNOMWRAP;
}

void TQROM_Init(CartInfo *info)
{
 GenMMC3_Init(info, 512, 64, 0, 0);
 cwrap=TQWRAP;
}

/* MMC6 board */
void HKROM_Init(CartInfo *info)
{
 GenMMC3_Init(info, 512, 512, 1, info->battery);
}
