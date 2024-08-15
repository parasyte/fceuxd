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

/********************************************************/
/*******		sound.c				*/
/*******						*/
/*******  Sound emulation code and waveform synthesis 	*/
/*******  routines.  A few ideas were inspired		*/
/*******  by code from Marat Fayzullin's EMUlib		*/
/*******						*/
/********************************************************/		

#include <stdlib.h>
#include <stdio.h>

#include <string.h>

#include "types.h"
#include "x6502.h"

#include "fce.h"
#include "svga.h"
#include "sound.h"
#include "filter.h"
#include "state.h"
#include "wave.h"
#include "cdlogger.h"

int32 WaveHi[40000];

static uint32 wlookup1[32];
static uint32 wlookup2[192];

uint32 Wave[2048+512];

int32 WaveFinal[2048+512];

EXPSOUND GameExpSound={0,0,0};

uint8 trimode=0;
uint8 tricoop=0;

static int32 tricount;
static int32 tristep;
static int32 ncount;

uint8 PSG[0x18];

uint8 decvolume[3];
uint8 realvolume[3];

static int tal[4]={2,4,8,12};
static int32 rcount[2];
static int32 rdc[2];

/* Stuff exclusively for low-quality sound. */
uint32 soundtsinc;
uint32 soundtsi;
int32 lpcmer;
static int32 sqacc[2];
static uint32 LQNFTable[0x10];
static int32 PCMaccLQ;
/* LQ stuff ends. */

uint8 sqnon=0;

uint16 nreg;
 
int32 lengthcount[4]; 

static const uint8 Slengthtable[0x20]=
{
 0x5,0x7f,0xA,0x1,0x14,0x2,0x28,0x3,0x50,0x4,0x1E,0x5,0x7,0x6,0x0E,0x7,
 0x6,0x08,0xC,0x9,0x18,0xa,0x30,0xb,0x60,0xc,0x24,0xd,0x8,0xe,0x10,0xf
};

static uint32 lengthtable[0x20];

static const uint32 SNoiseFreqTable[0x10]=
{
 2,4,8,0x10,0x20,0x30,0x40,0x50,0x65,0x7f,0xbe,0xfe,0x17d,0x1fc,0x3f9,0x7f2
};

int32 nesincsize;

// 6 d e
static const uint32 NTSCPCMTable[0x10]=
{
 428,380,340,320,286,254,226,214,
 190,160,142,128,106, 85 ,72,54
// 0xd6,0xbe,0xaa,0xa0,0x8f,0x7f,0x71,0x6b,
// 0x5f,0x50,0x47,0x40,0x35,0x2a,0x24,0x1b
};

/* This table is constructed from the NTSCPCMTable in the init code far below.*/
static uint32 PALPCMTable[0x10]=	// These values are just guessed.
{
// 0xc6,0xb0,0x9d,0x94,0x84,0x75,0x68,0x63,
// 0x58,0x4a,0x41,0x3b,0x31,0x27,0x21,0x19
};

uint32 PSG_base;

// $4010        -        Frequency
// $4011        -        Actual data outputted
// $4012        -        Address register: $c000 + V*64
// $4013        -        Size register:  Size in bytes = (V+1)*64

//static int bitmoo;
//static uint8 bitfoo;
static int32 PCMIRQCount;
static int32 PCMacc=0;
int32 PCMfreq;
uint8 PCMBitIndex=0;
uint32 PCMAddressIndex=0;
int32 PCMSizeIndex=0;
uint8 PCMBuffer=0; 

static void Dummyfunc(void) {};
static void (*DoNoise)(void)=Dummyfunc;
static void (*DoTriangle)(void)=Dummyfunc;
void (*DoPCM)(void)=Dummyfunc;
static void (*DoSQ1)(void)=Dummyfunc;
static void (*DoSQ2)(void)=Dummyfunc;
uint8 SIRQStat=0;

static void PrepDPCM()
{
 PCMAddressIndex=0x4000+(PSG[0x12]<<6);
 PCMSizeIndex=(PSG[0x13]<<4)+1;
 PCMBitIndex=0;

 PCMBuffer=X6502_DMR(0x8000+PCMAddressIndex);
 if(loggingcodedata)LogPCM(0x8000+PCMAddressIndex);//bbit edited: this was added for the CDLogger
 if(PAL)
  PCMfreq=PALPCMTable[PSG[0x10]&0xF];
 else
  PCMfreq=NTSCPCMTable[PSG[0x10]&0xF];

 //PCMfreq*=2;
 PCMacc=PCMfreq;
 PCMaccLQ=(int32)PCMfreq<<17;

 /* This isn't pretty... and most likely totally wrong*/
 if(PAL)
 PCMIRQCount=((PALPCMTable[PSG[0x10]&0xF]<<3)-((((PSG[0x10]&0xF)^0xF)+1)*16))*
                ((PSG[0x13]<<4)+1);
 else
 PCMIRQCount=((NTSCPCMTable[PSG[0x10]&0xF]<<3)-((((PSG[0x10]&0xF)^0xF)+1)*16))*
 		((PSG[0x13]<<4)+1);
 //PCMIRQCount=((NTSCPCMTable[PSG[0x10]&0xF]<<3)-76)*((PSG[0x13]<<7)+7);
 //PCMIRQCount>>=3;
 lpcmer=timestamp;
}

