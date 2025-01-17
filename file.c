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

#ifdef ZLIB
 #include <zlib.h>
 #include "zlib/unzip.h"
#endif

#include "types.h"
#include "file.h"
#include "endian.h"
#include "memory.h"
#include "driver.h"
#include "general.h"
#include "svga.h"

typedef struct {
           uint8 *data;
           uint32 size;
           uint32 location;
} MEMWRAP;

void ApplyIPS(FILE *ips, MEMWRAP *dest)
{
 uint8 header[5];
 uint32 count=0;

 FCEU_printf(" Applying IPS...\n");
 if(fread(header,1,5,ips)!=5)
 {
  fclose(ips);
  return;
 }
 if(memcmp(header,"PATCH",5))
 {
  fclose(ips);
  return;
 }

 while(fread(header,1,3,ips)==3)
 {
  uint32 offset=(header[0]<<16)|(header[1]<<8)|header[2];
  uint16 size;

  if(!memcmp(header,"EOF",3))
  {
   FCEU_printf(" IPS EOF:  Did %d patches\n\n",count);
   fclose(ips);
   return;
  }

  size=fgetc(ips)<<8;
  size|=fgetc(ips);
  if(!size)	/* RLE */
  {
   uint8 *start;
   uint8 b;
   size=fgetc(ips)<<8;
   size|=fgetc(ips);

   //FCEU_printf("  Offset: %8d  Size: %5d RLE\n",offset,size);

   if((offset+size)>dest->size)
   {
    void *tmp;

    // Probably a little slow.
    tmp=realloc(dest->data,offset+size);
    if(!tmp)
    {
     FCEU_printf("  Oops.  IPS patch %d(type RLE) goes beyond end of file.  Could not allocate memory.\n",count);
     fclose(ips);
     return;
    }
    dest->size=offset+size;
    dest->data=tmp;
    memset(dest->data+dest->size,0,offset+size-dest->size);
   }
   b=fgetc(ips);
   start=dest->data+offset;
   do
   {
    *start=b;
    start++;
   } while(--size);
  }
  else		/* Normal patch */
  {
   //FCEU_printf("  Offset: %8d  Size: %5d\n",offset,size);
   if((offset+size)>dest->size)
   {
    void *tmp;

    // Probably a little slow.
    tmp=realloc(dest->data,offset+size);
    if(!tmp)
    {
     FCEU_printf("  Oops.  IPS patch %d(type normal) goes beyond end of file.  Could not allocate memory.\n",count);
     fclose(ips);
     return;
    }
    dest->data=tmp;
    memset(dest->data+dest->size,0,offset+size-dest->size);
   }
   fread(dest->data+offset,1,size,ips);
  }
  count++;
 }
 fclose(ips);
 FCEU_printf(" Hard IPS end!\n");
}

static MEMWRAP *MakeMemWrap(void *tz, int type)
{
 MEMWRAP *tmp;

 if(!(tmp=FCEU_malloc(sizeof(MEMWRAP))))
  goto doret;
 tmp->location=0;

 if(type==0)
 {
  fseek(tz,0,SEEK_END);
  tmp->size=ftell(tz);
  fseek(tz,0,SEEK_SET);
  if(!(tmp->data=FCEU_malloc(tmp->size)))
  {
   free(tmp);
   tmp=0;
   goto doret;
  }
  fread(tmp->data,1,tmp->size,tz);
 }
 #ifdef ZLIB
 else if(type==1)
 {
  /* Bleck.  The gzip file format has the size of the uncompressed data,
     but I can't get to the info with the zlib interface(?). */
  for(tmp->size=0; gzgetc(tz) != EOF; tmp->size++);
  gzseek(tz,0,SEEK_SET);
  if(!(tmp->data=FCEU_malloc(tmp->size)))
  {
   free(tmp);
   tmp=0;
   goto doret;
  }
  gzread(tz,tmp->data,tmp->size);
 }
 else if(type==2)
 {
  unz_file_info ufo;
  unzGetCurrentFileInfo(tz,&ufo,0,0,0,0,0,0);

  tmp->size=ufo.uncompressed_size;
  if(!(tmp->data=FCEU_malloc(ufo.uncompressed_size)))
  {
   free(tmp);
   tmp=0;
   goto doret;
  }
  unzReadCurrentFile(tz,tmp->data,ufo.uncompressed_size);
 }
 #endif
 doret:
 if(type==0)
 {
  fclose(tz);
 }
 #ifdef ZLIB
 else if(type==1)
 {
  gzclose(tz);
 }
 else if(type==2)
 {
  unzCloseCurrentFile(tz);
  unzClose(tz);
 }
 #endif
 return tmp;
}

#ifndef __GNUC__
 #define strcasecmp strcmp
#endif


extern int paldetect;


