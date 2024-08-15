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

/*  This file contains or coordinates all of the code necessary to compile
    on a UNIX system that can use svgalib, such as FreeBSD and Linux.  
    This code is not guaranteed to compile on FreeBSD, though.
*/


#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "main.h"
#include "throttle.h"

#include "../common/config.h"
#include "../common/args.h"
#include "../common/unixdsp.h"
#include "../common/cheat.h"

#include "dface.h"

static char *soundrecfn=0;	/* File name of sound recording. */

static int ntsccol=0,ntschue=0,ntsctint=0;
static int soundvol=100;
static int soundq=0;
#ifdef FRAMESKIP
static int frameskip=0;
#endif
static int inited=0;
static int isloaded=0;	// Is game loaded?

#ifdef STDIOIFACE
int SpecialI=0;
#endif
int srendlinev[2]={8,0};
int erendlinev[2]={231,239};
int srendline,erendline;


static char BaseDirectory[2048];

int eoptions=0;

static void DriverKill(void);
static int DriverInitialize(void);

static int gametype;
#include "input.c"

static void ParseGI(FCEUGI *gi)
{
 gametype=gi->type;

 InputType[0]=UsrInputType[0];
 InputType[1]=UsrInputType[1];
 InputTypeFC=UsrInputTypeFC;

 if(gi->input[0]>=0)
  InputType[0]=gi->input[0];
 if(gi->input[1]>=0)
  InputType[1]=gi->input[1];
 if(gi->inputfc>=0)
  InputTypeFC=gi->inputfc;
 FCEUI_GetCurrentVidSystem(&srendline,&erendline);
}

void FCEUD_PrintError(char *s)
{
 #ifdef EXTGUI
 GUI_TextOut(s);
 GUI_TextOut("\n"); // Bleh
 #else
 puts(s);
 #endif
}

void FCEUD_Message(char *s)
{
 #ifdef EXTGUI
 GUI_TextOut(s);
 #else
 fputs(s,stdout);
 #endif
}

static char *cpalette=0;
static void LoadCPalette(void)
{
 char tmpp[192];
 FILE *fp;

 if(!(fp=fopen(cpalette,"rb")))
 {
  printf(" Error loading custom palette from file: %s\n",cpalette);
  return;
 }
 fread(tmpp,1,192,fp);
 FCEUI_SetPaletteArray(tmpp);
 fclose(fp);
}
#ifdef EXTGUI
extern CFGSTRUCT GUIConfig;
#endif
static CFGSTRUCT fceuconfig[]={
	AC(soundq),
	AC(soundvol),
	ACS(cpalette),
	AC(ntsctint),
	AC(ntschue),
	AC(ntsccol),
	AC(UsrInputTypeFC),
	ACA(UsrInputType),
	AC(powerpadside),
	AC(powerpadsc),
	AC(eoptions),
	ACA(srendlinev),
	ACA(erendlinev),
	ADDCFGSTRUCT(DriverConfig),
	#ifdef EXTGUI
	ADDCFGSTRUCT(GUIConfig),
	#endif
	ENDCFGSTRUCT
};

static void SaveConfig(void)
{	
	char tdir[2048];
	sprintf(tdir,"%s"PSS"fceu.cfg",BaseDirectory);
	FCEUI_GetNTSCTH(&ntsctint, &ntschue);
        SaveFCEUConfig(tdir,fceuconfig);
}

static void LoadConfig(void)
{
	char tdir[2048];
        sprintf(tdir,"%s"PSS"fceu.cfg",BaseDirectory);
	FCEUI_GetNTSCTH(&ntsctint, &ntschue);	/* Get default settings for if
					   no config file exists. */
        LoadFCEUConfig(tdir,fceuconfig);
}

static void CreateDirs(void)
{
 char *subs[5]={"fcs","snaps","gameinfo","sav","cheats"};
 char tdir[2048];
 int x;

 mkdir(BaseDirectory,S_IRWXU);
 for(x=0;x<5;x++)
 {
  sprintf(tdir,"%s"PSS"%s",BaseDirectory,subs[x]);
  mkdir(tdir,S_IRWXU);
 }
}

static void SetSignals(void (*t)(int))
{
  int sigs[11]={SIGINT,SIGTERM,SIGHUP,SIGPIPE,SIGSEGV,SIGFPE,SIGKILL,SIGALRM,SIGABRT,SIGUSR1,SIGUSR2};
  int x;
  for(x=0;x<11;x++)
   signal(sigs[x],t);
}

