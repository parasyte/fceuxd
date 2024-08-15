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

#include "../../types.h"

static uint32 CBM[3];
static uint32 *palettetranslate=0;

static int Bpp;	// BYTES per pixel
static int highefx;

#define BLUR_RED	20
#define BLUR_GREEN	20
#define BLUR_BLUE	10

#define FVB_SCANLINES	1

/* The blur effect is only available for bpp>=16.  It could be easily modified
   to look like what happens on the real NES and TV, but lack of decent
   synchronization to the vertical retrace period makes it look rather
   blah.
*/
#define FVB_BLUR	2

int InitBlitToHigh(int b, uint32 rmask, uint32 gmask, uint32 bmask, int efx)
{
 Bpp=b;

 highefx=efx;

 if(Bpp<=1 || Bpp>4)
  return(0);

 if(efx&FVB_BLUR)
 {
  if(Bpp==2)
   palettetranslate=malloc(65536*4);
  else if(Bpp>=3)
   palettetranslate=malloc(65536*4);
 }
 else
 {
  if(Bpp==2)
   palettetranslate=malloc(65536*4);
  else if(Bpp>=3)
   palettetranslate=malloc(256*4);
 }

 if(!palettetranslate)
  return(0);


 CBM[0]=rmask;
 CBM[1]=gmask;
 CBM[2]=bmask;
 return(1);
}

void KillBlitToHigh(void)
{
  free(palettetranslate);
}

void SetPaletteBlitToHigh(uint8 *src)
{ 
             int cshiftr[3];
             int cshiftl[3];
             int a,x,z,y;

             cshiftl[0]=cshiftl[1]=cshiftl[2]=-1;
             for(a=0;a<3;a++)
             {
              for(x=0,y=-1,z=0;x<32;x++)
              {
               if(CBM[a]&(1<<x))
               {
                if(cshiftl[a]==-1) cshiftl[a]=x;
                z++;
               }
              }
              cshiftr[a]=(8-z);
             }

 switch(Bpp)
 {
    case 2:
	     if(highefx&FVB_BLUR)
	     {
              for(x=0;x<256;x++)   
              {
               uint32 r,g,b;
	       for(y=0;y<256;y++)
               {
                r=src[x<<2]*(100-BLUR_RED);
                g=src[(x<<2)+1]*(100-BLUR_GREEN);
                b=src[(x<<2)+2]*(100-BLUR_BLUE);
  
                r+=src[y<<2]*BLUR_RED;
                g+=src[(y<<2)+1]*BLUR_GREEN;
                b+=src[(y<<2)+2]*BLUR_BLUE;
                r/=100;
		g/=100; 
		b/=100;

		if(r>255) r=255; if(g>255) g=255; if(b>255) b=255;
                palettetranslate[x|(y<<8)]=((r>>cshiftr[0])<<cshiftl[0])|
					   ((g>>cshiftr[1])<<cshiftl[1])|
					   ((b>>cshiftr[2])<<cshiftl[2]);
	       }
              }
	     }
	     else
             for(x=0;x<65536;x++)
             {
              uint16 lower,upper;

              lower=(src[((x&255)<<2)]>>cshiftr[0])<<cshiftl[0];
              lower|=(src[((x&255)<<2)+1]>>cshiftr[1])<<cshiftl[1];
              lower|=(src[((x&255)<<2)+2]>>cshiftr[2])<<cshiftl[2];
              upper=(src[((x>>8)<<2)]>>cshiftr[0])<<cshiftl[0];
              upper|=(src[((x>>8)<<2)+1]>>cshiftr[1])<<cshiftl[1];
              upper|=(src[((x>>8)<<2)+2]>>cshiftr[2])<<cshiftl[2];

              palettetranslate[x]=lower|(upper<<16);
             }
	    break;
    case 4:
            for(x=0;x<256;x++)
            {
	     uint32 r,g,b;

	     if(!(highefx&FVB_BLUR))
	     {
	      r=src[x<<2];
              g=src[(x<<2)+1];
              b=src[(x<<2)+2];
              palettetranslate[x]=(r<<cshiftl[0])|(g<<cshiftl[1])|(b<<cshiftl[2]);
	     }
	     else	
	     for(y=0;y<256;y++)
	     {
                r=src[x<<2]*(100-BLUR_RED);
                g=src[(x<<2)+1]*(100-BLUR_GREEN);
                b=src[(x<<2)+2]*(100-BLUR_BLUE);

                r+=src[y<<2]*BLUR_RED;
                g+=src[(y<<2)+1]*BLUR_GREEN;
                b+=src[(y<<2)+2]*BLUR_BLUE;
               
                r/=100;
                g/=100;
                b/=100;                  
                if(r>255) r=255; if(g>255) g=255; if(b>255) b=255;

                palettetranslate[x|(y<<8)]=(r<<cshiftl[0])|(g<<cshiftl[1])|(b<<cshiftl[2]);
	     }
            }
            break;
 }
}
void Blit8To8(uint8 *src, uint8 *dest, int xr, int yr, int pitch, int xscale, int yscale, int efx)
{
 int x,y;
 int pinc;
 
 pinc=pitch-(xr*xscale);
 if(xscale!=1 || yscale!=1)
 {
  if(efx&FVB_SCANLINES)
  {
   for(y=yr;y;y--,src+=272-xr)
   {
    int doo=yscale-(yscale>>1);
    do
    {
     for(x=xr;x;x--,src++)
     {
      int too=xscale;
      do
      {
       *(uint8 *)dest=*(uint8 *)src;
       dest++;
      } while(--too);
     }
     src-=xr;
     dest+=pinc;
    } while(--doo);
    //src-=xr*(yscale-(yscale>>1));
    dest+=pitch*(yscale>>1);

    src+=xr;
   }

  }
  else
  {
   for(y=yr;y;y--,src+=272-xr)
   {
    int doo=yscale;
    do
    {
     for(x=xr;x;x--,src++)
     {
      int too=xscale;
      do
      {
       *(uint8 *)dest=*(uint8 *)src;
       dest++;
      } while(--too);
     }
     src-=xr;
     dest+=pinc;
    } while(--doo);
    src+=xr;
   }
 }
 
 }
 else
 {
  for(y=yr;y;y--,dest+=pinc,src+=272-xr)
   for(x=xr;x;x-=4,dest+=4,src+=4)
    *(uint32 *)dest=*(uint32 *)src;
 }
}