uint8 sweepon[2]={0,0};
int32 curfreq[2]={0,0};


uint8 SweepCount[2];
uint8 DecCountTo1[3];

uint8 fcnt=0;
int32 fhcnt=0;
int32 fhinc;

/* Instantaneous?  Maybe the new freq value is being calculated all of the time... */
static int CheckFreq(uint32 cf, uint8 sr)
{
 uint32 mod;
 if(!(sr&0x8))
 {
  mod=cf>>(sr&7);
  if((mod+cf)&0x800)
   return(0);
 }
 return(1);
}

static DECLFW(Write0x11)
{
 DoPCM();
 PSG[0x11]=V&0x7F;
}

static void SQReload(int x, uint8 V)
{
           if(PSG[0x15]&(1<<x))
           {           
            if(x)
             DoSQ2();
            else
             DoSQ1();
            lengthcount[x]=lengthtable[(V>>3)&0x1f];
            sqnon|=1<<x;
	   }
           sweepon[x]=PSG[(x<<2)|1]&0x80;
           curfreq[x]=PSG[(x<<2)|0x2]|((V&7)<<8);
           decvolume[x]=0xF;
           DecCountTo1[x]=(PSG[(x<<2)]&0xF)+1;
           SweepCount[x]=((PSG[(x<<2)|0x1]>>4)&7)+1;           
           rdc[x]=0;
}

//#define DECLFW(x) void FP_FASTAPASS(2) x (uint32 A, uint8 V)
//static DECLFW(Write_PSG)
static void FP_FASTAPASS(2) Write_PSG(uint32 A, uint8 BOOP)
{
uint32 V = BOOP;
 //if((A>=0x4008 && A<=0x400b) || A==0x4015)
 // FCEU_printf("$%04x:$%02x, %lld, %04x\n",A,V,timestamp+timestampbase,X.PC);
 A&=0x1f;
 switch(A)
 {
  case 0x0:DoSQ1();
           if(V&0x10)
            realvolume[0]=V&0xF;
           break;
  case 0x1:
           sweepon[0]=V&0x80;
           break;
  case 0x2:
           DoSQ1();
           curfreq[0]&=0xFF00;
           curfreq[0]|=V;
           break;
  case 0x3:
           SQReload(0,V);
           break;

  case 0x4:           
	   DoSQ2();
           if(V&0x10)
            realvolume[1]=V&0xF;
	   break;
  case 0x5:        
          sweepon[1]=V&0x80;
          break;
  case 0x6:DoSQ2();
          curfreq[1]&=0xFF00;
          curfreq[1]|=V;
          break;
  case 0x7:          
          SQReload(1,V);
          break;
  case 0x8:                                 
          DoTriangle();
	  if(trimode) tricoop=V&0x7F;
          break;
  case 0xa:DoTriangle();
	   break;
  case 0xb:
	  if(PSG[0x15]&0x4)
	  {
	   DoTriangle();
           sqnon|=4;
           lengthcount[2]=lengthtable[(V>>3)&0x1f];
	  }
          trimode=1; 
          if(PSG[0x8]&0x80)
           tricoop=PSG[0x8]&0x7F;
          break;
  case 0xC:DoNoise();
           if(V&0x10)
            realvolume[2]=V&0xF;
           break;
  case 0xE:DoNoise();
           /* I don't know if this is right. */
	   #ifdef moo
           if((PSG[0xE]^V)&0x80)
	   {
	   if(V&0x80)
	    nreg=~0;
	   else
            nreg=1;}
	   #endif
           break;
  case 0xF:
           if(PSG[0x15]&8)
           {
	    DoNoise();
            sqnon|=8;
	    lengthcount[3]=lengthtable[(V>>3)&0x1f];
	   }
           decvolume[2]=0xF;
	   DecCountTo1[2]=(PSG[0xC]&0xF)+1;          
           break;
 case 0x10:
	//    printf("$%04x:$%02x, $%04x, $%02x\n",A,V,X.PC,X.X);
	//   FCEUI_DumpMem("dmp",0x8000,0xffff);
           DoPCM();
	   if(!(V&0x80))
	    X6502_IRQEnd(FCEU_IQDPCM);
	   break;
// case 0x13:PCMSizeIndex=(PSG[0x13]<<4)+1;break;
 case 0x15:

           {
	    int t=V^PSG[0x15];

            if(t&1)
             DoSQ1();
            if(t&2)
             DoSQ2();
            if(t&4)
             DoTriangle();
            if(t&8)
             DoNoise();
            if(t&0x10)
             DoPCM();
            sqnon&=V;
            if(V&0x10)
            {
             if(!(PSG[0x15]&0x10))
              PrepDPCM();
            }
	    else
	     PCMIRQCount=0;
            X6502_IRQEnd(FCEU_IQDPCM);
	   }
           break;
 case 0x17: 
	   V&=0xC0;
           fcnt=0;      
           if(V&0x80)
            FrameSoundUpdate();
           fhcnt=fhinc;
           X6502_IRQEnd(FCEU_IQFCOUNT);
	   SIRQStat&=~0x40;	   
           break;
 }
 PSG[A]=V;
}