static void CloseStuff(int signum)
{
	DriverKill();
        printf("\nSignal %d has been caught and dealt with...\n",signum);
        switch(signum)
        {
         case SIGINT:printf("How DARE you interrupt me!\n");break;
         case SIGTERM:printf("MUST TERMINATE ALL HUMANS\n");break;
         case SIGHUP:printf("Reach out and hang-up on someone.\n");break;
         case SIGPIPE:printf("The pipe has broken!  Better watch out for floods...\n");break;
         case SIGSEGV:printf("Iyeeeeeeeee!!!  A segmentation fault has occurred.  Have a fluffy day.\n");break;
	 /* So much SIGBUS evil. */
	 #ifdef SIGBUS
	 #if(SIGBUS!=SIGSEGV)
         case SIGBUS:printf("I told you to be nice to the driver.\n");break;
	 #endif
	 #endif
         case SIGFPE:printf("Those darn floating points.  Ne'er know when they'll bite!\n");break;
         case SIGALRM:printf("Don't throw your clock at the meowing cats!\n");break;
         case SIGABRT:printf("Abort, Retry, Ignore, Fail?\n");break;
         case SIGUSR1:
         case SIGUSR2:printf("Killing your processes is not nice.\n");break;
        }
        exit(1);
}

static void DoArgs(int argc, char *argv[])
{
        static char *cortab[5]={"none","gamepad","zapper","powerpad","arkanoid"};
        static int cortabi[5]={SI_NONE,SI_GAMEPAD,
                               SI_ZAPPER,SI_POWERPAD,SI_ARKANOID};
	static char *fccortab[5]={"none","arkanoid","shadow","4player","fkb"};
	static int fccortabi[5]={SIFC_NONE,SIFC_ARKANOID,SIFC_SHADOW,
			         SIFC_4PLAYER,SIFC_FKB};

	int x;
	static char *inputa[2]={0,0};
	static char *fcexp=0;

        static ARGPSTRUCT FCEUArgs[]={
	 {"-soundq",0,&soundq,0},
#ifdef FRAMESKIP
	 {"-frameskip",0,&frameskip,0},
#endif
         {"-soundvol",0,&soundvol,0},
         {"-cpalette",0,&cpalette,0x4001},
	 {"-soundrecord",0,&soundrecfn,0x4001},

         {"-ntsccol",0,&ntsccol,0},
         {"-pal",0,&eoptions,0x8000|EO_PAL},
         {"-input1",0,&inputa[0],0x4001},{"-input2",0,&inputa[1],0x4001},
         {"-fcexp",0,&fcexp,0x4001},
	 #ifdef STDIOIFACE
	 {"-interface",&SpecialI,0,0},
	 #endif
	 {"-lowpass",0,&eoptions,0x8000|EO_LOWPASS},
         {"-gg",0,&eoptions,0x8000|EO_GAMEGENIE},
         {"-no8lim",0,&eoptions,0x8001},
         {"-subase",0,&eoptions,0x8002},
         {"-snapname",0,&eoptions,0x8000|EO_SNAPNAME},
	 {"-nofs",0,&eoptions,0x8000|EO_NOFOURSCORE},
         {"-clipsides",0,&eoptions,0x8000|EO_CLIPSIDES},
	 {"-nothrottle",0,&eoptions,0x8000|EO_NOTHROTTLE},
         {"-slstart",0,&srendlinev[0],0},{"-slend",0,&erendlinev[0],0},
         {"-slstartp",0,&srendlinev[1],0},{"-slendp",0,&erendlinev[1],0},
	 {0,(void *)DriverArgs,0,0},
	 {0,0,0,0}
        };

	ParseArguments(argc, argv, FCEUArgs);
	if(cpalette)
	{
  	 if(cpalette[0]=='0')
	  if(cpalette[1]==0)
	  {
	   free(cpalette);
	   cpalette=0;
	  }
	}
	FCEUI_SetVidSystem((eoptions&EO_PAL)?1:0);
	FCEUI_SetGameGenie((eoptions&EO_GAMEGENIE)?1:0);
	FCEUI_SetLowPass((eoptions&EO_LOWPASS)?1:0);

        FCEUI_DisableSpriteLimitation(eoptions&1);
        FCEUI_SaveExtraDataUnderBase(eoptions&2);
	FCEUI_SetSnapName(eoptions&EO_SNAPNAME);

	for(x=0;x<2;x++)
	{
         if(srendlinev[x]<0 || srendlinev[x]>239) srendlinev[x]=0;
         if(erendlinev[x]<srendlinev[x] || erendlinev[x]>239) erendlinev[x]=239;
	}

        FCEUI_SetRenderedLines(srendlinev[0],erendlinev[0],srendlinev[1],erendlinev[1]);
        FCEUI_SetSoundVolume(soundvol);
	FCEUI_SetSoundQuality(soundq);
	DoDriverArgs();

	if(fcexp)
	{
	 int y;
         for(y=0;y<5;y++)
         {
          if(!strncmp(fccortab[y],fcexp,8))
          {
	   UsrInputTypeFC=fccortabi[y];
	   break;
          }
         }
	 free(fcexp);
	}
	for(x=0;x<2;x++)
	{
	 int y;

         if(!inputa[x])
	  continue;

	 for(y=0;y<5;y++)	 
	 {
	  if(!strncmp(cortab[y],inputa[x],8))
	  {
	   UsrInputType[x]=cortabi[y];
	   if(y==3)
	   {
	    powerpadside&=~(1<<x);
	    powerpadside|=((((inputa[x][8])-'a')&1)^1)<<x;
	   }
	   free(inputa[x]);
	  }
	 }
	}	
}

