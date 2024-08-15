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

FILE *soundlog=0;
void WriteWaveData(int32 *Buffer, int Count);
DWORD WINAPI DSThread(LPVOID lpParam);
LPDIRECTSOUND ppDS=0;
LPDIRECTSOUNDBUFFER ppbuf=0;
LPDIRECTSOUNDBUFFER ppbufsec=0;
LPDIRECTSOUNDBUFFER ppbufw;

DSBUFFERDESC DSBufferDesc;
WAVEFORMATEX wfa;
WAVEFORMATEX wf;

static int DSBufferSize=0;
static int bittage;

void TrashSound(void)
{
 FCEUI_Sound(0);
 if(ppbufsec)
 {
  IDirectSoundBuffer_Stop(ppbufsec);
  IDirectSoundBuffer_Release(ppbufsec);
  ppbufsec=0;
 }
 if(ppbuf)
 {
  IDirectSoundBuffer_Stop(ppbuf);
  IDirectSoundBuffer_Release(ppbuf);
  ppbuf=0;
 }
 if(ppDS)
 {
  IDirectSound_Release(ppDS);
  ppDS=0;
 }
}


 static VOID *feegle[2];
 static DWORD dook[2];
 static DWORD writepos=0,playpos=0,lplaypos=0;
void CheckDStatus(void)
{
  DWORD status;
  status=0;
  IDirectSoundBuffer_GetStatus(ppbufw, &status);

  if(status&DSBSTATUS_BUFFERLOST)
  {
   IDirectSoundBuffer_Restore(ppbufw);
  }

  if(!(status&DSBSTATUS_PLAYING))
  {
   lplaypos=0;
   writepos=((soundbufsize)<<bittage);
   IDirectSoundBuffer_SetFormat(ppbufw,&wf);
   IDirectSoundBuffer_Play(ppbufw,0,0,DSBPLAY_LOOPING);
  }
}

static int16 MBuffer[2048];
void FCEUD_WriteSoundData(int32 *Buffer, int Count)
{
 int P;

 if(soundlog)
  WriteWaveData(Buffer, Count);

 if(!bittage)
 {
  for(P=0;P<Count;P++)
   *(((uint8*)MBuffer)+P)=((int8)(Buffer[P]>>8))^128;
 }
 else
 {
  for(P=0;P<Count;P++)
   MBuffer[P]=Buffer[P];
 }
  ilicpo:
  CheckDStatus();
  IDirectSoundBuffer_GetCurrentPosition(ppbufw,&playpos,0);

  if(writepos>=DSBufferSize) 
   if(playpos<lplaypos)
    writepos-=DSBufferSize;
  lplaypos=playpos;

  /* If the write position is beyond the fill buffer, block. */
  if(writepos>=(playpos+(soundbufsize<<bittage)))
  //if(!(writepos<playpos+((soundbufsize)<<bittage)))
  {
   if(!NoWaiting)
   {
     int stime;      
     stime=writepos-(playpos+(soundbufsize<<bittage));
     stime*=1000;
     stime/=soundrate;
     stime>>=bittage;
      Sleep(stime);
   }
   //BlockingCheck();
   if(!soundo || NoWaiting) return;
   goto ilicpo;
  }

  if(netplaytype && netplayon)
  {
   if(writepos<=playpos+128)
   writepos=playpos+(soundbufsize<<bittage);
  }

  {
   feegle[0]=feegle[1]=0;
   dook[0]=dook[1]=0;

   
   ddrval=IDirectSoundBuffer_Lock(ppbufw,(writepos%DSBufferSize),Count<<bittage,&feegle[0],&dook[0],&feegle[1],&dook[1],0);
   if(ddrval!=DS_OK)
    goto nolock;

   if(feegle[1]!=0 && feegle[1]!=feegle[0])
   {
    if(soundflush)
    {
     memset(feegle[0],0x80,dook[0]);
     memset(feegle[1],0x80,dook[1]);
    }
    else
    {
     memcpy(feegle[0],(uint8 *)MBuffer,dook[0]);
     memcpy(feegle[1],((uint8 *)MBuffer)+dook[0],dook[1]);
    }
   }
   else
   {
    if(soundflush)
     memset(feegle[0],0x80,dook[0]);
    else
     memcpy(feegle[0],(uint8 *)MBuffer,dook[0]);
   }

   IDirectSoundBuffer_Unlock(ppbufw,feegle[0],dook[0],feegle[1],dook[1]);
   writepos+=Count<<bittage;
  }
 nolock: ;
 ///////// Ending
}

