/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
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
//undef printf

static int32 IRQCount,IRQLatch;
static uint8 IRQa,resetmode,mbia;
static uint8 sizer,bigbank,bigbank2;

static uint8 DRegBuf[8],MMC3_cmd;

static int masko8[8]={63,31,15,1,3,0,0,0};
//static int masko1[8]={511,255,127,7,7,0,0,0};

static void swsetprg8(uint32 A, uint32 V)
{
 V&=masko8[sizer&7];
 V|=(bigbank*2);
 setprg8r((V/64)&15,A,V);
}

static void swsetchr1(uint32 A, uint32 V)
{
 if(sizer&0x20)
  setchr1r(0x10,A,V);
 else
 {
//  V&=masko1[sizer&7];
  V|=bigbank2*8;
  setchr1r((V/512)&15,A,V);
 }
}

static void swsetchr2(uint32 A, uint32 V)
{
 if(sizer&0x20)
  setchr2r(0x10,A,V);
 else
 {
  //V&=masko1[sizer&7]>>1;
  V|=bigbank2*4;  
  setchr2r((V/256)&15,A,V);
 }
}

static void Sup24_hb(void)
{
      resetmode=0;
      if(IRQCount>=0)
      {
        IRQCount--;
        if(IRQCount<0)
        {
         if(IRQa)
         {
            resetmode = 1;
	    X6502_IRQBegin(FCEU_IQEXT);
         }
        }
      }
}

static DECLFW(Sup24IRQWrite)
{
        switch(A&0xE001)
        {
         case 0xc000:IRQLatch=V;
                     if(resetmode==1)
                      IRQCount=IRQLatch;
                     break;
         case 0xc001:resetmode=1;
                     IRQCount=IRQLatch;
                     break;
         case 0xE000:IRQa=0;X6502_IRQEnd(FCEU_IQEXT);
                     if(resetmode==1)
                      {IRQCount=IRQLatch;}
                     break;
         case 0xE001:IRQa=1;
                     if(resetmode==1)
                       {IRQCount=IRQLatch;}
                     break;
        }
}

static void FixMMC3PRG(int V)
{
	  swsetprg8(0xA000,DRegBuf[7]);
	  swsetprg8(0xE000,~0);
          if(V&0x40)
           {
            swsetprg8(0xC000,DRegBuf[6]);
            swsetprg8(0x8000,~1);
           }
          else
           {
            swsetprg8(0x8000,DRegBuf[6]);
            swsetprg8(0xC000,~1);
           }
}

static void FixMMC3CHR(int V)
{
           int cbase=(V&0x80)<<5;
           swsetchr2((cbase^0x000),DRegBuf[0]>>1);
           swsetchr2((cbase^0x800),DRegBuf[1]>>1);
           swsetchr1(cbase^0x1000,DRegBuf[2]);
           swsetchr1(cbase^0x1400,DRegBuf[3]);
           swsetchr1(cbase^0x1800,DRegBuf[4]);
           swsetchr1(cbase^0x1c00,DRegBuf[5]);
}

static DECLFW(Super24hiwrite)
{
	//printf("$%04x:$%02x, %d\n",A,V,scanline);
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
                  case 0: V>>=1;swsetchr2((cbase^0x000),V);break;
                  case 1: V>>=1;swsetchr2((cbase^0x800),V);break;
                  case 2: swsetchr1(cbase^0x1000,V); break;
                  case 3: swsetchr1(cbase^0x1400,V); break;
                  case 4: swsetchr1(cbase^0x1800,V); break;
                  case 5: swsetchr1(cbase^0x1C00,V); break;
                  case 6: if (MMC3_cmd&0x40) swsetprg8(0xC000,V);
                          else swsetprg8(0x8000,V);
                          break;
                  case 7: swsetprg8(0xA000,V);
                          break;
                 }
                }
                break;

        case 0xA000:
		mbia=V;
                setmirror((V&1)^1);
                break;
 }
}


DECLFW(Super24Write)
{
 //printf("$%04x:$%02x\n",A,V);
 switch(A)
 {
  case 0x5ff0:sizer=V;
	      FixMMC3PRG(MMC3_cmd);
	      FixMMC3CHR(MMC3_cmd);
	      break;
  case 0x5FF1:
	      bigbank=V;
	      FixMMC3PRG(MMC3_cmd);
	      break;
  case 0x5FF2:
	      bigbank2=V;
	      FixMMC3CHR(MMC3_cmd);
	      break;
 }
}

static void Super24Reset(void)
{
 SetWriteHandler(0x8000,0xBFFF,Super24hiwrite);
 SetWriteHandler(0x5000,0x7FFF,Super24Write);
 SetWriteHandler(0xC000,0xFFFF,Sup24IRQWrite);
 SetReadHandler(0x8000,0xFFFF,CartBR);
 GameHBIRQHook=Sup24_hb;
 IRQCount=IRQLatch=IRQa=resetmode=0;
 sizer=0x24;
 bigbank=159;
 bigbank2=0;

 MMC3_cmd=0;
 DRegBuf[6]=0;
 DRegBuf[7]=1;

 FixMMC3PRG(0);
 FixMMC3CHR(0);
}

static void MrRestore(int version)
{
 FixMMC3PRG(MMC3_cmd);
 FixMMC3CHR(MMC3_cmd);
 setmirror((mbia&1)^1);
}

void Super24_Init(CartInfo *info)
{
 info->Power=Super24Reset;
 SetupCartCHRMapping(0x10, GameMemBlock, 8192, 1);
 GameStateRestore=MrRestore;

 AddExState(GameMemBlock, 8192, 0, "CHRR");
 AddExState(DRegBuf, 8, 0, "DREG");
 AddExState(&IRQCount, 4, 1, "IRQC");
 AddExState(&IRQLatch, 4, 1, "IQL1");
 AddExState(&IRQa, 1, 0, "IRQA");
 AddExState(&sizer, 1, 0, "SIZA");
 AddExState(&bigbank, 1, 0, "BIG1");
 AddExState(&bigbank2, 1, 0, "BIG2");
}
