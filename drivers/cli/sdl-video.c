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
#include "sdl-opengl.h"
#include "../common/vidblit.h"

#define _sline srendline
#define _eline erendline

SDL_Surface *screen;
static int curbpp;
static int tlines;
static int inited=0;
#ifdef OPENGL
static double exs,eys;
#else
static int exs,eys;
#endif
static int eefx;

#define NWIDTH	(256-((eoptions&EO_CLIPSIDES)?16:0))
#define NOFFSET	(eoptions&EO_CLIPSIDES?8:0)

static void CleanSurface(void)
{
 uint32 x;

 x=screen->pitch*screen->h;

 if(SDL_MUSTLOCK(screen))
  SDL_LockSurface(screen);

 if(curbpp>8)
  memset((uint8*)screen->pixels,0,x);
 else
  memset((uint8*)screen->pixels, 0x80, x);

 if(SDL_MUSTLOCK(screen))
  SDL_UnlockSurface(screen);

 SDL_UpdateRect(screen, 0, 0, 0, 0);
}

static int paletterefresh;

void KillVideo(void)
{
 if(inited&1)
 {
  #ifdef OPENGL
  if(_opengl)
   KillOpenGL();
  else if(curbpp>8)
  #endif
   KillBlitToHigh();
  SDL_QuitSubSystem(SDL_INIT_VIDEO);
 }
 inited=0;
}

int InitVideo(void)
{
 const SDL_VideoInfo *vinf;
 int flags=0;

 #ifdef OPENGL
 if(_opengl)
  flags=SDL_OPENGL;
 #endif

 if(!(SDL_WasInit(SDL_INIT_VIDEO)&SDL_INIT_VIDEO))
  if(SDL_InitSubSystem(SDL_INIT_VIDEO)==-1)
  {
   puts(SDL_GetError());
   return(0);
  }
 inited|=1;

 SDL_ShowCursor(0);
 tlines=_eline-_sline+1;

 vinf=SDL_GetVideoInfo();

 if(vinf->hw_available)
  flags|=SDL_HWSURFACE;

 if(_fullscreen)
  flags|=SDL_FULLSCREEN;

 flags|=SDL_HWPALETTE;
 #ifdef OPENGL
 if(_opengl)
 {
  puts(" Initializing with OpenGL(Use \"-opengl 0\" to disable)...");
  SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
 }
 #endif
 if(_fullscreen)
 {
  exs=_xscalefs;
  eys=_yscalefs;
  eefx=_efxfs;

  #ifdef OPENGL
  if(!_opengl) {exs=(int)exs;eys=(int)eys;}

  if( (_opengl && !_stretchx) || !_opengl)
  #endif
   if(_xres<NWIDTH*exs)
   {
    puts("xscale out of bounds.");
    KillVideo();
    return(0);
   }

  #ifdef OPENGL
  if( (_opengl && !_stretchy) || !_opengl)
  #endif
   if(_yres<tlines*eys)
   {
    puts("yscale out of bounds.");
    KillVideo();
    return(0);
   }
  screen = SDL_SetVideoMode(_xres, _yres, _bppfs, flags);
  //curbpp=_bppfs;
 }
 else
 {
  exs=_xscale;
  eys=_yscale;
  eefx=_efx;
  #ifdef OPENGL
  if(!_opengl) {exs=(int)exs;eys=(int)eys;}
  screen = SDL_SetVideoMode((NWIDTH*exs), tlines*eys, _opengl?0:_bpp, flags);
  #else
  screen = SDL_SetVideoMode((NWIDTH*exs), tlines*eys, _bpp, flags);
  #endif
  //curbpp=screen->format->BitsPerPixel;
 }
 curbpp=screen->format->BitsPerPixel;
 if(!screen)
 {
  puts(SDL_GetError());
  KillVideo();
  return(0);
 }
 inited=1;

 printf(" Video Mode: %d x %d x %d bpp %s\n",screen->w,screen->h,screen->format->BitsPerPixel,_fullscreen?"full screen":"");
 if(curbpp!=16 && curbpp!=8 && curbpp!=32)
 {
  printf("  Sorry, %dbpp modes are not supported by FCE Ultra.  Supported bit depths are 8bpp, 16bpp, and 32bpp.\n",curbpp);
  KillVideo();
  return(0);
 }

 #ifdef OPENGL
 if(!_opengl)
 #endif
  CleanSurface();

 SDL_WM_SetCaption("FCE Ultra","FCE Ultra");
 paletterefresh=1;

 if(curbpp>8)
 #ifdef OPENGL
  if(!_opengl)
 #endif
  InitBlitToHigh(curbpp>>3,screen->format->Rmask,screen->format->Gmask,screen->format->Bmask,eefx);
 #ifdef OPENGL
 if(_opengl)
  InitOpenGL((eoptions&EO_CLIPSIDES)?8:0,256-((eoptions&EO_CLIPSIDES)?8:0),_sline,_eline+1,exs,eys,eefx,_stretchx,_stretchy,screen);
 #endif
 return 1;
}

void ToggleFS(void)
{
 KillVideo();
 _fullscreen=!_fullscreen;

 if(!InitVideo())
 {
  _fullscreen=!_fullscreen;
  if(!InitVideo())
  {
   puts("Gah, bailing out.");
   exit(1);
  }
 }
}

static SDL_Color psdl[256];
void FCEUD_SetPalette(uint8 index, uint8 r, uint8 g, uint8 b)
{
 psdl[index].r=r;
 psdl[index].g=g;
 psdl[index].b=b;

 paletterefresh=1;
}

void FCEUD_GetPalette(uint8 index, uint8 *r, uint8 *g, uint8 *b)
{
 *r=psdl[index].r;
 *g=psdl[index].g;
 *b=psdl[index].b;
}

static void RedoPalette(void)
{
 if(curbpp>8)
  SetPaletteBlitToHigh(psdl);
 else
  SDL_SetPalette(screen,SDL_PHYSPAL,psdl,0,256);
 #ifdef OPENGL
 if(_opengl)
  SetOpenGLPalette(psdl);
 #endif
}

void LockConsole(){}
void UnlockConsole(){}
void BlitScreen(uint8 *XBuf)
{
 uint8 *dest;
 int xo=0,yo=0;

 if(paletterefresh)
 {
  RedoPalette();
  paletterefresh=0;
 }

 #ifdef OPENGL
 if(_opengl)
 {
  BlitOpenGL(XBuf);
  return;
 }
 #endif

 XBuf+=_sline*272;

 if(SDL_MUSTLOCK(screen))
  SDL_LockSurface(screen);

 dest=screen->pixels;

 if(_fullscreen)
 {
  xo=(((screen->w-NWIDTH*exs))/2);
  dest+=xo*(curbpp>>3);
  if(screen->h>(tlines*eys))
  {
   yo=((screen->h-tlines*eys)/2);
   dest+=yo*screen->pitch;
  }
 }
 if(curbpp>8)
  Blit8ToHigh(XBuf+NOFFSET,dest, NWIDTH, tlines, screen->pitch,exs,eys);
 else
  Blit8To8(XBuf+NOFFSET,dest, NWIDTH, tlines, screen->pitch,exs,eys,eefx);
 if(SDL_MUSTLOCK(screen))
  SDL_UnlockSurface(screen);

 SDL_UpdateRect(screen, xo, yo, NWIDTH*exs, tlines*eys);
}

uint32 PtoV(uint16 x, uint16 y)
{
 if(_fullscreen)
 {

 }
 else
 {
  if(eoptions&EO_CLIPSIDES)
   x+=8;
  y+=srendline;
 }
 return(x|(y<<16));
}
