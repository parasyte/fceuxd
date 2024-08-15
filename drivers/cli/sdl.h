#include <SDL.h>
#include "../../driver.h"
#include "../common/args.h"
#include "../common/config.h"
#include "main.h"

typedef struct {
        int xres;
        int yres;
	double xscale,yscale;
	double xscalefs,yscalefs;
	int efx,efxfs;
	int bpp,bppfs;
        int fullscreen;
	int sound;
	int devdsp;
	int lbufsize,ebufsize;
	int joy[4];
	int joyAMap[4][2];
	int joyBMap[4][6];
	char *fshack;
	char *fshacksave;
	#ifdef OPENGL
	int opengl;
	int stretchx,stretchy;
	#endif
} DSETTINGS;

extern DSETTINGS Settings;

#define _devdsp Settings.devdsp
#define _bpp Settings.bpp
#define _bppfs Settings.bppfs
#define _xres Settings.xres
#define _yres Settings.yres
#define _fullscreen Settings.fullscreen
#define _sound Settings.sound
#define _xscale Settings.xscale
#define _yscale Settings.yscale
#define _xscalefs Settings.xscalefs
#define _yscalefs Settings.yscalefs
#define _efx Settings.efx
#define _efxfs Settings.efxfs
#define _ebufsize Settings.ebufsize
#define _lbufsize Settings.lbufsize
#define _fshack Settings.fshack
#define _fshacksave Settings.fshacksave

#ifdef OPENGL
#define _opengl Settings.opengl
#define _stretchx Settings.stretchx
#define _stretchy Settings.stretchy
#endif

#define joyAMap Settings.joyAMap
#define joyBMap Settings.joyBMap
#define joy	Settings.joy