FCEUFILE * FCEU_fopen(char *path, char *mode, char *ext)
{
 FILE *ipsfile=0;
 FCEUFILE *fceufp;
 void *t;

 char tempu[512];	// Longer filenames might be possible, but I don't
		 	// think people would name files that long in zip files...

 GetFileBase(path);

 if(strchr(mode,'r'))
  ipsfile=fopen(FCEU_MakeFName(FCEUMKF_IPS,0,0),"rb");
 fceufp=malloc(sizeof(FCEUFILE));

 #ifdef ZLIB
 {
  unzFile tz;
  if((tz=unzOpen(path)))  // If it's not a zip file, use regular file handlers.
			  // Assuming file type by extension usually works,
			  // but I don't like it. :)
  {
   if(unzGoToFirstFile(tz)==UNZ_OK)
   {
    for(;;)
    {
     unzGetCurrentFileInfo(tz,0,tempu,512,0,0,0,0);
     tempu[511]=0;
     if(strlen(tempu)>=4)
     {
      char *za=tempu+strlen(tempu)-4;

      if(!ext)
      {
       if(!strcasecmp(za,".nes") || !strcasecmp(za,".fds") ||
          !strcasecmp(za,".nsf") || !strcasecmp(za,".unf") ||
          !strcasecmp(za,".nez"))
        break;
      }
      else if(!strcasecmp(za,ext))
       break;
     }
     if(strlen(tempu)>=5)
     {
      if(!strcasecmp(tempu+strlen(tempu)-5,".unif"))
       break;
     }
     if(unzGoToNextFile(tz)!=UNZ_OK)
     {
      if(unzGoToFirstFile(tz)!=UNZ_OK) goto zpfail;
      break;
     }
    }
    if(unzOpenCurrentFile(tz)!=UNZ_OK)
     goto zpfail;
   }
   else
   {
    zpfail:
    free(fceufp);
    unzClose(tz);
    return 0;
   }
   if(!(fceufp->fp=MakeMemWrap(tz,2)))
   {
    free(fceufp);
    return(0);
   }
   fceufp->type=2;

   if (paldetect) { //para edit
     if (strstr(tempu,"(E)")) FSettings.PAL=1;
     else FSettings.PAL=0;
   }

   if(ipsfile)
    ApplyIPS(ipsfile,fceufp->fp);
   return(fceufp);
  }
 }
#endif

 #ifdef ZLIB
 if((t=fopen(path,"rb")))
 {
  uint32 magic;

  magic=fgetc(t);
  magic|=fgetc(t)<<8;
  magic|=fgetc(t)<<16;

  fclose(t);

  if(magic==0x088b1f)
  {
   if((t=gzopen(path,mode)))
   {
    fceufp->type=1;
    fceufp->fp=t;

    if (paldetect) { //para edit
      if (strstr(path,"(E)")) FSettings.PAL=1;
      else FSettings.PAL=0;
    }

    if(ipsfile)
    {
     if(!(fceufp->fp=MakeMemWrap(t,1)))
     {
      free(fceufp);
      return(0);
     }
     fceufp->type=3;
     ApplyIPS(ipsfile,fceufp->fp);
    }
    return(fceufp);
   }
  }
 }
 #endif

  if((t=fopen(path,mode)))
  {
   fseek(t,0,SEEK_SET);
   fceufp->type=0;
   fceufp->fp=t;

   if (paldetect) { //para edit
     if (strstr(path,"(E)")) FSettings.PAL=1;
     else FSettings.PAL=0;
   }

   if(ipsfile)
   {
    if(!(fceufp->fp=MakeMemWrap(t,0)))
    {
     free(fceufp);
     return(0);
    }
    fceufp->type=3;
    ApplyIPS(ipsfile,fceufp->fp);
   }
   return(fceufp);
  }

 free(fceufp);
 return 0;
}

int FCEU_fclose(FCEUFILE *fp)
{
 #ifdef ZLIB
 if(fp->type==1)
 {
  gzclose(fp->fp);
 }
 else if(fp->type>=2)
 {
  free(((MEMWRAP*)(fp->fp))->data);
  free(fp->fp);
 }
 else
 {
 #endif
  fclose(fp->fp);
 #ifdef ZLIB
 }
 #endif
 free(fp);
 return 1;
}

uint64 FCEU_fread(void *ptr, size_t size, size_t nmemb, FCEUFILE *fp)
{
 #ifdef ZLIB
 if(fp->type==1)
 {
  return gzread(fp->fp,ptr,size*nmemb);
 }
 else if(fp->type>=2)
 {
  MEMWRAP *wz;
  uint32 total=size*nmemb;

  wz=(MEMWRAP*)fp->fp;
  if(wz->location>=wz->size) return 0;

  if((wz->location+total)>wz->size)
  {
   int ak=wz->size-wz->location;
   memcpy((uint8*)ptr,wz->data+wz->location,ak);
   wz->location=wz->size;
   return(ak/size);
  }
  else
  {
   memcpy((uint8*)ptr,wz->data+wz->location,total);
   wz->location+=total;
   return nmemb;
  }
 }
 else
 {
 #endif
 return fread(ptr,size,nmemb,fp->fp);
 #ifdef ZLIB
 }
 #endif
}