int InitSound()
{
 DSCAPS dscaps;
 DSBCAPS dsbcaps;

 memset(&wf,0x00,sizeof(wf));
 wf.wFormatTag = WAVE_FORMAT_PCM;
 wf.nChannels = 1;
 wf.nSamplesPerSec = soundrate;

 ddrval=DirectSoundCreate(0,&ppDS,0);
 if (ddrval != DS_OK)
 {
  FCEUD_PrintError("DirectSound: Error creating DirectSound object.");
  return 0;
 }

 if(soundoptions&SO_SECONDARY)
 {
  trysecondary:
  ddrval=IDirectSound_SetCooperativeLevel(ppDS,hAppWnd,DSSCL_PRIORITY);
  if (ddrval != DS_OK)
  {
   FCEUD_PrintError("DirectSound: Error setting cooperative level to DDSCL_PRIORITY.");
   TrashSound();
   return 0;
  }
 }
 else
 {
  ddrval=IDirectSound_SetCooperativeLevel(ppDS,hAppWnd,DSSCL_WRITEPRIMARY);
  if (ddrval != DS_OK)
  {
   FCEUD_PrintError("DirectSound: Error setting cooperative level to DDSCL_WRITEPRIMARY.  Forcing use of secondary sound buffer and trying again...");
   soundoptions|=SO_SECONDARY;
   goto trysecondary;
  }
 }
 memset(&dscaps,0x00,sizeof(dscaps));
 dscaps.dwSize=sizeof(dscaps);
 ddrval=IDirectSound_GetCaps(ppDS,&dscaps);
 if(ddrval!=DS_OK)
 {
  FCEUD_PrintError("DirectSound: Error getting capabilities.");
  return 0;
 }

 if(dscaps.dwFlags&DSCAPS_EMULDRIVER)
  FCEUD_PrintError("DirectSound: Sound device is being emulated through waveform-audio functions.  Sound quality will most likely be awful.  Try to update your sound device's sound drivers.");

 IDirectSound_Compact(ppDS);

 memset(&DSBufferDesc,0x00,sizeof(DSBUFFERDESC));
 DSBufferDesc.dwSize=sizeof(DSBufferDesc);
 if(soundoptions&SO_SECONDARY)
  DSBufferDesc.dwFlags=DSBCAPS_PRIMARYBUFFER;
 else
  DSBufferDesc.dwFlags=DSBCAPS_PRIMARYBUFFER|DSBCAPS_GETCURRENTPOSITION2;

 ddrval=IDirectSound_CreateSoundBuffer(ppDS,&DSBufferDesc,&ppbuf,0);
 if (ddrval != DS_OK)
 {
  FCEUD_PrintError("DirectSound: Error creating primary buffer.");
  TrashSound();
  return 0;
 } 

 memset(&wfa,0x00,sizeof(wfa));

 if(soundoptions&SO_FORCE8BIT)
  bittage=0;
 else
 {
  bittage=1;
  if( (!(dscaps.dwFlags&DSCAPS_PRIMARY16BIT) && !(soundoptions&SO_SECONDARY)) ||
      (!(dscaps.dwFlags&DSCAPS_SECONDARY16BIT) && (soundoptions&SO_SECONDARY)))
  {
   FCEUD_PrintError("DirectSound: 16-bit sound is not supported.  Forcing 8-bit sound.");
   bittage=0;
   soundoptions|=SO_FORCE8BIT;
  }
 }

 wf.wBitsPerSample=8<<bittage;
 wf.nBlockAlign = bittage+1;
 wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;
 
 ddrval=IDirectSoundBuffer_SetFormat(ppbuf,&wf);
 if (ddrval != DS_OK)
 {
  FCEUD_PrintError("DirectSound: Error setting primary buffer format.");
  TrashSound();
  return 0;
 }

 IDirectSoundBuffer_GetFormat(ppbuf,&wfa,sizeof(wfa),0);

 if(soundoptions&SO_SECONDARY)
 {
  memset(&DSBufferDesc,0x00,sizeof(DSBUFFERDESC));  
  DSBufferDesc.dwSize=sizeof(DSBufferDesc);
  DSBufferDesc.dwFlags=DSBCAPS_GETCURRENTPOSITION2;
  if(soundoptions&SO_GFOCUS)
   DSBufferDesc.dwFlags|=DSBCAPS_GLOBALFOCUS;
  DSBufferDesc.dwBufferBytes=65536;
  DSBufferDesc.lpwfxFormat=&wfa;  
  ddrval=IDirectSound_CreateSoundBuffer(ppDS, &DSBufferDesc, &ppbufsec, 0);
  if (ddrval != DS_OK)
  {
   FCEUD_PrintError("DirectSound: Error creating secondary buffer.");
   TrashSound();
   return 0;
  }
 }

 //sprintf(TempArray,"%d\n",wfa.nSamplesPerSec);
 //FCEUD_PrintError(TempArray);

 if(soundoptions&SO_SECONDARY)
 {
  DSBufferSize=65536;
  IDirectSoundBuffer_SetCurrentPosition(ppbufsec,0);
  ppbufw=ppbufsec;
 }
 else
 {
  memset(&dsbcaps,0,sizeof(dsbcaps));
  dsbcaps.dwSize=sizeof(dsbcaps);
  ddrval=IDirectSoundBuffer_GetCaps(ppbuf,&dsbcaps);
  if (ddrval != DS_OK)
  {
   FCEUD_PrintError("DirectSound: Error getting buffer capabilities.");
   TrashSound();
   return 0;
  }

  DSBufferSize=dsbcaps.dwBufferBytes;

  if(DSBufferSize<8192)
  {
   FCEUD_PrintError("DirectSound: Primary buffer size is too small!");
   TrashSound();
   return 0;
  }
  ppbufw=ppbuf;
 }

 soundbufsize=(soundbuftime*soundrate/1000);
 FCEUI_Sound(soundrate);
 return 1;
}

