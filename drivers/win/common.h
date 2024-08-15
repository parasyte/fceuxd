#include <stdio.h>
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h> //bbit edited:this line added

#ifndef WIN32
#define WIN32
#endif
#undef  WINNT
#define NONAMELESSUNION

#define DIRECTSOUND_VERSION  0x0700
#define DIRECTDRAW_VERSION 0x0700
#define DIRECTINPUT_VERSION     0x700
#include "../../driver.h"

/* Message logging(non-netplay messages, usually) for all. */
#include "log.h"
extern HWND hAppWnd;
extern HINSTANCE fceu_hInstance;

extern int NoWaiting;
extern FCEUGI *GI;
void DSMFix(UINT msg);
void StopSound(void);
