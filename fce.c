/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2003 Xodnizel
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

#include        <string.h>
#include	<stdio.h>
#include	<stdlib.h>

#include	"types.h"
#include	"x6502.h"
#include	"fce.h"
#include	"ppu.h"
#include	"sound.h"
#include        "svga.h"
#include	"netplay.h"
#include	"general.h"
#include	"endian.h"
#include	"version.h"
#include        "memory.h"

#include	"cart.h"
#include	"nsf.h"
#include	"fds.h"
#include	"ines.h"
#include	"unif.h"
#include        "cheat.h"
#include	"palette.h"
#include	"state.h"
#include        "video.h"
#include	"input.h"
#include	"file.h"
#include	"crc32.h"

uint64 timestampbase;
FCEUGI FCEUGameInfo;
void (*GameInterface)(int h);

void (*GameStateRestore)(int version);
void (*GameHBIRQHook)(void);

readfunc ARead[0x10000];
writefunc BWrite[0x10000];
static readfunc *AReadG;
static writefunc *BWriteG;
static int RWWrap=0;

DECLFW(BNull)
{

}

DECLFR(ANull)
{
 return(X.DB);
}

int AllocGenieRW(void)
{
 if(!(AReadG=FCEU_malloc(0x8000*sizeof(readfunc))))
  return 0;
 if(!(BWriteG=FCEU_malloc(0x8000*sizeof(writefunc))))
  return 0;
 RWWrap=1;
 return 1;
}

void FlushGenieRW(void)
{
 int32 x;

 if(RWWrap)
 {
  for(x=0;x<0x8000;x++)
  {
   ARead[x+0x8000]=AReadG[x];
   BWrite[x+0x8000]=BWriteG[x];
  }
  free(AReadG);
  free(BWriteG);
  AReadG=0;
  BWriteG=0;
  RWWrap=0;
 }
}

readfunc FASTAPASS(1) GetReadHandler(int32 a)
{
  if(a>=0x8000 && RWWrap)
   return AReadG[a-0x8000];
  else
   return ARead[a];
}

void FASTAPASS(3) SetReadHandler(int32 start, int32 end, readfunc func)
{
  int32 x;

  if(!func)
   func=ANull;

  if(RWWrap)
   for(x=end;x>=start;x--)
   {
    if(x>=0x8000)
     AReadG[x-0x8000]=func;
    else
     ARead[x]=func;
   }
  else

   for(x=end;x>=start;x--)
    ARead[x]=func;
}

writefunc FASTAPASS(1) GetWriteHandler(int32 a)
{
  if(RWWrap && a>=0x8000)
   return BWriteG[a-0x8000];
  else
   return BWrite[a];
}

void FASTAPASS(3) SetWriteHandler(int32 start, int32 end, writefunc func)
{
  int32 x;

  if(!func)
   func=BNull;

  if(RWWrap)
   for(x=end;x>=start;x--)
   {
    if(x>=0x8000)
     BWriteG[x-0x8000]=func;
    else
     BWrite[x]=func;
   }
  else
   for(x=end;x>=start;x--)
    BWrite[x]=func;
}

uint8 GameMemBlock[131072];
uint8 RAM[0x800];
uint8 PAL=0;

static DECLFW(BRAML)
{  
        RAM[A]=V;
}

static DECLFW(BRAMH)
{
        RAM[A&0x7FF]=V;
}

static DECLFR(ARAML)
{
        return RAM[A];
}

static DECLFR(ARAMH)
{
        return RAM[A&0x7FF];
}

static int GameLoaded=0;
static void CloseGame(void)
{
 if(GameLoaded)
 {
  if(FCEUGameInfo.type!=GIT_NSF)
   FCEU_FlushGameCheats();
  #ifdef NETWORK
  if(FSettings.NetworkPlay) KillNetplay();
  #endif       
  GameInterface(GI_CLOSE);
  ResetExState();
  CloseGenie();
  GameLoaded=0;
 }
}

void ResetGameLoaded(void)
{
        if(GameLoaded) CloseGame();
        GameStateRestore=0;
        PPU_hook=0;
        GameHBIRQHook=0;
        if(GameExpSound.Kill)
         GameExpSound.Kill();
        memset(&GameExpSound,0,sizeof(GameExpSound));
        MapIRQHook=0;
        MMC5Hack=0;
        PAL&=1;
	pale=0;

	FCEUGameInfo.name=0;
	FCEUGameInfo.type=GIT_CART;
	FCEUGameInfo.vidsys=GIV_USER;
	FCEUGameInfo.input[0]=FCEUGameInfo.input[1]=-1;
	FCEUGameInfo.inputfc=-1;
	FCEUGameInfo.cspecial=0;
}

int UNIFLoad(char *name, FCEUFILE *fp);
int iNESLoad(char *name, FCEUFILE *fp);
int FDSLoad(char *name, FCEUFILE *fp);
int NSFLoad(FCEUFILE *fp);