DECLFR(Read_PSG)
{
   uint8 ret;

   if(!fceuindbg)
    if(PSG[0x15]&0x10)
     DoPCM();
   ret=(PSG[0x15]&(sqnon|0x10))|SIRQStat;

   if(!fceuindbg)
   {
    SIRQStat&=~0x40;
    X6502_IRQEnd(/*FCEU_IQDPCM|*/FCEU_IQFCOUNT);
   }
   return ret;
}

DECLFR(Read_PSGDummy)
{
   uint8 ret;

   ret=(PSG[0x15]&sqnon)|SIRQStat;
   if(!fceuindbg)
   {
    SIRQStat&=~0x40;
    X6502_IRQEnd(/*FCEU_IQDPCM|*/FCEU_IQFCOUNT);
   }
   return ret;
}

static void FrameSoundStuff(int V)
{
 int P;

 DoSQ1();
 DoSQ2();
 DoNoise();

 switch((V&1))
 {
  case 1:       /* Envelope decay, linear counter, length counter, freq sweep */
        if(PSG[0x15]&4 && sqnon&4)
         if(!(PSG[8]&0x80))
         {
          if(lengthcount[2]>0)
          {
            lengthcount[2]--;
            if(lengthcount[2]<=0)
             {
              DoTriangle();
              sqnon&=~4;
             }
           }        
         }

        for(P=0;P<2;P++)
        {
         if(PSG[0x15]&(P+1) && sqnon&(P+1))
 	 {
          if(!(PSG[P<<2]&0x20))
          {
           if(lengthcount[P]>0)
           {
            lengthcount[P]--;            
            if(lengthcount[P]<=0)
             {
              sqnon&=~(P+1);
             }
           }
          }
	 }
		/* Frequency Sweep Code Here */
		/* xxxx 0000 */
		/* xxxx = hz.  120/(x+1)*/
	  if(sweepon[P])
          {
           int32 mod=0;

	   if(SweepCount[P]>0) SweepCount[P]--; 
	   if(SweepCount[P]<=0)
	   {
	    SweepCount[P]=((PSG[(P<<2)+0x1]>>4)&7)+1; //+1;
            {
             if(PSG[(P<<2)+0x1]&0x8)
             {
              mod-=(P^1)+((curfreq[P])>>(PSG[(P<<2)+0x1]&7));          

              if(curfreq[P] && (PSG[(P<<2)+0x1]&7)/* && sweepon[P]&0x80*/)
              {
               curfreq[P]+=mod;
              }
             }
             else
             {
              mod=curfreq[P]>>(PSG[(P<<2)+0x1]&7);
              if((mod+curfreq[P])&0x800)
              {
               sweepon[P]=0;
               curfreq[P]=0;
              }
              else
              {
               if(curfreq[P] && (PSG[(P<<2)+0x1]&7)/* && sweepon[P]&0x80*/)
               {
                curfreq[P]+=mod;
               }
              }
             }
            }
	   }
          }
	  else
	  {
           //curfreq[P]&=0xFF00;
           //curfreq[P]|=PSG[(P<<2)|0x2]; //|((PSG[(P<<2)|3]&7)<<8); 
	  }
         }

       if(PSG[0x15]&0x8 && sqnon&8)
        {
         if(!(PSG[0xC]&0x20))
         {
          if(lengthcount[3]>0)
          {
           lengthcount[3]--;
           if(lengthcount[3]<=0)
           {
            sqnon&=~8;
           }
          }
         }
        }

  case 0:       /* Envelope decay + linear counter */
	 if(!(PSG[0x8]&0x80) && trimode)
	 {
	  trimode=0;
	  tricoop=PSG[0x8]&0x7F;
	 }
         if(!trimode)
         {
           if(tricoop)
           {
            if(tricoop==1) DoTriangle();
            tricoop--;
           }
         }

        for(P=0;P<2;P++)
        {
	  if(DecCountTo1[P]>0) DecCountTo1[P]--;
          if(DecCountTo1[P]<=0)
          {
	   DecCountTo1[P]=(PSG[P<<2]&0xF)+1;
           if(decvolume[P] || PSG[P<<2]&0x20)
           {
            decvolume[P]--;
	    /* Step from 0 to full volume seems to take twice as long
	       as the other steps.  I don't know if this is the correct
	       way to double its length, though(or if it even matters).
	    */
            if((PSG[P<<2]&0x20) && (decvolume[P]==0))
             DecCountTo1[P]<<=1;
	    decvolume[P]&=15;
           }
          }
          if(!(PSG[P<<2]&0x10))
           realvolume[P]=decvolume[P];
        }

         if(DecCountTo1[2]>0) DecCountTo1[2]--;
         if(DecCountTo1[2]<=0)
         {
          DecCountTo1[2]=(PSG[0xC]&0xF)+1;
          if(decvolume[2] || PSG[0xC]&0x20)
          {
            decvolume[2]--;
            /* Step from 0 to full volume seems to take twice as long
               as the other steps.  I don't know if this is the correct
               way to double its length, though(or if it even matters).
            */
            if((PSG[0xC]&0x20) && (decvolume[2]==0))
             DecCountTo1[2]<<=1;
            decvolume[2]&=15;
          }
         }
         if(!(PSG[0xC]&0x10))
          realvolume[2]=decvolume[2];

        break;
 }

}