#include "usage.h"

/* Loads a game, given a full path/filename.  The driver code must be
   initialized after the game is loaded, because the emulator code
   provides data necessary for the driver code(number of scanlines to
   render, what virtual input devices to use, etc.).
*/
int LoadGame(char *path)
{
	FCEUGI *tmp;

	CloseGame();
        if(!(tmp=FCEUI_LoadGame(path)))
	 return 0;
        ParseGI(tmp);
        RefreshThrottleFPS();
        if(!DriverInitialize())
         return(0);  
	if(soundrecfn)
	{
	 if(!FCEUI_BeginWaveRecord(soundrecfn))
	 {
 	  free(soundrecfn);
	  soundrecfn=0;
	 }
	}
	isloaded=1;
	return 1;
}

/* Closes a game.  Frees memory, and deinitializes the drivers. */
int CloseGame(void)
{
	if(!isloaded) return(0);
	FCEUI_CloseGame();
	DriverKill();
	isloaded=0;

	if(soundrecfn)
         FCEUI_EndWaveRecord();

	return(1);
}

int CLImain(int argc, char *argv[])
{
	int ret,r;

	if(!(ret=FCEUI_Initialize()))
         return(1);
        GetBaseDirectory(BaseDirectory);
	FCEUI_SetBaseDirectory(BaseDirectory);

	CreateDirs();

	#ifdef EXTGUI
	if(argc==2 && !strcmp(argv[1],"-help")) // I hope no one has a game named "-help" :b
	#else
        if(argc<=1) 
	#endif
        {
         ShowUsage(argv[0]);
         return 1;
        }

        LoadConfig();
        DoArgs(argc-2,&argv[1]);
	FCEUI_SetNTSCTH(ntsccol, ntsctint, ntschue);
	if(cpalette)
	 LoadCPalette();

	/* All the config files and arguments are parsed now. */
	#ifdef EXTGUI
	GUI_Init(&argc,&argv);

	if(argc>1)
	 LoadGame(argv[argc-1]);
	while(GUI_Idle())
	{
	 if(isloaded)
	  FCEUI_Emulate();
	}
	#else
        if(!LoadGame(argv[argc-1]))
        {
	 DriverKill();
	 return(-1);
        }

	FCEUI_Emulate();
	CloseGame();
	#endif
	FCEUI_Kill();
        return(1);
}

static int DriverInitialize(void)
{
	int r;
	SetSignals((void *)CloseStuff);
        
	if((r=InitSound()))
        {
         inited|=1;
         if(soundq)
          if((r<44100 || r>48000) && r!=96000)
          {
           printf("    Sound playback rate of %d is out of the acceptable range(44100-48000, 96000) when high-quality sound is enabled.\n",r);
           KillSound();   
           FCEUI_Sound(0);
           inited&=~1;
          }
        }

	if(InitJoysticks())
	 inited|=2;
	if(!InitVideo()) return 0;
	inited|=4;
	if(!InitKeyboard()) return 0;
	inited|=8;

	InitOtherInput();
	return 1;
}

static void DriverKill(void)
{
 SaveConfig();
 SetSignals(SIG_IGN);

 if(inited&2)
  KillJoysticks();
 if(inited&8)
  KillKeyboard();
 if(inited&4)
  KillVideo();
 if(inited&1)
  KillSound();
 if(inited&16)
  KillMouse();
 inited=0;
}

void FCEUD_Update(uint8 *XBuf, int32 *Buffer, int Count)
{
 if(!Count && !NoWaiting && !(eoptions&EO_NOTHROTTLE))
  SpeedThrottle();
 if(XBuf)
 {
  #ifdef FRAMESKIP
  if(frameskip)
   FCEUI_FrameSkip(frameskip);
  #endif
  BlitScreen(XBuf);
 }
 if(Count)
  WriteSound(Buffer,Count,NoWaiting);
 FCEUD_UpdateInput();
 #ifdef EXTGUI
 GUI_Update();
 #endif
}

