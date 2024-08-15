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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "types.h"
#include "x6502.h"
#include "fce.h"
#include "svga.h"
#include "video.h"
#include "sound.h"
#include "ines.h"
#include "nsf.h"
#include "general.h"
#include "memory.h"
#include "file.h"
#include "fds.h"
#include "cart.h"
#include "input.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static uint8 SongReload;
static int CurrentSong;

static DECLFW(NSF_write);
static DECLFR(NSF_read);


static uint8 NSFROM[0x30+6]=
{
/* 0x00 - NMI */
0x8D,0xF4,0x3F,                         /* Stop play routine NMIs. */
0xA2,0xFF,0x9A,                         /* Initialize the stack pointer. */
0xAD,0xF0,0x3F,                         /* See if we need to init. */
0xF0,0x09,                              /* If 0, go to play routine playing. */

0xAD,0xF1,0x3F,                         /* Confirm and load A      */
0xAE,0xF3,0x3F,                         /* Load X with PAL/NTSC byte */

0x20,0x00,0x00,                         /* JSR to init routine     */

0xA9,0x00,
0xAA,
0xA8,
0x20,0x00,0x00,                         /* JSR to play routine  */
0x8D,0xF5,0x3F,				/* Start play routine NMIs. */
0x90,0xFE,                               /* Loopie time. */

/* 0x20 */
0x8D,0xF3,0x3F,				/* Init init NMIs */
0x18,
0x90,0xFE				/* Loopie time. */
};

static DECLFR(NSFROMRead)
{
 return (NSFROM-0x3800)[A];
}

static int doreset=0;
static int NSFNMIFlags;
static uint8 *NSFDATA=0;
static int NSFMaxBank;

static int NSFSize;
static uint8 BSon;
static uint16 PlayAddr;
static uint16 InitAddr;
static uint16 LoadAddr;

NSF_HEADER NSFHeader; //Edited: keyword static removed to allow debugger.c access to this variable

void NSFGI(int h)
{
 switch(h)
 {
 case GI_CLOSE:
  if(NSFDATA) {free(NSFDATA);NSFDATA=0;}
  break;
 case GI_RESETM2:
 case GI_POWER: NSF_init();break;
 }
}

// First 32KB is reserved for sound chip emulation in the iNES mapper code.

#define WRAM (GameMemBlock+32768)
#define FDSMEM (GameMemBlock+32768)

static INLINE void BANKSET(uint32 A, uint32 bank)
{
 bank&=NSFMaxBank;
 if(NSFHeader.SoundChip&4)
  memcpy(FDSMEM+(A-0x6000),NSFDATA+(bank<<12),4096);
 else 
  setprg4(A,bank);
}