void FrameSoundUpdate(void)
{
 // Linear counter:  Bit 0-6 of $4008
 // Length counter:  Bit 4-7 of $4003, $4007, $400b, $400f

 if(fcnt==3)
 {
	if(PSG[0x17]&0x80)
	 fhcnt+=fhinc;
        if(!(PSG[0x17]&0xC0))
        {
         SIRQStat|=0x40;
	 X6502_IRQBegin(FCEU_IQFCOUNT);         
        }
 }
 //if(SIRQStat&0x40) X6502_IRQBegin(FCEU_IQFCOUNT);
 FrameSoundStuff(fcnt);
 fcnt=(fcnt+1)&3;
}

static uint32 ChannelBC[5];

static INLINE int PCMFreak(void)
{
   int t;
   uint8 bah=PSG[0x11];

   if(!PCMBitIndex)
   { 
    if(!PCMSizeIndex)
    {
     if(PSG[0x10]&0x40)
      PrepDPCM();
     else
     {
      //if(PSG[0x10]&0x80)
      //{
      // SIRQStat|=0x80;
      // X6502_IRQBegin(FCEU_IQDPCM);
      //}
      PSG[0x15]&=~0x10;
      return(0);
     }
    } 
    else
    {   
     if(FSettings.soundq>=1) PCMacc-=1;
     PCMBuffer=X6502_DMR(0x8000+PCMAddressIndex);
	 if(loggingcodedata)LogPCM(0x8000+PCMAddressIndex);//bbit edited: this was added for the CDLogger
     PCMAddressIndex=(PCMAddressIndex+1)&0x7fff;
    }
    PCMSizeIndex--;
   }
   t=((((PCMBuffer>>PCMBitIndex)&1))<<2)-2;
   PSG[0x11]+=t;
   if(PSG[0x11]&0x80)
    PSG[0x11]=bah; 
   PCMBitIndex=(PCMBitIndex+1)&7;
  return(1);
}

void RDoPCM(void)
{
 int32 V;
 int32 cycs=timestamp-ChannelBC[4];
 //printf("Hohoho: %d, %d, %d\n",cycs,ChannelBC[4],timestamp);
 if(cycs<=0) return; // Shouldn't happen... but it will because of save states.

 if(PCMIRQCount>0)
 {
  PCMIRQCount-=cycs;
  if(PCMIRQCount<=0)
  {
   if((PSG[0x10]&0x80) && !(PSG[0x10]&0x40))
   {
    SIRQStat|=0x80;
    X6502_IRQBegin(FCEU_IQDPCM);
    PSG[0x15]&=~0x10;
   }
  }
 }
 if(PSG[0x15]&0x10)
 {
  //printf("gogo: %d %d %d\n",ChannelBC[4],timestamp,PCMacc);
  PCMacc-=cycs;
  /* Begin */
  //printf("%d,%d\n",PCMacc,PCMfreq);
  /* Weird bug.  Maybe I should rewrite this.  Occurs after save state loading. */
  if(ChannelBC[4]>=(timestamp+PCMacc)) PCMacc=0; //fprintf(stderr,"oops\n");
  while(PCMacc<=0)
  {
   for(V=ChannelBC[4];V<(timestamp+PCMacc);V++)
    WaveHi[V]+=PSG[0x11]<<16;
   ChannelBC[4]=timestamp+PCMacc;

   PCMacc+=PCMfreq;

   if(!PCMFreak()) goto nana;
  }
  /* End */
 }

 nana:
 {
  int32 V;
  uint32 *dest;
  uint32 data;

  dest=&WaveHi[ChannelBC[4]];
  data=PSG[0x11]<<16;
  for(V=timestamp-ChannelBC[4];V;V--,dest++)
   *dest+=data;
  ChannelBC[4]=timestamp;
 }
}

