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
#include <string.h>
#include <stdlib.h>

#include "sdl.h"
#ifdef UNIXDSP
#include "../common/unixdsp.h"
#endif

#include "../common/sfifo.h"

static int16 *AudioBuf=NULL;
static volatile uint32 readoffs,writeoffs;
static volatile uint32 inbuffer;

static sfifo_t fifi;

void fillaudio(void *udata, uint8 *stream, int len)
{
 int r;
 r=sfifo_read(&fifi,stream,len);
 len-=r;
 if(len) memset(stream+r,0,len);
 // if(len) printf("Underflow by %d\n",len);
}

void WriteSound(int32 *Buffer, int Count, int NoWaiting)
{
 int16 tbuffer[2048+1024];	// 96000/50 = 1920
 char *tmp;
 int t;

 #ifdef UNIXDSP
 if(_devdsp)
 {
  WriteUNIXDSPSound(Buffer, Count, NoWaiting);
  return;
 }
 #endif

 t=0;
 while(t<Count)
 {
  tbuffer[t]=Buffer[t];
  t++;
 }

 Count<<=1;

 tmp=(char *)tbuffer;
 while(Count)
 {
  int r;
  r=sfifo_write(&fifi,tmp,Count);
  Count-=r;
  tmp+=r;
  if(!r) 	// No bytes written, sooo...
  {
   if(NoWaiting)
    return;
   else
    SDL_Delay(1);
  }
 }
}

int InitSound(void)
{
 #ifdef UNIXDSP
 if(_devdsp)
 {
        if(_sound)
        {
         int rate;
         if(_sound==1)
          _sound=48000;
         rate=_sound;
         if(InitUNIXDSPSound(&rate,1,7,8,_devdsp>0?_devdsp-1:-1))
         {
          FCEUI_Sound(rate);
          return(rate);
         }
        }
        return(0);  
 }
 #endif
 if(_sound)
 {
  SDL_AudioSpec spec;

  if(_lbufsize<_ebufsize) 
  {
   puts("Ack, lbufsize must not be smaller than ebufsize!");
   return(0);
  }
  if(_lbufsize<6)
  {
   puts("lbufsize out of range");
   return(0);
  }
  if(_ebufsize<5)
  {
   puts("ebufsize out of range");
   return(0);
  }
  memset(&spec,0,sizeof(spec));
  if(SDL_InitSubSystem(SDL_INIT_AUDIO)<0)
  {
   puts(SDL_GetError());
   return(0);
  }

  sfifo_init(&fifi,(1<<_lbufsize)*sizeof(int16),2);
  AudioBuf=malloc((1<<_lbufsize)*sizeof(int16));
  readoffs=writeoffs=inbuffer=0;
  if(_sound==1) _sound=48000;
  spec.freq=_sound;
  spec.format=AUDIO_S16SYS;
  spec.channels=1;
  spec.samples=1<<_ebufsize;
  spec.callback=fillaudio;
  spec.userdata=0;

  if(SDL_OpenAudio(&spec,0)<0)
  {
   puts(SDL_GetError());
   SDL_QuitSubSystem(SDL_INIT_AUDIO);
   return(0);
  }
  printf("%d, %d\n",spec.samples,1<<_lbufsize);
  FCEUI_Sound(_sound);
  SDL_PauseAudio(0);
  return(_sound);
 }
 return(0);
}

void SilenceSound(int n)
{
 SDL_PauseAudio(n);

}

void KillSound(void)
{
 #ifdef UNIXDSP
 if(_devdsp)
  KillUNIXDSPSound();
 else
 #endif
 {
  SDL_CloseAudio();
  SDL_QuitSubSystem(SDL_INIT_AUDIO);
  sfifo_close(&fifi);
 }
}