uint64 FCEU_fwrite(void *ptr, size_t size, size_t nmemb, FCEUFILE *fp)
{
 #ifdef ZLIB
 if(fp->type==1)
 {
  return gzwrite(fp->fp,ptr,size*nmemb);
 }
 else if(fp->type>=2)
 {
  return 0;
 }
 else
 #endif
  return fwrite(ptr,size,nmemb,fp->fp);
}

int FCEU_fseek(FCEUFILE *fp, long offset, int whence)
{
 #ifdef ZLIB
 if(fp->type==1)
 {
  return( (gzseek(fp->fp,offset,whence)>0)?0:-1);
 }
 else if(fp->type>=2)
 {
  MEMWRAP *wz;
  wz=(MEMWRAP*)fp->fp;

  switch(whence)
  {
   case SEEK_SET:if(offset>=wz->size)
                  return(-1);
                 wz->location=offset;break;
   case SEEK_CUR:if(offset+wz->location>wz->size)
                  return (-1);
                 wz->location+=offset;
                 break;
  }
  return 0;
 }
 else
 #endif
  return fseek(fp->fp,offset,whence);
}

uint64 FCEU_ftell(FCEUFILE *fp)
{
 #ifdef ZLIB
 if(fp->type==1)
 {
  return gztell(fp->fp);
 }
 else if(fp->type>=2)
 {
  return (((MEMWRAP *)(fp->fp))->location);
 }
 else
 #endif
  return ftell(fp->fp);
}

void FCEU_rewind(FCEUFILE *fp)
{
 #ifdef ZLIB
 if(fp->type==1)
 {
  gzrewind(fp->fp);
 }
 else if(fp->type>=2)
 {
  ((MEMWRAP *)(fp->fp))->location=0;
 }
 else
 #endif
 #ifdef _WIN32_WCE
  fseek(fp->fp,0,SEEK_SET);
 #else
  rewind(fp->fp);
 #endif
}

int FCEU_read16(uint16 *val, FCEUFILE *fp)
{
 uint8 t[2];
 #ifdef ZLIB
 if(fp->type>=1)
 {
  if(fp->type>=2)
  {
   MEMWRAP *wz;
   wz=fp->fp;
   if(wz->location+2>wz->size)
    {return 0;}
   *(uint32 *)t=*(uint32 *)(wz->data+wz->location);
   wz->location+=2;
  }
  else if(fp->type==1)
   if(gzread(fp->fp,&t,2)!=2) return(0);
  return(1);
 }
 else
 #endif
 {
  if(fread(t,1,2,fp->fp)!=2) return(0);
 }
 *val=t[0]|(t[1]<<8);
 return(1);
}

int FCEU_read32(void *Bufo, FCEUFILE *fp)
{
 #ifdef ZLIB
 if(fp->type>=1)
 {
  uint8 t[4];
  #ifndef LSB_FIRST
  uint8 x[4];
  #endif
  if(fp->type>=2)
  {
   MEMWRAP *wz;
   wz=fp->fp;
   if(wz->location+4>wz->size)
    {return 0;}
   *(uint32 *)t=*(uint32 *)(wz->data+wz->location);
   wz->location+=4;
  }
  else if(fp->type==1)
   gzread(fp->fp,&t,4);
  #ifndef LSB_FIRST
  x[0]=t[3];
  x[1]=t[2];
  x[2]=t[1];
  x[3]=t[0];
  *(uint32*)Bufo=*(uint32*)x;
  #else
  *(uint32*)Bufo=*(uint32*)t;
  #endif
  return 1;
 }
 else
 #endif
 {
  return read32(Bufo,fp->fp);
 }
}

int FCEU_fgetc(FCEUFILE *fp)
{
 #ifdef ZLIB
 if(fp->type==1)
  return gzgetc(fp->fp);
 else if(fp->type>=2)
 {
  MEMWRAP *wz;
  wz=fp->fp;
  if(wz->location<wz->size)
   return wz->data[wz->location++];
  return EOF;
 }
 else
#endif
  return fgetc(fp->fp);
}

uint64 FCEU_fgetsize(FCEUFILE *fp)
{
 #ifdef ZLIB
 if(fp->type==1)
 {
  int x,t;
  t=gztell(fp->fp);
  gzrewind(fp->fp);
  for(x=0; gzgetc(fp->fp) != EOF; x++);
  gzseek(fp->fp,t,SEEK_SET);
  return(x);
 }
 else if(fp->type>=2)
  return ((MEMWRAP*)(fp->fp))->size;
 else
 #endif
 {
  long t,r;
  t=ftell(fp->fp);
  fseek(fp->fp,0,SEEK_END);
  r=ftell(fp->fp);
  fseek(fp->fp,t,SEEK_SET);
  return r;
 }
}

int FCEU_fisarchive(FCEUFILE *fp)
{
 #ifdef ZLIB
 if(fp->type==2)
  return 1;
 #endif
 return 0;
}