static void RDoPCMLQ(void)
{
   int32 V;
   int32 start,end;
   int32 freq;
   uint32 out=(PSG[0x11]<<3)-(PSG[0x11]<<1);

   start=ChannelBC[4];
   end=(timestamp<<16)/soundtsinc;
   if(end<=start) return;   
   ChannelBC[4]=end;

   if(PCMIRQCount>0)
   {   
    int cycs=timestamp-lpcmer;
    PCMIRQCount-=cycs;
    if(PCMIRQCount<=0)
    {
     if((PSG[0x10]&0x80) && !(PSG[0x10]&0x40))
     {     
      SIRQStat|=0x80;
      X6502_IRQBegin(FCEU_IQDPCM);
      PSG[0x15]&=~0x10;
     }     
    }
   } 
   lpcmer=timestamp;

   if(PSG[0x15]&0x10)
   {
      freq=PCMfreq;
      freq<<=17;

      for(V=start;V<end;V++)
      {
       PCMaccLQ-=nesincsize;
       if(PCMaccLQ<=0)
       {
        if(!PCMFreak())
	{
	 for(;V<end;V++)
            Wave[V>>4]+=(PSG[0x11]<<3)-(PSG[0x11]<<1);
           goto endopcmo;
	}
	PCMaccLQ+=freq;
       }
       Wave[V>>4]+=out;
      }
   }
   else
   {
     if((end-start)>64)
     {   
      for(V=start;V<=(start|15);V++)
       Wave[V>>4]+=out;
      out<<=4;
      for(V=(start>>4)+1;V<(end>>4);V++)
       Wave[V]+=out;
      out>>=4;
      for(V=end&(~15);V<end;V++)
       Wave[V>>4]+=out;
     }
     else
      for(V=start;V<end;V++)
       Wave[V>>4]+=out;
   }
    endopcmo:;
}

/* This has the correct phase.  Don't mess with it. */
static INLINE void RDoSQ(int x)
{
   int32 V;
   int32 amp;
   int32 rthresh;
   int32 *D;
   int32 currdc;
   int32 cf;
   int32 rc;

   if(curfreq[x]<8 || curfreq[x]>0x7ff)
    goto endit;
   if(!CheckFreq(curfreq[x],PSG[(x<<2)|0x1]))
    goto endit;
   if(!(PSG[0x15]&sqnon&(1<<x)))
    goto endit;

   amp=realvolume[x]<<24;
   rthresh=tal[(PSG[(x<<2)]&0xC0)>>6];

   D=&WaveHi[ChannelBC[x]];
   V=timestamp-ChannelBC[x];
   
   currdc=rdc[x];
   cf=curfreq[x]+1;
   rc=rcount[x];

   while(V>0)
   {
    if(currdc<rthresh)
     *D+=amp;
    rc--;
    if(!rc)
    {
     rc=cf;
     currdc=(currdc+1)&15;
    }
    V--;
    D++;
   }   
  
   rdc[x]=currdc;
   rcount[x]=rc;

   endit:
   ChannelBC[x]=timestamp;
}

static void RDoSQ1(void)
{
 RDoSQ(0);
}

static void RDoSQ2(void)
{
 RDoSQ(1);
}
#ifdef MOO
static INLINE void RDoSQVLQ(int x)
{
   int32 start,end;
   int32 freq;
   int32 V;
   int32 amp;
   int32 rthresh;
 
   start=ChannelBC[x];
   end=(timestamp<<16)/soundtsinc;
   if(end<=start) return;   
   ChannelBC[x]=end;

   if(curfreq[x]<8 || curfreq[x]>0x7ff)
    goto endit;
   if(!CheckFreq(curfreq[x],PSG[(x<<2)|0x1]))
    goto endit;
   if(!(PSG[0x15]&sqnon&(1<<x)))
    goto endit;
 
   amp=realvolume[x]*16*3/5;
   rthresh=tal[(PSG[(x<<2)]&0xC0)>>6];
   freq=curfreq[x]+1;
   freq<<=17;
     
   for(V=start;V<end;V++)
   { 
    if(rdc[x]<rthresh)   
     Wave[V]+=amp;
    sqacc[x]-=nesincsize>>4;
    if(sqacc[x]<=0)
    {
     rea:
     sqacc[x]+=freq;
     rdc[x]=(rdc[x]+1)&15;
     if(sqacc[x]<=0) goto rea;
    }
   }
   endit:;
}
#endif

static INLINE void RDoSQLQ(int x) 
{
   int32 start,end;    
   int32 freq;
   int32 V;
   int32 amp;
   int32 rthresh;

   start=ChannelBC[x];
   end=(timestamp<<16)/soundtsinc;
   if(end<=start) return;
   ChannelBC[x]=end;

   if(curfreq[x]<8 || curfreq[x]>0x7ff)
    goto endit;
   if(!CheckFreq(curfreq[x],PSG[(x<<2)|0x1]))
    goto endit;
   if(!(PSG[0x15]&sqnon&(1<<x)))
    goto endit;
    
   amp=realvolume[x]*16*3/5;
   rthresh=tal[(PSG[(x<<2)]&0xC0)>>6];
   freq=curfreq[x]+1;
   freq<<=17;

   for(V=start;V<end;V++)
   {
    if(rdc[x]<rthresh)
     Wave[V>>4]+=amp;
    sqacc[x]-=nesincsize;
    if(sqacc[x]<=0)
    {
     rea:
     sqacc[x]+=freq;
     rdc[x]=(rdc[x]+1)&15;
     if(sqacc[x]<=0) goto rea;
    }
   }
   endit:;
}