static HWND uug=0;

static void UpdateSD(HWND hwndDlg)
{
 int t;

 CheckDlgButton(hwndDlg,126,soundo?BST_CHECKED:BST_UNCHECKED);
 CheckDlgButton(hwndDlg,122,(soundoptions&SO_FORCE8BIT)?BST_CHECKED:BST_UNCHECKED);
 CheckDlgButton(hwndDlg,123,(soundoptions&SO_SECONDARY)?BST_CHECKED:BST_UNCHECKED);
 CheckDlgButton(hwndDlg,124,(soundoptions&SO_GFOCUS)?BST_CHECKED:BST_UNCHECKED);
 SendDlgItemMessage(hwndDlg,129,CB_SETCURSEL,soundquality,(LPARAM)(LPSTR)0);
 t=0;
 if(soundrate==22050) t=1;
 else if(soundrate==44100) t=2;
 else if(soundrate==48000) t=3;
 else if(soundrate==96000) t=4;
 SendDlgItemMessage(hwndDlg,200,CB_SETCURSEL,t,(LPARAM)(LPSTR)0);
}

BOOL CALLBACK SoundConCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

  switch(uMsg) {
   case WM_NCRBUTTONDOWN:
   case WM_NCMBUTTONDOWN:
   case WM_NCLBUTTONDOWN:StopSound();break;

   case WM_INITDIALOG:
                /* Volume Trackbar */
                SendDlgItemMessage(hwndDlg,500,TBM_SETRANGE,1,MAKELONG(0,250));
                SendDlgItemMessage(hwndDlg,500,TBM_SETTICFREQ,25,0);
                SendDlgItemMessage(hwndDlg,500,TBM_SETPOS,1,250-soundvolume);

                /* buffer size time trackbar */
                SendDlgItemMessage(hwndDlg,128,TBM_SETRANGE,1,MAKELONG(15,200));
                SendDlgItemMessage(hwndDlg,128,TBM_SETTICFREQ,1,0);
                SendDlgItemMessage(hwndDlg,128,TBM_SETPOS,1,soundbuftime);

                {
                 char tbuf[8];
                 sprintf(tbuf,"%d",soundbuftime);
                 SetDlgItemText(hwndDlg,666,(LPTSTR)tbuf);
                }

                SendDlgItemMessage(hwndDlg,129,CB_ADDSTRING,0,(LPARAM)(LPSTR)"Low");
                SendDlgItemMessage(hwndDlg,129,CB_ADDSTRING,0,(LPARAM)(LPSTR)"High");
                SendDlgItemMessage(hwndDlg,129,CB_ADDSTRING,0,(LPARAM)(LPSTR)"Highest");

                SendDlgItemMessage(hwndDlg,200,CB_ADDSTRING,0,(LPARAM)(LPSTR)"11025");
                SendDlgItemMessage(hwndDlg,200,CB_ADDSTRING,0,(LPARAM)(LPSTR)"22050");
                SendDlgItemMessage(hwndDlg,200,CB_ADDSTRING,0,(LPARAM)(LPSTR)"44100");
                SendDlgItemMessage(hwndDlg,200,CB_ADDSTRING,0,(LPARAM)(LPSTR)"48000");
                SendDlgItemMessage(hwndDlg,200,CB_ADDSTRING,0,(LPARAM)(LPSTR)"96000");

                UpdateSD(hwndDlg);
                break;
   case WM_VSCROLL:
                soundvolume=250-SendDlgItemMessage(hwndDlg,500,TBM_GETPOS,0,0);
                FCEUI_SetSoundVolume(soundvolume);
                break;
   case WM_HSCROLL:
                {
                 char tbuf[8];
                 soundbuftime=SendDlgItemMessage(hwndDlg,128,TBM_GETPOS,0,0);
                 sprintf(tbuf,"%d",soundbuftime);
                 SetDlgItemText(hwndDlg,666,(LPTSTR)tbuf);
                 soundbufsize=(soundbuftime*soundrate/1000);
                }
                break;
   case WM_CLOSE:
   case WM_QUIT: goto gornk;
   case WM_COMMAND:
                switch(HIWORD(wParam))
                {
                 case CBN_SELENDOK:
			switch(LOWORD(wParam))
                        {
			 case 200:
                         {
                          int tmp;
                          tmp=SendDlgItemMessage(hwndDlg,200,CB_GETCURSEL,0,(LPARAM)(LPSTR)0);
                          if(tmp==0) tmp=11025;
                          else if(tmp==1) tmp=22050;
                          else if(tmp==2) tmp=44100;
                          else if(tmp==3) tmp=48000;
                          else tmp=96000;
                          if(tmp!=soundrate)
                          {
                           soundrate=tmp;
                           if(soundrate<44100)
                           {
			    soundquality=0;
                            FCEUI_SetSoundQuality(0);
                            UpdateSD(hwndDlg);
                           }
                           if(soundo)
                           {
                            TrashSound();
                            soundo=InitSound();
                            UpdateSD(hwndDlg);
                           }
                          }
                         }
			 break;

			case 129:
			 soundquality=SendDlgItemMessage(hwndDlg,129,CB_GETCURSEL,0,(LPARAM)(LPSTR)0);
                         if(soundrate<44100) soundquality=0;
                         FCEUI_SetSoundQuality(soundquality);
                         UpdateSD(hwndDlg);
			 break;
			}
                        break;

                 case BN_CLICKED:
                        switch(LOWORD(wParam))
                        {
                         case 122:soundoptions^=SO_FORCE8BIT;   
                                  if(soundo)
                                  {
                                   TrashSound();
                                   soundo=InitSound();
                                   UpdateSD(hwndDlg);                                   
                                  }
                                  break;
                         case 123:soundoptions^=SO_SECONDARY;
                                  if(soundo)
                                  {
                                   TrashSound();
                                   soundo=InitSound();
                                   UpdateSD(hwndDlg);
                                  }
                                  break;
                         case 124:soundoptions^=SO_GFOCUS;
                                  if(soundo)
                                  {
                                   TrashSound();
                                   soundo=InitSound();
                                   UpdateSD(hwndDlg);
                                  }
                                  break;
                         case 126:soundo=!soundo;
                                  if(!soundo) TrashSound();
                                  else soundo=InitSound();
                                  UpdateSD(hwndDlg);
                                  break;
                        }
                }

                if(!(wParam>>16))
                switch(wParam&0xFFFF)
                {
                 case 1:
                        gornk:                         
                        DestroyWindow(hwndDlg);
                        uug=0;
                        break;
               }
              }
  return 0;
}


void ConfigSound(void)
{
 if(!uug)
  uug=CreateDialog(fceu_hInstance,"SOUNDCONFIG",0,SoundConCallB);
 else
  SetFocus(uug);
}


void StopSound(void)
{
 if(soundo)
  IDirectSoundBuffer_Stop(ppbufw);
}

#include "wave.c"
