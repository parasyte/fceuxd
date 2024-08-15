#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sdl.h"
#include "sdl-video.h"
#ifdef NETWORK
#include "unix-netplay.h"
#endif

DSETTINGS Settings;
CFGSTRUCT DriverConfig[]={
	#ifdef UNIXDSP
	AC(_devdsp),
	#endif
	#ifdef OPENGL
	AC(_stretchx),
	AC(_stretchy),
	AC(_opengl),
	#endif
	AC(_xscale),
	AC(_yscale),
	AC(_xscalefs),
	AC(_yscalefs),
	AC(_bpp),
	AC(_bppfs),
	AC(_efx),
	AC(_efxfs),
        AC(_sound),
	AC(_ebufsize),
	AC(_lbufsize),
	AC(_fullscreen),
        AC(_xres),
	AC(_yres),
        ACA(joyBMap),
	ACA(joyAMap),
        ACA(joy),
        //ACS(_fshack),
        ENDCFGSTRUCT
};

//-fshack x       Set the environment variable SDL_VIDEODRIVER to \"x\" when
//                entering full screen mode and x is not \"0\".

char *DriverUsage=
#ifdef UNIXDSP
"-devdsp x	Use /dev/dsp(x-1) for sound output.  -1 for auto-find.  -1 is the default setting.\n"
#endif
"-xres   x	Set horizontal resolution to x for full screen mode.\n\
-yres   x       Set vertical resolution to x for full screen mode.\n\
-xscale(fs) x	Multiply width by x(Real numbers >0 with OpenGL, otherwise integers >0).\n\
-yscale(fs) x	Multiply height by x(Real numbers >0 with OpenGL, otherwise integers >0).\n\
-bpp(fs) x	Bits per pixel for SDL surface(and video mode in fs). 8, 16, 32.\n\
-opengl x	Enable OpenGL support if x is 1.\n\
-stretch(x/y) x	Stretch to fill surface on x or y axis(fullscreen, only with OpenGL).\n\
-efx(fs) x	Enable special effects.  Logically OR the following together:\n\
		 1 = scanlines(for yscale>=2).\n\
		 2 = TV blur(for bpp of 16 or 32) or linear interpolation(OpenGL).\n\
-fs	 x      Select full screen mode if x is non zero.\n\
-joyx   y       Use joystick y as virtual joystick x.\n\
-sound x        Sound.\n\
                 0 = Disabled.\n\
                 Otherwise, x = playback rate.\n\
-lbufsize x	Internal FCE Ultra sound buffer size. Size = 2^x samples.\n\
-ebufsize x	External SDL sound buffer size. Size = 2^x samples.\n\
-connect s      Connect to server 's' for TCP/IP network play.\n\
-server         Be a host/server for TCP/IP network play.\n\
-netportu x	Use UDP/IP port x for network play.\n\
-netport x      Use TCP/IP port x for network play.";

static int docheckie[2]={0,0};
ARGPSTRUCT DriverArgs[]={
         {"-joy1",0,&joy[0],0},{"-joy2",0,&joy[1],0},
         {"-joy3",0,&joy[2],0},{"-joy4",0,&joy[3],0},
	#ifdef OPENGL
	 {"-opengl",0,&_opengl,0},
	 {"-stretchx",0,&_stretchx,0},
	 {"-stretchy",0,&_stretchy,0},
	#endif
	 {"-bpp",0,&_bpp,0},
	 {"-bppfs",0,&_bppfs,0},
 	 #ifdef UNIXDSP
	 {"-devdsp",0,&_devdsp,0},
	 #endif
	 {"-xscale",0,&_xscale,2},
	 {"-yscale",0,&_yscale,2},
	 {"-efx",0,&_efx,0},
         {"-xscalefs",0,&_xscalefs,2},
         {"-yscalefs",0,&_yscalefs,2},
         {"-efxfs",0,&_efxfs,0},
	 {"-xres",0,&_xres,0},
         {"-yres",0,&_yres,0},
         {"-fs",0,&_fullscreen,0},
         //{"-fshack",0,&_fshack,0x4001},
         {"-sound",0,&_sound,0},
	 {"-lbufsize",0,&_lbufsize,0},
	 {"-ebufsize",0,&_ebufsize,0},
	 #ifdef NETWORK
         {"-connect",&docheckie[0],&netplayhost,0x4001},
         {"-server",&docheckie[1],0,0},
         {"-netport",0,&tport,0},
	 {"-netportu",0,&uport,0},
	 #endif
         {0,0,0,0}
};

static void SetDefaults(void)
{
 #ifdef UNIXDSP
 _devdsp=-1;
 #endif
 _bpp=0;
 _bppfs=8;
 _xres=640;
 _yres=480;
 _fullscreen=0;
 _sound=48000;
 _lbufsize=10;
 _ebufsize=9;
 _xscale=2.40;
 _yscale=2;
 _xscalefs=_yscalefs=2;
 _efx=_efxfs=0;
 //_fshack=_fshacksave=0;
#ifdef OPENGL
 _opengl=1;
 _stretchx=1; 
 _stretchy=0;
 _efx=_efxfs=2;
#endif
 memset(joy,0,sizeof(joy));
}

void DoDriverArgs(void)
{
        int x;

	#ifdef BROKEN
        if(_fshack)
        {
         if(_fshack[0]=='0')
          if(_fshack[1]==0)
          {
           free(_fshack);
           _fshack=0;
          }
        }
	#endif

	#ifdef NETWORK
        if(docheckie[0])
         netplay=2;
        else if(docheckie[1])
         netplay=1;

        if(netplay)
         FCEUI_SetNetworkPlay(netplay);
	#endif

        for(x=0;x<4;x++)
         if(!joy[x]) 
	 {
	  memset(joyBMap[x],0,sizeof(joyBMap[0]));
	  memset(joyAMap[x],0,sizeof(joyAMap[0]));
	 }
}

int InitMouse(void)
{
 return(0);
}

void KillMouse(void){}

void GetMouseData(uint32 *d)
{
 int x,y;
 uint32 t;

 t=SDL_GetMouseState(&x,&y);
 d[2]=0;
 if(t&SDL_BUTTON(1))
  d[2]|=1;
 if(t&SDL_BUTTON(3))
  d[2]|=2;
 t=PtoV(x,y); 
 d[0]=t&0xFFFF;
 d[1]=(t>>16)&0xFFFF;
}

int InitKeyboard(void)
{
 return(1);
}

int UpdateKeyboard(void)
{
 return(1);
}

void KillKeyboard(void)
{

}

char *GetKeyboard(void)
{
 SDL_PumpEvents();
 return(SDL_GetKeyState(0));
}
#include "unix-basedir.h"

int main(int argc, char *argv[])
{
        puts("\nStarting FCE Ultra "VERSION_STRING"...\n");
	if(SDL_Init(SDL_INIT_VIDEO)) /* SDL_INIT_VIDEO Needed for (joystick config) event processing? */
	{
	 printf("Could not initialize SDL: %s.\n", SDL_GetError());
	 return(-1);
	}
	SetDefaults();

	{
	 int ret=CLImain(argc,argv);
	 SDL_Quit();
	 return(ret);
	}
}