int NSFLoad(FCEUFILE *fp)
{
  int x;

  FCEU_fseek(fp,0,SEEK_SET);
  FCEU_fread(&NSFHeader,1,0x80,fp);
  if (memcmp(NSFHeader.ID,"NESM\x1a",5))
                  return 0;
  NSFHeader.SongName[31]=NSFHeader.Artist[31]=NSFHeader.Copyright[31]=0;

  LoadAddr=NSFHeader.LoadAddressLow;
  LoadAddr|=NSFHeader.LoadAddressHigh<<8;

  InitAddr=NSFHeader.InitAddressLow;
  InitAddr|=NSFHeader.InitAddressHigh<<8;

  PlayAddr=NSFHeader.PlayAddressLow;
  PlayAddr|=NSFHeader.PlayAddressHigh<<8;

  NSFSize=FCEU_fgetsize(fp)-0x80;

  NSFMaxBank=((NSFSize+(LoadAddr&0xfff)+4095)/4096);
  NSFMaxBank=uppow2(NSFMaxBank);

  if(!(NSFDATA=(uint8 *)FCEU_malloc(NSFMaxBank*4096)))
   return 0;

  FCEU_fseek(fp,0x80,SEEK_SET);
  memset(NSFDATA,0x00,NSFMaxBank*4096);
  FCEU_fread(NSFDATA+(LoadAddr&0xfff),1,NSFSize,fp);
 
  NSFMaxBank--;

  BSon=0;
  for(x=0;x<8;x++)
   BSon|=NSFHeader.BankSwitch[x];

 FCEUGameInfo.type=GIT_NSF;
 FCEUGameInfo.input[0]=FCEUGameInfo.input[1]=SI_GAMEPAD;

 for(x=0;;x++)
 {
  if(NSFROM[x]==0x20)
  {
   NSFROM[x+1]=InitAddr&0xFF;
   NSFROM[x+2]=InitAddr>>8;
   NSFROM[x+8]=PlayAddr&0xFF;
   NSFROM[x+9]=PlayAddr>>8;
   break;
  }
 }

 if(NSFHeader.VideoSystem==0)
  FCEUGameInfo.vidsys=GIV_NTSC;
 else if(NSFHeader.VideoSystem==1)
  FCEUGameInfo.vidsys=GIV_PAL;

 GameInterface=NSFGI;

 puts("NSF Loaded.  File information:\n");
 FCEU_printf(" Name:       %s\n Artist:     %s\n Copyright:  %s\n\n",NSFHeader.SongName,NSFHeader.Artist,NSFHeader.Copyright);
 if(NSFHeader.SoundChip)
 {
  static char *tab[6]={"Konami VRCVI","Konami VRCVII","Nintendo FDS","Nintendo MMC5","Namco 106","Sunsoft FME-07"};
  for(x=0;x<6;x++)
   if(NSFHeader.SoundChip&(1<<x))
   {
    FCEU_printf(" Expansion hardware:  %s\n",tab[x]);
    break;
   }
 }
 if(BSon)
  puts(" Bank-switched.");
 FCEU_printf(" Load address:  $%04x\n Init address:  $%04x\n Play address:  $%04x\n",LoadAddr,InitAddr,PlayAddr);
 FCEU_printf(" %s\n",(NSFHeader.VideoSystem&1)?"PAL":"NTSC");
 FCEU_printf(" Starting song:  %d / %d\n\n",NSFHeader.StartingSong,NSFHeader.TotalSongs);
 return 1;
}

static DECLFW(BWRAM)
{
                (WRAM-0x6000)[A]=V;
}

static DECLFW(NSFFDSWrite)
{
	(FDSMEM-0x6000)[A]=V;
}

static DECLFR(NSFFDSRead)
{
	return (FDSMEM-0x6000)[A];
}

static DECLFR(AWRAM)
{
	return WRAM[A-0x6000];
}

static DECLFR(NSFVectorRead)
{
 if(((NSFNMIFlags&1) && SongReload) || (NSFNMIFlags&2) || doreset)
 {
  if(A==0xFFFA) return(0x00);
  else if(A==0xFFFB) return(0x38);
  else if(A==0xFFFC) return(0x20);
  else if(A==0xFFFD) {doreset=0;return(0x38);}
  return(X.DB);
 }
 else
 {
  if(NSFHeader.SoundChip&4)
   return(NSFFDSRead(A));
  else
   return(CartBR(A));
 }
}
void NSF_init(void)
{
  doreset=1;
  if(NSFHeader.SoundChip&4)
  {
   memset(FDSMEM,0x00,32768+8192);
   SetWriteHandler(0x6000,0xDFFF,NSFFDSWrite);
   SetReadHandler(0x6000,0xFFFF,NSFFDSRead);
  }
  else
  {
   memset(WRAM,0x00,8192);
   SetReadHandler(0x6000,0x7FFF,AWRAM);
   SetWriteHandler(0x6000,0x7FFF,BWRAM);
   ResetCartMapping();
   SetupCartPRGMapping(0,NSFDATA,((NSFMaxBank+1)*4096),0);
   SetReadHandler(0x8000,0xFFFF,CartBR);
  }

  if(BSon)
  {
   int32 x;
   for(x=0;x<8;x++)
   {
    if(NSFHeader.SoundChip&4 && x>=6)
     BANKSET(0x6000+(x-6)*4096,NSFHeader.BankSwitch[x]);
    BANKSET(0x8000+x*4096,NSFHeader.BankSwitch[x]);
   }
  }
  else
  {
   int32 x;
    for(x=(LoadAddr&0x7000);x<0x8000;x+=0x1000)
     BANKSET(0x8000+x,((x-(LoadAddr&0x7000))>>12));
  }

  SetReadHandler(0xFFFA,0xFFFD,NSFVectorRead);

  SetWriteHandler(0x2000,0x3fff,0);
  SetReadHandler(0x2000,0x37ff,0);
  SetReadHandler(0x3836,0x3FFF,0);
  SetReadHandler(0x3800,0x3835,NSFROMRead);

  SetWriteHandler(0x5ff6,0x5fff,NSF_write);

  SetWriteHandler(0x3ff0,0x3fff,NSF_write);
  SetReadHandler(0x3ff0,0x3fff,NSF_read);


  if(NSFHeader.SoundChip&1) { 
   NSFVRC6_Init();
  } else if (NSFHeader.SoundChip&2) {
   NSFVRC7_Init();
  } else if (NSFHeader.SoundChip&4) {
   FDSSoundReset();
  } else if (NSFHeader.SoundChip&8) {
   NSFMMC5_Init();
  } else if (NSFHeader.SoundChip&0x10) {
   NSFN106_Init();
  } else if (NSFHeader.SoundChip&0x20) {
   NSFAY_Init();
  }
  CurrentSong=NSFHeader.StartingSong;
  SongReload=0xFF;
  NSFNMIFlags=0;
}