static void RDoSQ1LQ(void)
{
 RDoSQLQ(0);
}

static void RDoSQ2LQ(void)
{
 RDoSQLQ(1);
}

static void RDoTriangle(void)
{
 int32 V;
 int32 tcout;

 tcout=(tristep&0xF);
 if(tristep&0x10) tcout^=0xF;
 tcout=(tcout<<1);

 if(! (PSG[0x15]&0x4 && sqnon&4 && tricoop) )
 {   // Counter is halted, but we still need to output.
  for(V=ChannelBC[2];V<timestamp;V++)
   WaveHi[V]+=tcout<<16;
 }
 else
  for(V=ChannelBC[2];V<timestamp;V++)
  {
    WaveHi[V]+=tcout<<16;
    tricount--;
    if(!tricount)
    {
     tricount=(PSG[0xa]|((PSG[0xb]&7)<<8))+1;
     tristep++;
     tcout=(tristep&0xF);
     if(tristep&0x10) tcout^=0xF;
     tcout=(tcout<<1);
    }
  }

 ChannelBC[2]=timestamp;
}

static void RDoTriangleLQ(void)
{
   static uint32 tcout=0;
   int32 V;
   int32 start,end; //,freq;
   int32 freq=(((PSG[0xa]|((PSG[0xb]&7)<<8))+1));
     
   start=ChannelBC[2];
   end=(timestamp<<16)/soundtsinc;
   if(end<=start) return;
   ChannelBC[2]=end;
 
    if(! (PSG[0x15]&0x4 && sqnon&4 && tricoop) )
    {   // Counter is halted, but we still need to output.
     for(V=start;V<end;V++) 
       Wave[V>>4]+=tcout;
    }
    else if(freq<=4) // 55.9Khz - Might be barely audible on a real NES, but
               // it's too costly to generate audio at this high of a frequency
               // (55.9Khz * 32 for the stepping).
               // The same could probably be said for ~27.8Khz, so we'll
               // take care of that too.  We'll just output the average
               // value(15/2 - scaled properly for our output format, 
                //of course).
               // We'll also take care of ~18Khz and ~14Khz too, since 
                //they should be barely audible.
               // (Some proof or anything to confirm/disprove this 
                //would be nice.).
    {
     for(V=start;V<end;V++)
      Wave[V>>4]+=(0xF<<4)>>1;
    }   
    else
    {
     static int32 triacc=0;
     static uint8 tc=0;
               
      freq<<=17;
      for(V=start;V<end;V++)
      {
       triacc-=nesincsize;
       if(triacc<=0)
       {
        rea:
        triacc+=freq; //t; 
        tc=(tc+1)&0x1F;
        if(triacc<=0) goto rea;
        tcout=(tc&0xF);
        if(tc&0x10) tcout^=0xF;
        tcout=(tcout<<4);
       }
       Wave[V>>4]+=tcout;
      }        
    }
}


static void RDoNoise(void)
{
 int32 V;
 int32 outo;
 uint32 amptab[2];

 amptab[0]=realvolume[2]<<16;
 amptab[1]=0;
 outo=amptab[(nreg>>0xe)&1];

 if(!(PSG[0x15]&0x8 && sqnon&8))
 {
  ChannelBC[3]=timestamp;
  return;
 }
 if(PSG[0xE]&0x80)        // "short" noise
  for(V=ChannelBC[3];V<timestamp;V++)
  {
   WaveHi[V]+=outo;
   ncount--;
   if(!ncount)
   {
    uint8 feedback;
    ncount=SNoiseFreqTable[PSG[0xE]&0xF]<<1;
    feedback=((nreg>>8)&1)^((nreg>>14)&1);
    nreg=(nreg<<1)+feedback;
    nreg&=0x7fff;
    outo=amptab[(nreg>>0xe)&1];
   }
  }
 else
  for(V=ChannelBC[3];V<timestamp;V++)
  {
   WaveHi[V]+=outo;
   ncount--;
   if(!ncount)
   {
    uint8 feedback;
    ncount=SNoiseFreqTable[PSG[0xE]&0xF]<<1;
    feedback=((nreg>>13)&1)^((nreg>>14)&1);
    nreg=(nreg<<1)+feedback;
    nreg&=0x7fff;
    outo=amptab[(nreg>>0xe)&1];
   }
  }
 ChannelBC[3]=timestamp;
}

