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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sched.h>
#include <sys/soundcard.h>

#include "../../types.h"
#include "../../driver.h"

static int format;
static int dspfd;
static int stereo;

// fsize is in samples, not bytes(gets translated before ioctl())
int InitUNIXDSPSound(int *rate, int bits, int fsize, int nfrags, int dev)
{
 int x;
 char buf[32];

 if(dev==-1)
 {
  x=-1;
  do
  {
   if(x==-1) 
    strcpy(buf,"/dev/dsp");
   else
    sprintf(buf,"/dev/dsp%d",x);
   FCEUI_printf("  Trying %s...",buf);
   dspfd=open(buf,O_WRONLY|O_NONBLOCK);
   if(dspfd!=-1) break;
   FCEUI_printf(" Nope.\n");
   x++;
  } while(errno!=ENOENT);
  if(dspfd==-1) return(0);
  fcntl(dspfd,F_SETFL,fcntl(dspfd,F_GETFL)&~O_NONBLOCK);
 }
 else
 {
  sprintf(buf,"/dev/dsp%d",dev);
  FCEUI_printf("  Opening %s...",buf);
  dspfd=open(buf,O_WRONLY);
  if(dspfd==-1) goto __disperror;
 }

 if(!bits) goto skip16check;
 x=AFMT_S16_LE;
 format=0;
 FCEUI_printf("\n   Setting format to 16-bit, signed, LSB first...");
 if(ioctl(dspfd,SNDCTL_DSP_SETFMT,&x)==-1)
 {
  skip16check:
  x=AFMT_U8;
  FCEUI_printf("\n   Setting format to 8-bit, unsigned...");
  if(ioctl(dspfd,SNDCTL_DSP_SETFMT,&x)==-1) goto __disperror;
  format=1;
 }

 FCEUI_printf("\n   Setting fragment size to %d samples and number of fragments to %d...",1<<fsize,nfrags);

 if(!format)
  fsize++;
 x=fsize|(nfrags<<16);

 if(ioctl(dspfd,SNDCTL_DSP_SETFRAGMENT,&x)==-1)
  FCEUI_printf("ERROR (continuing anyway)\n");

 x=1;
 FCEUI_printf("\n   Setting mono sound...");  
 if(ioctl(dspfd,SNDCTL_DSP_CHANNELS,&x)==-1)
 {
  x=2;
  FCEUI_printf("Nope.  Trying stereo sound...");
  if(ioctl(dspfd,SNDCTL_DSP_CHANNELS,&x)==-1 || x!=2)
   goto __disperror;
 }
 stereo=x-1;

 FCEUI_printf("\n   Setting playback rate of %d hz...",*rate);
 if(ioctl(dspfd,SNDCTL_DSP_SPEED,rate)==-1) goto __disperror;
 FCEUI_printf("Set to %d hz\n",*rate);
 return 1;

 __disperror:
 FCEUI_printf("ERROR\n");
 return 0;
}

void KillUNIXDSPSound(void)
{
  close(dspfd);
}

void WriteUNIXDSPSound(int32 *Buffer, int Count, int noblocking)
{
 int P,c;
 int32 *src=Buffer;
 int16 MBuffer[4096];

 if(format)
 {
  uint8 *dest=(uint8 *)MBuffer;
  if(stereo)
   for(P=Count;P;P--,dest+=2,src++)
    *dest=*(dest+1)=(uint8)((*src)>>8)^128;
  else
   for(P=Count;P;P--,dest++,src++)
    *dest=(uint8)((*src)>>8)^128;
  c=Count;
 }
 else
 {
  int16 *dest=MBuffer;

  if(stereo)
   for(P=Count;P;P--,dest+=2,src++)
    *dest=*(dest+1)=*src;
  else
   for(P=Count;P;P--,dest++,src++)
    *dest=*src;
  c=Count<<1;
 }

 c*=stereo+1;

 //noblocking=!noblocking; // speed testing
 if(noblocking)
 {
  struct audio_buf_info ai;
  if(!ioctl(dspfd,SNDCTL_DSP_GETOSPACE,&ai))
   if(ai.bytes<c)
    return;
 }
 write(dspfd,(uint8 *)MBuffer,c);
}