FCEUGI *FCEUI_LoadGame(char *name)
{
        FCEUFILE *fp;

        Exit=1;
        ResetGameLoaded();

	FCEU_printf("Loading %s...\n\n",name);
	fp=FCEU_fopen(name,"rb",0);
	if(!fp)
        {
 	 FCEU_PrintError("Error opening \"%s\"!",name);
	 return 0;
	}

        if(iNESLoad(name,fp))
         goto endlseq;
        if(NSFLoad(fp))
         goto endlseq;
        if(FDSLoad(name,fp))
         goto endlseq;
        if(UNIFLoad(name,fp))
         goto endlseq;

        FCEU_PrintError("An error occurred while loading the file.");
        FCEU_fclose(fp);
        return 0;

        endlseq:
        FCEU_fclose(fp);
        GameLoaded=1;        

        FCEU_ResetVidSys();
        if(FCEUGameInfo.type!=GIT_NSF)
         if(FSettings.GameGenie)
	  OpenGenie();

        PowerNES();
	#ifdef NETWORK
        if(FSettings.NetworkPlay) InitNetplay();
	#endif
        SaveStateRefresh();
        if(FCEUGameInfo.type!=GIT_NSF)
        {
         FCEU_LoadGamePalette();
         FCEU_LoadGameCheats();
        }
        
	FCEU_ResetPalette();
	FCEU_ResetMessages();	// Save state, status messages, etc.
        Exit=0;
        return(&FCEUGameInfo);
}


int FCEUI_Initialize(void)
{
        if(!FCEU_InitVirtualVideo())
         return 0;
	memset(&FSettings,0,sizeof(FSettings));
	FSettings.UsrFirstSLine[0]=8;
	FSettings.UsrFirstSLine[1]=0;
        FSettings.UsrLastSLine[0]=231;
	FSettings.UsrLastSLine[1]=239;
	FSettings.SoundVolume=100;
	FCEUPPU_Init();
	X6502_Init();
        return 1;
}

void FCEUI_Kill(void)
{
 FCEU_KillVirtualVideo();
 FCEU_KillGenie();
}

void EmLoop(void)
{
 while(!Exit)
 {
  int r,ssize;

  FCEU_UpdateInput();

   //if(FCEUGameInfo.type==GIT_NSF)
   // DoNSFFrame();
  // else
  // if(FCEUGameInfo.type==GIT_NSF)
  //  X6502_Run((256+85)*240);

  if(geniestage!=1) FCEU_ApplyPeriodicCheats();
  r=FCEUPPU_Loop();

  ssize=FlushEmulateSound();
  FCEUD_Update(r?XBuf+8:0,WaveFinal,ssize);
 }	// while(!Exit)

 CloseGame();
}

#ifdef FPS
#include <sys/time.h>
uint64 frcount;
#endif

void FCEUI_Emulate(void)
{
	#ifdef FPS
        uint64 starttime,end;
        struct timeval tv;
	frcount=0;
        gettimeofday(&tv,0);
        starttime=((uint64)tv.tv_sec*1000000)+tv.tv_usec;
	#endif
	EmLoop();

        #ifdef FPS
        // Probably won't work well on Windows port; for
	// debugging/speed testing.
	{
	 uint64 w;
	 int i,frac;
         gettimeofday(&tv,0);
         end=((uint64)tv.tv_sec*1000000)+tv.tv_usec;
	 w=frcount*10000000000LL/(end-starttime);
	 i=w/10000;
	 frac=w-i*10000;
         FCEU_printf("Average FPS: %d.%04d\n",i,frac);
	}
        #endif

}

void FCEUI_CloseGame(void)
{
        Exit=1;
}

void ResetNES(void)
{
        if(!GameLoaded) return;
        GameInterface(GI_RESETM2);
        FCEUSND_Reset();
        FCEUPPU_Reset();
        X6502_Reset();
}

void FCEU_MemoryRand(uint8 *ptr, uint32 size)
{
 int x=0;
 while(size)
 {
  *ptr=(x&4)?0xFF:0x00;
  x++;
  size--;
  ptr++;
 }
}

void hand(X6502 *X, int type, unsigned int A)
{

}

void PowerNES(void) 
{
        if(!GameLoaded) return;

	FCEU_CheatResetRAM();
	FCEU_CheatAddRAM(2,0,RAM);

        GeniePower();

	FCEU_MemoryRand(RAM,0x800);
	//memset(RAM,0xFF,0x800);

        SetReadHandler(0x0000,0xFFFF,ANull);
        SetWriteHandler(0x0000,0xFFFF,BNull);
        
        SetReadHandler(0,0x7FF,ARAML);
        SetWriteHandler(0,0x7FF,BRAML);
        
        SetReadHandler(0x800,0x1FFF,ARAMH);  /* Part of a little */
        SetWriteHandler(0x800,0x1FFF,BRAMH); /* hack for a small speed boost. */
 
        InitializeInput();
        FCEUSND_Power();
        FCEUPPU_Power();

	/* Have the external game hardware "powered" after the internal NES stuff.  
	   Needed for the NSF code and VS System code.
	*/
	GameInterface(GI_POWER);
	timestampbase=0;
	X6502_Power();
	FCEU_PowerCheats();
}

void FCEU_ResetVidSys(void)
{
 int w;
  
 if(FCEUGameInfo.vidsys==GIV_NTSC)
  w=0; 
 else if(FCEUGameInfo.vidsys==GIV_PAL)
  w=1;  
 else
  w=FSettings.PAL;
  
 PAL=w?1:0;
 FCEUPPU_SetVideoSystem(w);
 SetSoundVariables();
}
