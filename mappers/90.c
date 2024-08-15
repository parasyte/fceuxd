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

static uint8 mul[2];
static uint8 regie;
#define tkcom1 mapbyte1[1]
#define tkcom2 mapbyte1[2]
#define tkcom3 mapbyte1[3]

#define prgb	mapbyte2
#define chrlow  mapbyte3
#define chrhigh mapbyte4

static uint16 names[4];
static uint8 tekker;

static DECLFR(tekread)
{
// FCEU_printf("READ READ READ: $%04x, $%04x\n",A,X.PC);
 switch(A)
 {
  case 0x5000:return(tekker);
  case 0x5800:return(mul[0]*mul[1]);
  case 0x5801:return((mul[0]*mul[1])>>8);
  case 0x5803:return(regie);
  default:break;
 } 
 return(X.DB);
}

static void mira(void)
{
 if(tkcom1&0x20)
 {
  int x;
  for(x=0;x<4;x++)
  {
   if(!(names[x]&0xFF00)) 
    setntamem(NTARAM+(((names[x]>>1)&1)<<10), 1, x);
   else
    setntamem(CHRptr[0]+(((names[x])&CHRmask1[0])<<10), 0, x);
  }
    //setmirrorw(0,0,1,1);
 }
 else
 {
  switch(tkcom3&3){
                  case 0:MIRROR_SET(0);break;
                  case 1:MIRROR_SET(1);break;
                  case 2:onemir(0);break;
                  case 3:onemir(1);break;
               }
 }
}

static void tekprom(void)
{
 switch(tkcom1&3)
  {
   case 1:              // 16 KB
          ROM_BANK16(0x8000,prgb[0]);
          ROM_BANK16(0xC000,prgb[2]);
          break;

   case 2:              //2 = 8 KB ??
	  if(tkcom1&0x4)
	  {
           ROM_BANK8(0x8000,prgb[0]);
           ROM_BANK8(0xa000,prgb[1]);
           ROM_BANK8(0xc000,prgb[2]);
           ROM_BANK8(0xe000,prgb[3]);
	  }
	  else
	  {
	   if(tkcom1&0x80)
	    ROM_BANK8(0x6000,prgb[3]);
           ROM_BANK8(0x8000,prgb[0]);
           ROM_BANK8(0xa000,prgb[1]);
           ROM_BANK8(0xc000,prgb[2]);
           ROM_BANK8(0xe000,~0);
	  }
 	  break;
   case 3:
          ROM_BANK8(0x8000,prgb[0]);
          ROM_BANK8(0xa000,prgb[1]);
          ROM_BANK8(0xc000,prgb[2]);
          ROM_BANK8(0xe000,prgb[3]);
          break;
  }
}

static void tekvrom(void)
{
 int x;

 switch(tkcom1&0x18)
  {
   case 0x00:      // 8KB
           VROM_BANK8(chrlow[0]|(chrhigh[0]<<8));
	   break;
   case 0x08:      // 4KB
          for(x=0;x<8;x+=4)
           VROM_BANK4(x<<10,chrlow[x]|(chrhigh[x]<<8));
	  break;
   case 0x10:      // 2KB
	  for(x=0;x<8;x+=2)
           VROM_BANK2(x<<10,chrlow[x]|(chrhigh[x]<<8));
	  break;
   case 0x18:      // 1KB
	   for(x=0;x<8;x++)
	    VROM_BANK1(x<<10,(chrlow[x]|(chrhigh[x]<<8)));
	   break;
 }
}
static DECLFW(Mapper90_write)
{
 if(A==0x5800) mul[0]=V;
 else if(A==0x5801) mul[1]=V;
 else if(A==0x5803) regie=V;
 //else if(A<0x8000) 
 // FCEU_printf("$%04x:$%02x $%04x, %d, %d\n",A,V,X.PC,scanline,timestamp);

 A&=0xF007;
 if(A>=0x8000 && A<=0x8003)
 {
  prgb[A&3]=V;
  tekprom();
 }
 else if(A>=0x9000 && A<=0x9007)
 {
  chrlow[A&7]=V;
  tekvrom();
 }
 else if(A>=0xa000 && A<=0xa007)
 {
  chrhigh[A&7]=V;
  tekvrom();
 }
 else if(A>=0xb000 && A<=0xb007)
 {
  //printf("$%04x:$%02x\n",A,V);
  if(A&4)
  {
   names[A&3]&=0x00FF;
   names[A&3]|=V<<8;
  }
  else
  {
   names[A&3]&=0xFF00;
   names[A&3]|=V;
  }
  mira();
 }
 else switch(A)
 {
   case 0xc002:IRQa=0;X6502_IRQEnd(FCEU_IQEXT);break;
   case 0xc003:
   case 0xc004:if(!IRQa) {IRQa=1;IRQCount=IRQLatch;}
		break;

   case 0xc005:IRQCount=IRQLatch=V;
	       X6502_IRQEnd(FCEU_IQEXT);break;
   case 0xc006:break;
   case 0xd000:tkcom1=V;
	       mira();
	       break;
   case 0xd001:tkcom3=V;
	       mira();
	       break;
 }
}
extern int squak;
static void Mapper90_hb(void)
{
 //if(scanline==(0xFB-0xD0) || scanline==(133+squak) || scanline==175) TriggerIRQ();
 //return;
 if(IRQCount) IRQCount--;
 if(!IRQCount)
 {
  if(IRQa) X6502_IRQBegin(FCEU_IQEXT);
  IRQa=0;
 }
}
static void togglie()
{
 tekker^=0xFF;
}

static void m90rest(int version)
{
 if(version>=92)
  mira();
}

void Mapper90_init(void)
{
  tekker=0;
  MapperReset=togglie;
  SetWriteHandler(0x5000,0xffff,Mapper90_write);
  SetReadHandler(0x5000,0x5fff,tekread);
  GameHBIRQHook=Mapper90_hb;
  MapStateRestore=m90rest;
}