static DECLFW(NSF_write)
{
 switch(A)
 {
  case 0x3FF3:NSFNMIFlags|=1;break;
  case 0x3FF4:NSFNMIFlags&=~2;break;
  case 0x3FF5:NSFNMIFlags|=2;break;

  case 0x5FF6:
  case 0x5FF7:if(!(NSFHeader.SoundChip&4)) return;
  case 0x5FF8:
  case 0x5FF9:
  case 0x5FFA:
  case 0x5FFB:
  case 0x5FFC:
  case 0x5FFD:
  case 0x5FFE:
  case 0x5FFF:if(!BSon) return;
              A&=0xF;
              BANKSET((A*4096),V);
  	      break;
 } 
}

static DECLFR(NSF_read)
{
 int x;

 switch(A)
 {
 case 0x3ff0:x=SongReload;
	     if(!fceuindbg)
	      SongReload=0;
	     return x;
 case 0x3ff1:
	    if(!fceuindbg)
	    {
             memset(RAM,0x00,0x800);
             memset(WRAM,0x00,8192);
             BWrite[0x4015](0x4015,0x0);
             for(x=0;x<0x14;x++)
              BWrite[0x4000+x](0x4000+x,0);
             BWrite[0x4015](0x4015,0xF);
	     BWrite[0x4017](0x4017,0x40);
	     if(NSFHeader.SoundChip&4) 
	     {
	      BWrite[0x4089](0x4089,0x80);
	      BWrite[0x408A](0x408A,0xE8);
	     }
             if(BSon)
             {
              for(x=0;x<8;x++)
	       BANKSET(0x8000+x*4096,NSFHeader.BankSwitch[x]);
             }
             return (CurrentSong-1);
 	     }
 case 0x3FF3:return PAL;
 }
 return 0;
}

uint8 FCEU_GetJoyJoy(void);

static uint8 saver[272*240];
static int special=0;

