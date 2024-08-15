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

/*                      SVGA High Level Routines
                          FCE / FCE Ultra
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdarg.h>


#include "types.h"
#include "svga.h" 
#include "fce.h"
#include "general.h"
#include "video.h"
#include "sound.h"
#include "version.h"
#include "nsf.h"
#include "palette.h"
#include "fds.h"
#include "netplay.h"
#include "state.h"
#include "cart.h"
#include "input.h"

FCEUS FSettings;

static int howlong;
static char errmsg[65];

void FCEU_printf(char *format, ...)
{
 char temp[2048];

 va_list ap;

 va_start(ap,format);
 vsprintf(temp,format,ap);
 FCEUD_Message(temp);

 va_end(ap);
}

void FCEU_PrintError(char *format, ...)
{
 char temp[2048];

 va_list ap;

 va_start(ap,format);
 vsprintf(temp,format,ap);
 FCEUD_PrintError(temp);

 va_end(ap);
}

void FCEU_DispMessage(char *format, ...)
{
 va_list ap;

 va_start(ap,format);
 vsprintf(errmsg,format,ap);
 va_end(ap);

 howlong=180;
}

void FCEUI_SetRenderedLines(int ntscf, int ntscl, int palf, int pall)
{
 FSettings.UsrFirstSLine[0]=ntscf;
 FSettings.UsrLastSLine[0]=ntscl;
 FSettings.UsrFirstSLine[1]=palf;
 FSettings.UsrLastSLine[1]=pall;
 if(PAL)
 {
  FSettings.FirstSLine=FSettings.UsrFirstSLine[1];
  FSettings.LastSLine=FSettings.UsrLastSLine[1];
 }
 else
 {
  FSettings.FirstSLine=FSettings.UsrFirstSLine[0];
  FSettings.LastSLine=FSettings.UsrLastSLine[0];
 }

}

void FCEUI_SetVidSystem(int a)
{
 FSettings.PAL=a?1:0;
 FCEU_ResetVidSys();
 FCEU_ResetPalette();
}

int FCEUI_GetCurrentVidSystem(int *slstart, int *slend)
{
 if(slstart)
  *slstart=FSettings.FirstSLine;
 if(slend)
  *slend=FSettings.LastSLine;
 return(PAL);
}

#ifdef NETWORK
void FCEUI_SetNetworkPlay(int type)
{
 FSettings.NetworkPlay=type;
}
#endif

void FCEUI_SetGameGenie(int a)
{
 FSettings.GameGenie=a?1:0;
}

#ifndef NETWORK
#define netplay 0
#endif

static uint8 StateShow=0;

uint8 Exit=0;

uint8 DIPS=0;
uint8 vsdip=0;
int coinon=0;

uint8 CommandQueue=0;

void FCEUI_SetSnapName(int a)
{
 FSettings.SnapName=a;
}

void FCEUI_SaveExtraDataUnderBase(int a)
{
 FSettings.SUnderBase=a;
}

void FCEUI_SelectState(int w)
{
 if(FCEUGameInfo.type!=GIT_NSF) 
  CommandQueue=42+w;
}

void FCEUI_SaveState(void)
{
 if(FCEUGameInfo.type!=GIT_NSF)
  CommandQueue=40;
}

void FCEUI_LoadState(void)
{
 if(FCEUGameInfo.type!=GIT_NSF) 
  CommandQueue=41;
}

int32 FCEUI_GetDesiredFPS(void)
{
  if(PAL)
   return(838977920); // ~50.007
  else
   return(1008307711);	// ~60.1
}

static int dosnapsave=0;
void FCEUI_SaveSnapshot(void)
{
 dosnapsave=1;
}

/* I like the sounds of breaking necks. */
static void ReallySnap(void)
{
 int x=SaveSnapshot();
 if(!x)
  FCEU_DispMessage("Error saving screen snapshot.");
 else
  FCEU_DispMessage("Screen snapshot %d saved.",x-1);
}


void FCEUI_ResetNES(void)
{
	if(netplay)
	 CommandQueue=30;
        else   /* Needed for proper functioning of debug code. */
         ResetNES();
}

void FCEUI_PowerNES(void)
{
	CommandQueue=31;
}

void DriverInterface(int w, void *d)
{
 switch(w)
 {
  case DES_FDSINSERT:CommandQueue=2;break;
  case DES_FDSSELECT:CommandQueue=1;break;

  case DES_VSUNIDIPSET:CommandQueue=10+(int)d;break;
  case DES_VSUNITOGGLEDIPVIEW:CommandQueue=10;break;
  case DES_VSUNICOIN:CommandQueue=19;break;
  }
}

#include "drawing.h"
#ifdef FRAMESKIP

void FCEU_PutImageDummy(void)
{
 if(FCEUGameInfo.type!=GIT_NSF)
 {
  FCEU_DrawNTSCControlBars(XBuf);
  if(StateShow) StateShow--; /* DrawState() */
 }
 if(howlong) howlong--; /* DrawMessage() */
 #ifdef FPS
 {
  extern uint64 frcount;
  frcount++;
 }
 #endif
}
#endif

void FCEU_PutImage(void)
{
        if(FCEUGameInfo.type==GIT_NSF)
	{
         DrawNSF(XBuf);
	 /* Save snapshot after NSF screen is drawn.  Why would we want to
	    do it before?
	 */
         if(dosnapsave)
         {
          ReallySnap();
          dosnapsave=0;
         }
	}
        else
        {	
	 /* Save snapshot before overlay stuff is written. */
         if(dosnapsave)
         {
          ReallySnap();
          dosnapsave=0;
         }
	 if(FCEUGameInfo.type==GIT_VSUNI && DIPS&2)         
	  DrawDips();
         if(StateShow) DrawState();
	 FCEU_DrawNTSCControlBars(XBuf);
        }
	DrawMessage();
        #ifdef FPS
	{
         extern uint64 frcount;
         frcount++;
	}
        #endif
        FCEU_DrawInput(XBuf+8);
}

void FlushCommandQueue(void)
{
  if(!netplay && CommandQueue) {DoCommand(CommandQueue);CommandQueue=0;}
}

void DoCommand(uint8 c)
{
 switch(c)
 {
  case 1:FDSControl(FDS_SELECT);break;
  case 2:FDSControl(FDS_IDISK);break;

  case 10:DIPS^=2;break;
  case 11:vsdip^=1;DIPS|=2;break;
  case 12:vsdip^=2;DIPS|=2;break;
  case 13:vsdip^=4;DIPS|=2;break;
  case 14:vsdip^=8;DIPS|=2;break;
  case 15:vsdip^=0x10;DIPS|=2;break;
  case 16:vsdip^=0x20;DIPS|=2;break;
  case 17:vsdip^=0x40;DIPS|=2;break;
  case 18:vsdip^=0x80;DIPS|=2;break;
  case 19:coinon=6;break;
  case 30:ResetNES();break;
  case 31:PowerNES();break;
  case 40:CheckStates();StateShow=0;SaveState();break;
  case 41:CheckStates();StateShow=0;LoadState();break;
  case 42: case 43: case 44: case 45: case 46: case 47: case 48: case 49:
  case 50: case 51:StateShow=180;CurrentState=c-42;CheckStates();break;
 }
}

void FCEU_ResetMessages(void)
{
 StateShow=0;
 SaveStateRefresh();
 howlong=0;
}