void Blit8ToHigh(uint8 *src, uint8 *dest, int xr, int yr, int pitch, 
		 int xscale, int yscale)
{
 int x,y;
 int pinc;

 if(highefx&FVB_BLUR)	// DONE
 {
  /* Clean left side to prevent weird artifacts. */
  for(y=0;y<yr;y++)
   src[y*272-1]=src[y*272];

  if(xscale!=1 || yscale!=1 || (highefx&FVB_SCANLINES)) // DONE
  {
   switch(Bpp)
   {
    case 4:
    pinc=pitch-((xr*xscale)<<2);
    for(y=yr;y;y--,src+=272-xr)
    {
     int doo=yscale;   
    
     if(highefx& FVB_SCANLINES)
      doo-=yscale>>1;
     do
     {
      for(x=xr;x;x--,src++)
      {
       int too=xscale;
       do
       {
        *(uint32 *)dest=palettetranslate[*src|((*(src-1))<<8)];
        dest+=4;
       } while(--too);
      }
      src-=xr;
      dest+=pinc;
     } while(--doo);
     src+=xr;
     if(highefx&FVB_SCANLINES)
      dest+=pitch*(yscale>>1);
    }
    break;
  case 2:
    pinc=pitch-((xr*xscale)<<1);

    for(y=yr;y;y--,src+=272-xr)
    {
     int doo=yscale;

     if(highefx& FVB_SCANLINES)
      doo-=yscale>>1;     
     do
     {
      for(x=xr;x;x--,src++)
      {
       int too=xscale;
       do
       {
        *(uint16 *)dest=palettetranslate[*src|((*(src-1))<<8)];
        dest+=2;
       } while(--too);
      }
     src-=xr;
     dest+=pinc;
     } while(--doo);
     src+=xr;
     if(highefx&FVB_SCANLINES) 
      dest+=pitch*(yscale>>1);
    }
    break;   
   }   
  }
  else // No scaling, no scanlines, just blurring. - DONE
  switch(Bpp)
  {
   case 4:   
    pinc=pitch-(xr<<2);
    for(y=yr;y;y--,src+=272-xr)
    {
     for(x=xr;x;x--)
     {
      *(uint32 *)dest=palettetranslate[*src|((*(src-1))<<8)];
      dest+=4;
      src++; 
     }
     dest+=pinc;
    }
    break;
  case 2:
   pinc=pitch-(xr<<1);
   for(y=yr;y;y--,src+=272-xr)
   {
    for(x=xr;x;x--)
    {
     *(uint16 *)dest=palettetranslate[*src|((*(src-1))<<8)];
     dest+=2;
     src++;
    }
    dest+=pinc;
   }
   break;
  }
 }
 else	// No blur effects.
 {
  if(xscale!=1 || yscale!=1 || (highefx&FVB_SCANLINES))
  {
   switch(Bpp)
   {
    case 4:
    pinc=pitch-((xr*xscale)<<2);
    for(y=yr;y;y--,src+=272-xr)
    {
     int doo=yscale;
             
     if(highefx& FVB_SCANLINES)
      doo-=yscale>>1;
     do
     {
      for(x=xr;x;x--,src++)
      {
       int too=xscale;
       do
       {
        *(uint32 *)dest=palettetranslate[*src]; 
        dest+=4;
       } while(--too);
      }
      src-=xr;
      dest+=pinc;
     } while(--doo);
     src+=xr;
     if(highefx&FVB_SCANLINES)
      dest+=pitch*(yscale>>1);
    }  
    break;
  case 2:
    pinc=pitch-((xr*xscale)<<1);
       
    for(y=yr;y;y--,src+=272-xr)
    {   
     int doo=yscale;
        
     if(highefx& FVB_SCANLINES)
      doo-=yscale>>1;
     do
     {
      for(x=xr;x;x--,src++)
      {
       int too=xscale;
       do
       {
        *(uint16 *)dest=palettetranslate[*src];
        dest+=2;
       } while(--too);
      }
     src-=xr;
     dest+=pinc;
     } while(--doo);
     src+=xr;
     if(highefx&FVB_SCANLINES)
      dest+=pitch*(yscale>>1);
    }  
    break;
   }
  }
  else
  switch(Bpp)
  {
   case 4:
    pinc=pitch-(xr<<2);
    for(y=yr;y;y--,src+=272-xr)
    {
     for(x=xr;x;x--)
     {
      *(uint32 *)dest=palettetranslate[(uint32)*src];
      dest+=4;
      src++;
     }
     dest+=pinc;
    }
    break;
  case 2:
   pinc=pitch-(xr<<1);
   for(y=yr;y;y--,src+=272-xr)
   {
    for(x=xr>>1;x;x--)
    {
     *(uint32 *)dest=palettetranslate[*(uint16 *)src];
     dest+=4;
     src+=2;
    }
    dest+=pinc;
   }
   break;
  }
 }

}