static void RDoNoiseLQ(void)
{
   int32 inc,V;
   int32 start,end;
   static int32 count=0;

   start=ChannelBC[3];
   end=(timestamp<<16)/soundtsinc;
   if(end<=start) return;
   ChannelBC[3]=end;
   if(PSG[0x15]&0x8 && sqnon&8)
   {
      uint32 outo;
      uint32 amptab[2];
 
      inc=LQNFTable[PSG[0xE]&0xF];
      amptab[0]=realvolume[2]<<3;
      amptab[1]=0;

      outo=amptab[(nreg>>0xe)&1];
      if(realvolume[2])
      {
       if(PSG[0xE]&0x80)        // "short" noise
        for(V=start;V<end;V++)
        {
         Wave[V>>4]+=outo;
         if(count>=inc)
         {
          uint8 feedback;
      
          feedback=((nreg>>8)&1)^((nreg>>14)&1);
          nreg=(nreg<<1)+feedback;
          nreg&=0x7fff;
	  outo=amptab[(nreg>>0xe)&1];
          count-=inc;
         }
         count+=0x4000;
        }
       else
        for(V=start;V<end;V++)
        {
         Wave[V>>4]+=outo;
         if(count>=inc)
         {
          uint8 feedback;
          
          feedback=((nreg>>13)&1)^((nreg>>14)&1);
          nreg=(nreg<<1)+feedback;
          nreg&=0x7fff;
	  outo=amptab[(nreg>>0xe)&1];
          count-=inc;
         }
         count+=0x4000;
        }
      }  
       
   }
}


void SetNESSoundMap(void)
{ 
  SetWriteHandler(0x4000,0x4013,Write_PSG);
  SetWriteHandler(0x4011,0x4011,Write0x11);
  SetWriteHandler(0x4015,0x4015,Write_PSG);
  SetWriteHandler(0x4017,0x4017,Write_PSG);        
  SetReadHandler(0x4015,0x4015,Read_PSG);
}

static int32 inbuf=0;
static uint32 lastpoo=0;
int FlushEmulateSound(void)
{
  int x;
  int32 end,left;

  if(!timestamp) return(0);

  if(!FSettings.SndRate)
  {
   left=0;
   end=0;
   goto nosoundo;
  }

  DoSQ1();
  DoSQ2();
  DoTriangle();
  DoNoise();
  DoPCM();

  if(FSettings.soundq>=1)
  {
   int32 *tmpo=&WaveHi[lastpoo];

   if(GameExpSound.HiFill) GameExpSound.HiFill();

   for(x=timestamp-lastpoo;x;x--)
   {
    uint32 b=*tmpo;    
    *tmpo=(b&65535)+wlookup2[(b>>16)&255]+wlookup1[b>>24];
    tmpo++;
   }
   end=NeoFilterSound(WaveHi,WaveFinal,timestamp,&left);

   memmove(WaveHi,WaveHi+timestamp-left,left*sizeof(uint32));
   memset(WaveHi+left,0,sizeof(WaveHi)-left*sizeof(uint32));

   if(GameExpSound.HiSync) GameExpSound.HiSync(left);
   for(x=0;x<5;x++)
    ChannelBC[x]=left;
  }
  else
  {
   end=(timestamp<<16)/soundtsinc;
   if(GameExpSound.Fill)
    GameExpSound.Fill(end&0xF);
   SexyFilter(Wave,WaveFinal,end>>4);
   if(FSettings.lowpass)
    SexyFilter2(WaveFinal,end>>4);
   if(end&0xF)
    Wave[0]=Wave[(end>>4)];
   Wave[end>>4]=0;
  }
  nosoundo:

  if(FSettings.soundq>=1)
  {
   timestampbase+=timestamp;
   timestamp=left;
   timestampbase-=timestamp;
   lastpoo=timestamp;
  }
  else
  {
   for(x=0;x<5;x++)
    ChannelBC[x]=end&0xF;
   timestampbase+=timestamp;
   timestamp=(soundtsinc*(end&0xF))>>16;
   timestampbase-=timestamp;
   end>>=4;
   lpcmer=timestamp;
  }
  inbuf=end;

  FCEU_WriteWaveData(WaveFinal, end); /* This function will just return
				    if sound recording is off. */
  return(end);
}

int GetSoundBuffer(int32 **W)
{
 *W=WaveFinal;
 return(inbuf);
}


void FCEUSND_Power(void)
{
	int x;

        SetNESSoundMap();

        memset(PSG,0,sizeof(PSG));
        curfreq[0]=curfreq[1]=0;
        sqnon=0;
        PSG[0x17]=0; //x40;
        fhcnt=fhinc;
        fcnt=0;
        nreg=1;
        memset(WaveHi,0,sizeof(WaveHi));
        for(x=0;x<5;x++)
         ChannelBC[x]=0;
        for(x=0;x<2;x++)
         rcount[x]=2048;
        tricount=2048;
        ncount=2048;
	lastpoo=0;
}

void FCEUSND_Reset(void)
{
        int x;
        for(x=0;x<0x16;x++)
         if(x!=1 && x!=5 && x!=0x14) BWrite[0x4000+x](0x4000+x,0);
        PSG[0x17]=0;
        fhcnt=fhinc;
        fcnt=0;
        nreg=1;
        for(x=0;x<2;x++)
	{
         rcount[x]=2048;
	 if(nesincsize) // lq mode
	  sqacc[x]=((uint32)2048<<17)/nesincsize;
	 else
	  sqacc[x]=1;
	}
        tricount=2048;
        ncount=2048;
}

