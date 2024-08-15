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

/****************************************/
/*		FCE Ultra		*/
/*					*/
/*		video.c			*/
/*					*/
/*  Some generic high-level video	*/
/*  related functions.			*/
/****************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "types.h"
#include "video.h"
#include "fce.h"
#include "svga.h"
#include "version.h"
#include "general.h"
#include "memory.h"

uint8 *XBuf=NULL;
static uint8 *xbsave=NULL;

void FCEU_KillVirtualVideo(void)
{
 if(xbsave)
 {
  free(xbsave);
  xbsave=0;
 }
}

int FCEU_InitVirtualVideo(void)
{
 if(!XBuf)		/* Some driver code may allocate XBuf externally. */
			/* 272 bytes per scanline, * 240 scanline maximum, +8 for alignment,
			   and another +8 to catch overflow(the scanline rendering code
			   will draw up to 7 bytes past the 272-byte scanline limitation, but
			   this is only a problem on the last line, and malloc()'ing an
			   additional 8 bytes fixes it.)
			*/
 // if(!(XBuf = (uint8*) (FCEU_malloc((256+16) * 240 + 8 + 8))))
 //  return 0;
 // 256(272)x256 for lazy opengl goodness.
 if(!(XBuf= (uint8*) (FCEU_malloc((256+16) * 256 + 8))))
  return 0;
 xbsave=XBuf;

 if(sizeof(uint8*)==4)
 {
  uint32 m;
  m=(uint32)XBuf;
  m+=8;
  m&=~(uint32)7;
  XBuf=(uint8 *)m;
 } 
 memset(XBuf,128,272*256); //*240);
 return 1;
}

#ifndef ZLIB
static uint8 pcxheader[128] =
{
 10,5,1,8,1,0,1,0,0,1,240,0,2,1,234,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

int SaveSnapshot(void)
{
 char *fn=0;
 uint8 *tmp;
 int x,u,y;
 FILE *pp=NULL;

 for(u=0;u<999;u++)
 {
  pp=fopen((fn=FCEU_MakeFName(FCEUMKF_SNAP,u,"pcx")),"rb");
  if(pp==NULL) break;
  fclose(pp);
 }

 if(!(pp=fopen(fn,"wb")))
  return 0;

 {
  int totallines=FSettings.LastSLine-FSettings.FirstSLine+1;

  tmp=XBuf+8+FSettings.FirstSLine*272;

  pcxheader[10]=totallines;
  fwrite(pcxheader,1,128,pp);
  for(y=0;y<totallines;y++)
  {
   for(x=0;x<256;x++)
   {
    if(*tmp>=0xc0) fputc(0xC1,pp);
     fputc(*tmp,pp);
    tmp++;
   }
   tmp+=16;
  }
 }

 fputc(0xC,pp);
 for(x=0;x<256;x++)
 {
  uint8 r,g,b;

  FCEUD_GetPalette(x,&r,&g,&b);
  fputc(r,pp);
  fputc(g,pp);
  fputc(b,pp);
 }
 fclose(pp);
 
 return u+1;
}

#else

#include <zlib.h>
#include "crc32.h"

static int WritePNGChunk(FILE *fp, uint32 size, char *type, uint8 *data)
{
 uint32 crc;

 uint8 tempo[4];

 tempo[0]=size>>24;
 tempo[1]=size>>16;
 tempo[2]=size>>8;
 tempo[3]=size;

 if(fwrite(tempo,4,1,fp)!=1)
  return 0;
 if(fwrite(type,4,1,fp)!=1)
  return 0;

 if(size)
  if(fwrite(data,1,size,fp)!=size)
   return 0;

 crc=CalcCRC32(0,type,4);
 if(size)
  crc=CalcCRC32(crc,data,size);

 tempo[0]=crc>>24;
 tempo[1]=crc>>16;
 tempo[2]=crc>>8;
 tempo[3]=crc;

 if(fwrite(tempo,4,1,fp)!=1)
  return 0;
 return 1;
}

int SaveSnapshot(void)
{
 char *fn=0;
 int totallines=FSettings.LastSLine-FSettings.FirstSLine+1;
 int x,u,y;
 FILE *pp=NULL;
 uint8 *compmem=NULL;
 uint32 compmemsize=totallines*263+12;

 if(!(compmem=FCEU_malloc(compmemsize)))
  return 0;

 for(u=0;u<999;u++)
 {
  pp=fopen((fn=FCEU_MakeFName(FCEUMKF_SNAP,u,"png")),"rb");
  if(pp==NULL) break;
  fclose(pp);
 }

 if(!(pp=fopen(fn,"wb")))
  return 0;
 {
  static uint8 header[8]={137,80,78,71,13,10,26,10};
  if(fwrite(header,8,1,pp)!=1)
   goto PNGerr;
 }

 {
  uint8 chunko[13];

  chunko[0]=chunko[1]=chunko[3]=0;
  chunko[2]=0x1;			// Width of 256

  chunko[4]=chunko[5]=chunko[6]=0;
  chunko[7]=totallines;			// Height

  chunko[8]=8;				// bit depth
  chunko[9]=3;				// Color type; indexed 8-bit
  chunko[10]=0;				// compression: deflate
  chunko[11]=0;				// Basic adapative filter set(though none are used).
  chunko[12]=0;				// No interlace.

  if(!WritePNGChunk(pp,13,"IHDR",chunko))
   goto PNGerr;
 }

 {
  char pdata[256*3];
  for(x=0;x<256;x++)
   FCEUD_GetPalette(x,pdata+x*3,pdata+x*3+1,pdata+x*3+2);
  if(!WritePNGChunk(pp,256*3,"PLTE",pdata))
   goto PNGerr;
 }

 {
  uint8 *tmp=XBuf+FSettings.FirstSLine*272+8;
  uint8 *dest,*mal,*mork;

  /* If memory couldn't be allocated, just use XBuf(screen contents
     will be corrupted for one frame, though.
  */
  if(!(mal=mork=dest=malloc((totallines<<8)+totallines)))
   mork=dest=XBuf;

  for(y=0;y<totallines;y++)
  {
   *dest=0;			// No filter.
   dest++;
   for(x=256;x;x--,tmp++,dest++)
    *dest=*tmp; 	
   tmp+=16;
  }

  if(compress(compmem,&compmemsize,mork,(totallines<<8)+totallines)!=Z_OK)
  {
   if(mal) free(mal);
   goto PNGerr;
  }
  if(mal) free(mal);
  if(!WritePNGChunk(pp,compmemsize,"IDAT",compmem))
   goto PNGerr;
 }
 if(!WritePNGChunk(pp,0,"IEND",0))
  goto PNGerr;

 free(compmem);
 fclose(pp);

 return u+1;


 PNGerr:
 if(compmem)
  free(compmem);
 if(pp)
  fclose(pp);
 return(0);
}

#endif