void DrawNSF(uint8 *XBuf)
{
 char snbuf[16];
 uint8 *boo=saver+8;
 int x,y;
 XBuf+=8;

 if(special)
 {
  memset(saver+120*272,0,272);
  for(y=0;y<120;y++)
  {
   boo[y*272+128]=0;
   for(x=0;x<128;x++)
   {
    int32 t;
    t=(boo[(y+1)*272+x]*2+boo[(y+1)*272+x+1])/3;
    if(t&128) t=127;
    boo[y*272+x]=t; 
   }
   for(x=129;x<256;x++)
   {
    int32 t;
    t=(boo[(y+1)*272+x]*2+boo[(y+1)*272+x-1])/3;
    if(t&128) t=127;
    boo[y*272+x]=t;
   }
  }
  for(y=239;y>=121;y--)
   for(x=0;x<256;x++)
   {
    int32 t;
    t=(boo[(y-1)*272+x]*2+boo[(y-1)*272+x+1]+boo[(y-1)*272+x-1])/4;
    t*=1+2*log(1+abs(128-x))/128;
    if(t>=128) t=127;
    boo[y*272+x]=t; //(boo[(y-1)*272+x]+boo[(y-1)*272+x+1]+boo[(y-1)*272+x-1])/3;
   } 
 }
 else
  memset(XBuf-8,0,272*240);

 {
  int32 *Bufpl;
  int l;
  l=GetSoundBuffer(&Bufpl);
  if(special==0)
  {
   for(x=0;x<256;x++)
   {
    uint32 y;
    y=120+Bufpl[(x*l)>>8]*240/16384;
    if(y<240)
     XBuf[x+y*272]=131;
   }   
  }  
  else if(special==1)
   for(x=0;x<256;x++)
   {
    double r;
    uint32 xp,yp;

    r=Bufpl[(x*l)>>8]*240/8192;
    xp=128+r*cos(x*M_PI*2/256);
    yp=120+r*sin(x*M_PI*2/256);
    xp&=255;
    yp%=240;
    boo[xp+yp*272]=131;
   }
  else if(special==2)
  {
   static double theta=0;
   for(x=0;x<128;x++)
   {
    double xc,yc;
    double r,t;
    uint32 m,n;
    
    xc=(double)128-x;
    yc=0-((double)(Bufpl[(x*l)>>8])*240/16384);
    t=M_PI+atan(yc/xc);
    r=sqrt(xc*xc+yc*yc);
    
    t+=theta;
    m=128+r*cos(t);
    n=120+r*sin(t);
    
    if(m<256 && n<240)
     boo[m+n*272]=131; 

   }
   for(x=128;x<256;x++)
   {
    double xc,yc;
    double r,t;
    uint32 m,n;

    xc=(double)x-128;
    yc=(double)(Bufpl[(x*l)>>8]*240/16384);
    t=atan(yc/xc);
    r=sqrt(xc*xc+yc*yc);
    
    t+=theta;
    m=128+r*cos(t);
    n=120+r*sin(t);
    
    if(m<256 && n<240)
     boo[m+n*272]=131;

   }
   theta+=(double)M_PI/256;
  }
 }
 if(special)
  memcpy(XBuf-8,saver,272*240);
 DrawTextTrans(XBuf+10*272+4+(((31-strlen(NSFHeader.SongName))<<2)), 272, NSFHeader.SongName, 128+6);
 DrawTextTrans(XBuf+30*272+4+(((31-strlen(NSFHeader.Artist))<<2)), 272,NSFHeader.Artist, 128+6);
 DrawTextTrans(XBuf+50*272+4+(((31-strlen(NSFHeader.Copyright))<<2)), 272,NSFHeader.Copyright, 128+6);
 
 DrawTextTrans(XBuf+90*272+4+(((31-strlen("Song:"))<<2)), 272, "Song:", 128+6);
 sprintf(snbuf,"<%d/%d>",CurrentSong,NSFHeader.TotalSongs);
 DrawTextTrans(XBuf+102*272+4+(((31-strlen(snbuf))<<2)), 272, snbuf, 128+6);

 {
  static uint8 last=0;
  uint8 tmp;
  tmp=FCEU_GetJoyJoy();
  if((tmp&JOY_RIGHT) && !(last&JOY_RIGHT))
  {
   if(CurrentSong<NSFHeader.TotalSongs) 
   {
    CurrentSong++;   
    SongReload=0xFF;
   }
  }
  else if((tmp&JOY_LEFT) && !(last&JOY_LEFT))
  {
   if(CurrentSong>1)
   {
    CurrentSong--;
    SongReload=0xFF;
   }
  }
  else if((tmp&JOY_UP) && !(last&JOY_UP))
  {
   CurrentSong+=10;
   if(CurrentSong>NSFHeader.TotalSongs) CurrentSong=NSFHeader.TotalSongs;
   SongReload=0xFF;
  }
  else if((tmp&JOY_DOWN) && !(last&JOY_DOWN))
  {
   CurrentSong-=10;
   if(CurrentSong<1) CurrentSong=1;
   SongReload=0xFF;
  }
  else if((tmp&JOY_START) && !(last&JOY_START))
   SongReload=0xFF;
  else if((tmp&JOY_A) && !(last&JOY_A))
  {
   special=(special+1)%3;
  }
  last=tmp;
 }
}

void DoNSFFrame(void)
{
 if(((NSFNMIFlags&1) && SongReload) || (NSFNMIFlags&2))
  TriggerNMI();
}