void SetSoundVariables(void)
{
  int x;  

  fhinc=PAL?16626:14915;	// *2 CPU clock rate
  fhinc*=24;
  for(x=0;x<0x20;x++)
   lengthtable[x]=Slengthtable[x]<<1;

  if(FSettings.SndRate)
  {
   if(FSettings.soundq>=1)
   {
    for(x=0;x<192;x++)
    {
     uint64 tmp;
     tmp=21053UL*(uint64)x*x*x-12738700UL*(uint64)x*x+3778359010UL*(uint64)x;

     wlookup2[x]=tmp>>25;
     if(x<64 && !(x&1))
      wlookup1[x>>1]=(tmp*3/5)>>25;
    }
    DoNoise=RDoNoise;
    DoTriangle=RDoTriangle;
    DoPCM=RDoPCM;
    DoSQ1=RDoSQ1;
    DoSQ2=RDoSQ2;
   }
   else
   {
    DoNoise=DoTriangle=DoPCM=DoSQ1=DoSQ2=Dummyfunc;
    DoSQ1=RDoSQ1LQ;
    DoSQ2=RDoSQ2LQ;
    DoTriangle=RDoTriangleLQ;
    DoNoise=RDoNoiseLQ;
    DoPCM=RDoPCMLQ;
   }
  }  
  else
  {
   DoNoise=DoTriangle=DoPCM=DoSQ1=DoSQ2=Dummyfunc;
   return;
  }

  MakeFilters(FSettings.SndRate);

  if(GameExpSound.RChange)
   GameExpSound.RChange();

  nesincsize=(int64)(((int64)1<<17)*(double)(PAL?PAL_CPU:NTSC_CPU)/(FSettings.SndRate * 16));
  memset(sqacc,0,sizeof(sqacc));
  memset(ChannelBC,0,sizeof(ChannelBC));

  for(x=0;x<0x10;x++)
  {
   double z;
   z=SNoiseFreqTable[x]<<1;
   z=(PAL?PAL_CPU:NTSC_CPU)/z;
   z=(double)(FSettings.SndRate * 16)*16384/z;
   LQNFTable[x]=z;
   PALPCMTable[x]=(int64)PAL_CPU*NTSCPCMTable[x]/NTSC_CPU;
   //PALPCMTable[x]=NTSCPCMTable[x]*77/85;
  // printf("%02x\n",NTSCPCMTable[x]*77/85/2);
  }

  PSG_base=(uint32)(PAL?(long double)PAL_CPU/16:(long double)NTSC_CPU/16);

  soundtsinc=(uint32)((uint64)(PAL?(long double)PAL_CPU*65536:(long double)NTSC_CPU*65536)/(FSettings.SndRate * 16));
}

void FixOldSaveStateSFreq(void)
{
        int x;
        for(x=0;x<2;x++)
        {
         curfreq[x]=PSG[0x2+(x<<2)]|((PSG[0x3+(x<<2)]&7)<<8);
        }
}

void FCEUI_Sound(int Rate)
{
 FSettings.SndRate=Rate;
 SetSoundVariables();
}

void FCEUI_SetLowPass(int q)
{
 FSettings.lowpass=q;
}

void FCEUI_SetSoundQuality(int quality)
{
 FSettings.soundq=quality;
 SetSoundVariables();
}

void FCEUI_SetSoundVolume(uint32 volume)
{
 FSettings.SoundVolume=volume;
}


SFORMAT FCEUSND_STATEINFO[]={
 { &fhcnt, 4|FCEUSTATE_RLSB,"FHCN"},
 { &fcnt, 1, "FCNT"},  
 { PSG, 14, "PSG"},
 { &PSG[0x15], 1, "P15"},
 { &PSG[0x17], 1, "P17"},
 { decvolume, 3, "DECV"}, 
 { realvolume, 3, "RALV"}, 
 { &sqnon, 1, "SQNO"},
 { &nreg, 2|FCEUSTATE_RLSB, "NREG"},
 { &trimode, 1, "TRIM"},
 { &tricoop, 1, "TRIC"},
 { sweepon, 2, "SWEE"},
 { &curfreq[0], 4|FCEUSTATE_RLSB,"CRF1"},
 { &curfreq[1], 4|FCEUSTATE_RLSB,"CRF2"},
 { SweepCount, 2,"SWCT"},
 { DecCountTo1, 3,"DCT1"},
 { &PCMBitIndex, 1,"PBIN"},
 { &PCMAddressIndex, 4|FCEUSTATE_RLSB, "PAIN"},
 { &PCMSizeIndex, 4|FCEUSTATE_RLSB, "PSIN"},   
 { &PCMfreq, 4|FCEUSTATE_RLSB, "PCMF"},
 { 0 }
};

void FCEUSND_SaveState(void)
{

}

void FCEUSND_LoadState(int version)
{

}
