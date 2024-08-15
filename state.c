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

/*  TODO: Add (better) file io error checking */
/*  TODO: Change save state file format. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "x6502.h"
#include "version.h"
#include "fce.h"
#include "sound.h"
#include "ines.h"
#include "svga.h"
#include "endian.h"
#include "fds.h"
#include "general.h"
#include "state.h"
#include "memory.h"
#include "ppu.h"

static SFORMAT SFMDATA[64];
static int SFEXINDEX;
static int stateversion;

#define RLSB 		FCEUSTATE_RLSB
//0x80000000


extern SFORMAT FCEUPPU_STATEINFO[];
extern SFORMAT FCEUSND_STATEINFO[];

SFORMAT SFCPU[]={
 { &X.PC, 2|RLSB, "PC\0"},
 { &X.A, 1, "A\0\0"},
 { &X.P, 1, "P\0\0"},
 { &X.X, 1, "X\0\0"},
 { &X.Y, 1, "Y\0\0"},
 { &X.S, 1, "S\0\0"},
 { RAM, 0x800, "RAM"},
 { 0 }
};

SFORMAT SFCPUC[]={
 { &X.jammed, 1, "JAMM"},
 { &X.IRQlow, 4|RLSB, "IQLB"},
 { &X.tcount, 4|RLSB, "ICoa"},
 { &X.count,  4|RLSB, "ICou"},
 { 0 }
};

// Is this chunk necessary?  I'll fix it later.
//SFORMAT SFCTLR[]={
// { &joy_readbit, 1, "J1RB"},
// { &joy2_readbit, 1, "J2RB"}
//};

static int WriteStateChunk(FILE *st, int type, SFORMAT *sf)
{
 int bsize;
 int x;

 fputc(type,st);
 
 x=-1;
 bsize=0;
 while(sf[++x].v)
  bsize+=sf[x].s&(~RLSB);
 bsize+=(x-1)<<3;
 //count<<3;
 write32(bsize,st);

 x=-1;
 while(sf[++x].v)
 {
  fwrite(sf[x].desc,1,4,st);
  write32(sf[x].s&(~RLSB),st);
  #ifdef LSB_FIRST
  fwrite((uint8 *)sf[x].v,1,sf[x].s&(~RLSB),st);
  #else
  {
  int z;
  if(sf[x].s&RLSB)
  {
   for(z=(sf[x].s&(~RLSB))-1;z>=0;z--)
   {
    fputc(*(((uint8*)sf[x].v)+z),st);
   }
  }
  else
   fwrite((uint8 *)sf[x].v,1,sf[x].s&(~RLSB),st);
  }
  #endif
 }
 return (bsize+5);
}

static int ReadStateChunk(FILE *st, SFORMAT *sf, int size)
{
 int bsize;
 int x;

 x=-1;
 bsize=0;
 while(sf[++x].v)
  bsize+=sf[x].s&(~RLSB);
 bsize+=(x-1)<<3;
	//count<<3;

 {
  int temp;
  temp=ftell(st);

  while(ftell(st)<temp+size)
  {
   int tsize;
   char toa[4];

   if(fread(toa,1,4,st)<=0)
    return 0;
   read32(&tsize,st);

   x=-1;
   while(sf[++x].v)
   {
    //printf("test, %d %s %.4s\n",x,sf[x].desc,toa);
    if(!memcmp(toa,sf[x].desc,4))
    {
     if(tsize!=(sf[x].s&(~RLSB)))
      goto nkayo;
     #ifndef LSB_FIRST
     if(sf[x].s&RLSB)
     {
      int z;
       for(z=(sf[x].s&(~RLSB))-1;z>=0;z--)       
        *(((uint8*)sf[x].v)+z)=fgetc(st);
     }
     else
     #endif
     {
       fread((uint8 *)sf[x].v,1,sf[x].s&(~RLSB),st);
     }
     goto bloo;
    }
   }
  nkayo:
  //printf("Not ok %d, %d, %d, %s, %s.\n",tsize,x,(sf[x].s&(~RLSB)),sf[x].desc,sf[x].v);
  fseek(st,tsize,SEEK_CUR);
  bloo:;
  } // while(...)
 }
 return 1;
}

static int ReadStateChunks(FILE *st)
{
 int t;
 uint32 size;
 int ret=1;

 for(;;)
 {
  t=fgetc(st);
  if(t==EOF) break;
  if(!read32(&size,st)) break;
  switch(t)
  {
   case 1:if(!ReadStateChunk(st,SFCPU,size)) ret=0;break;
   case 2:if(!ReadStateChunk(st,SFCPUC,size)) ret=0;
          else
	  {
	   X.mooPI=X.P;	// Quick and dirty hack.
	  }
	  break;
   case 3:if(!ReadStateChunk(st,FCEUPPU_STATEINFO,size)) ret=0;break;
   //   case 4:if(!ReadStateChunk(st,SFCTLR,SFCTLRELEMENTS,size)) ret=0;break;
   case 5:if(!ReadStateChunk(st,FCEUSND_STATEINFO,size)) ret=0;break;
   case 0x10:if(!ReadStateChunk(st,SFMDATA,size)) ret=0;break;
   default: if(fseek(st,size,SEEK_CUR)<0) goto endo;break;
  }
 }
 endo:
 return ret;
}


int CurrentState=0;
extern int geniestage;
void SaveState(void)
{
	FILE *st=NULL;

	if(geniestage==1)
	{
	 FCEU_DispMessage("Cannot save FCS in GG screen.");
	 return;
        }

	 st=fopen(FCEU_MakeFName(FCEUMKF_STATE,CurrentState,0),"wb");

	 if(st!=NULL)
	 {
	  static uint32 totalsize;
	  static uint8 header[16]="FCS";

	  memset(header+4,0,13);
	  header[3]=VERSION_NUMERIC;
	  fwrite(header,1,16,st);

	  FCEUPPU_SaveState();
	  FCEUSND_SaveState();

	  totalsize=WriteStateChunk(st,1,SFCPU);
	  totalsize+=WriteStateChunk(st,2,SFCPUC);
	  totalsize+=WriteStateChunk(st,3,FCEUPPU_STATEINFO);
	//  totalsize+=WriteStateChunk(st,4,SFCTLR);
	  totalsize+=WriteStateChunk(st,5,FCEUSND_STATEINFO);
	  totalsize+=WriteStateChunk(st,0x10,SFMDATA);
	
	  fseek(st,4,SEEK_SET);
	  write32(totalsize,st);
	  SaveStateStatus[CurrentState]=1;
	  fclose(st);
	  FCEU_DispMessage("State %d saved.",CurrentState);
	 }
	 else
	  FCEU_DispMessage("State %d save error.",CurrentState);
}

void LoadState(void)
{
	int x;
	FILE *st=NULL;

        if(geniestage==1)
        {
         FCEU_DispMessage("Cannot load FCS in GG screen.");
         return;
        }

	st=fopen(FCEU_MakeFName(FCEUMKF_STATE,CurrentState,0),"rb");

	if(st!=NULL)
	{
	 uint8 header[16];

         fread(&header,1,16,st);
         if(memcmp(header,"FCS",3))
         {
          fseek(st,0,SEEK_SET);    
          goto okload;
         }
         stateversion=header[3];
	 x=ReadStateChunks(st);
	 if(stateversion<95) X.IRQlow=0;
	 if(!PCMfreq) PSG[0x15]&=~0x10; // Quick fix for <0.95 release

	 if(GameStateRestore) GameStateRestore(header[3]);
	 if(x)
	 {
	  okload:

	  FCEUPPU_LoadState(header[3]);
	  FCEUSND_LoadState(header[3]);

	  SaveStateStatus[CurrentState]=1;
	  FCEU_DispMessage("State %d loaded.",CurrentState);
	  SaveStateStatus[CurrentState]=1;
	 }
	 else
	 {
	  SaveStateStatus[CurrentState]=1;
	  FCEU_DispMessage("Error(s) reading state %d!",CurrentState);
	 }
	}
	else
	{
	 FCEU_DispMessage("State %d load error.",CurrentState);
	 SaveStateStatus[CurrentState]=0;
	 return;
	}
	fclose(st);
}

int SaveStateStatus[10];
void CheckStates(void)
{
	FILE *st=NULL;
	int ssel;

	if(SaveStateStatus[0]==-1)
 	 for(ssel=0;ssel<10;ssel++)
	 {
	  st=fopen(FCEU_MakeFName(FCEUMKF_STATE,ssel,0),"rb");
          if(st)
          {
           SaveStateStatus[ssel]=1;
           fclose(st);
          }
          else
           SaveStateStatus[ssel]=0;
	 }
}

void SaveStateRefresh(void)
{
 SaveStateStatus[0]=-1;
 CurrentState=0;
}

void ResetExState(void)
{
 int x;
 for(x=0;x<SFEXINDEX;x++)
  free(SFMDATA[x].desc);
 SFEXINDEX=0;
}

void AddExState(void *v, uint32 s, int type, char *desc)
{
 SFMDATA[SFEXINDEX].desc=FCEU_malloc(5);
 if(SFMDATA[SFEXINDEX].desc)
 {
  strcpy(SFMDATA[SFEXINDEX].desc,desc);
  SFMDATA[SFEXINDEX].v=v;
  SFMDATA[SFEXINDEX].s=s;
  if(type) SFMDATA[SFEXINDEX].s|=RLSB;
  if(SFEXINDEX<63) SFEXINDEX++;
  SFMDATA[SFEXINDEX].v=0;		// End marker.
 }
}

