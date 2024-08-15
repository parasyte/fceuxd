#ifdef MACOSX
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <OpenGL/glext.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>
#endif
#include <string.h>
#include <stdlib.h>

#include "sdl.h"
#include "sdl-opengl.h"
#include "../common/vidblit.h"

GLuint textures[2];	// Normal image, scanline overlay.

static int left,right,top,bottom; // right and bottom are not inclusive.
static int scanlines;
static void *HiBuffer;

void SetOpenGLPalette(uint8 *data)
{
 if(!HiBuffer)
  glColorTableEXT(GL_TEXTURE_2D,GL_RGB,256,GL_RGBA,GL_UNSIGNED_BYTE,data);
}

void BlitOpenGL(uint8 *buf)
{
 glBindTexture(GL_TEXTURE_2D, textures[0]);
 if(HiBuffer)
 {
  Blit8ToHigh(buf,HiBuffer,256,240,512*4,1,1); 
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,256,256, 0, GL_RGBA,GL_UNSIGNED_BYTE,
			HiBuffer);
 }
 else
 {
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 272);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_COLOR_INDEX8_EXT, 256, 256, 0,
		GL_COLOR_INDEX,GL_UNSIGNED_BYTE,buf);
 }
 //glColor4d(1.0f,1.0f,1.0f,0.5f);
 //glEnable(GL_BLEND);
 //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
 //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
 //glBlendFunc(GL_SRC_COLOR, GL_SRC_ALPHA);

 glBegin(GL_QUADS);
 glTexCoord2f(1.0f*left/256, 1.0f*bottom/256);	// Bottom left of our picture.
 glVertex2f(-1.0f, -1.0f);	// Bottom left of target.

 glTexCoord2f(1.0f*right/256, 1.0f*bottom/256);	// Bottom right of our picture.
 glVertex2f( 1.0f, -1.0f);	// Bottom right of target.

 glTexCoord2f(1.0f*right/256, 1.0f*top/256);	// Top right of our picture.
 glVertex2f( 1.0f,  1.0f);	// Top right of target.

 glTexCoord2f(1.0f*left/256, 1.0f*top/256);	// Top left of our picture.
 glVertex2f(-1.0f,  1.0f);	// Top left of target.
 glEnd();
 //glDisable(GL_BLEND);
 if(scanlines)
 {
  glEnable(GL_BLEND);

  glBindTexture(GL_TEXTURE_2D, textures[1]);
  glBlendFunc(GL_DST_COLOR, GL_SRC_ALPHA);

  glBegin(GL_QUADS);
  glTexCoord2f(1.0f*left/256, 1.0f*bottom/256);  // Bottom left of our picture.
  glVertex2f(-1.0f, -1.0f);      // Bottom left of target.
 
  glTexCoord2f(1.0f*right/256, 1.0f*bottom/256); // Bottom right of our picture.
  glVertex2f( 1.0f, -1.0f);      // Bottom right of target.
 
  glTexCoord2f(1.0f*right/256, 1.0f*top/256);    // Top right of our picture.
  glVertex2f( 1.0f,  1.0f);      // Top right of target.
 
  glTexCoord2f(1.0f*left/256, 1.0f*top/256);     // Top left of our picture.
  glVertex2f(-1.0f,  1.0f);      // Top left of target.

  glEnd();
  glDisable(GL_BLEND);
 }
  SDL_GL_SwapBuffers();
}

void KillOpenGL(void)
{
 if(HiBuffer)
 {
  free(HiBuffer);
  HiBuffer=0;
 }
}
/* Rectangle, left, right(not inclusive), top, bottom(not inclusive). */

int InitOpenGL(int l, int r, int t, int b, double xscale,double yscale, int efx, 
		int stretchx, int stretchy, SDL_Surface *screen)
{
 const char *extensions;

 left=l;
 right=r;
 top=t;
 bottom=b;

 HiBuffer=0;
 
 extensions=glGetString(GL_EXTENSIONS);
 if(!strstr(extensions,"GL_EXT_paletted_texture"))
 {
  puts("Paletted texture extension not found.  Using slower texture format...");
  HiBuffer=malloc(4*256*256);
  memset(HiBuffer,0x00,4*256*256);
  #ifndef LSB_FIRST
  InitBlitToHigh(4,0xFF000000,0xFF0000,0xFF00,0);
  #else
  InitBlitToHigh(4,0xFF,0xFF00,0xFF0000,0);
  #endif
 }

 {
  int rw=(r-l)*xscale;
  int rh=(b-t)*yscale;
  int sx=(screen->w-rw)/2;     // Start x
  int sy=(screen->h-rh)/2;      // Start y

  if(stretchx) { sx=0; rw=screen->w; }
  if(stretchy) { sy=0; rh=screen->h; }
  glViewport(sx, sy, rw, rh);
 }
 glGenTextures(2, &textures[0]);

 scanlines=0;
 if(efx&1)
 {
  uint8 *buf;
  int x,y;

  scanlines=1;
  buf=malloc(256*512*4);

  for(y=0;y<512;y++)
  for(x=0;x<256;x++)
  {
   buf[y*256*4+x*4]=0;
   buf[y*256*4+x*4+1]=0;
   buf[y*256*4+x*4+2]=0;
   buf[y*256*4+x*4+3]=(y&1)?0x00:0xFF; //?0xa0:0xFF; // <-- Pretty
   //buf[y*256+x]=(y&1)?0x00:0xFF;
  }
  glBindTexture(GL_TEXTURE_2D, textures[1]);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,(efx&2)?GL_LINEAR:GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,(efx&2)?GL_LINEAR:GL_NEAREST);

  //glTexImage2D(GL_TEXTURE_2D, 0, GL_COLOR_INDEX8_EXT, 256, 512, 0,
  //              GL_COLOR_INDEX,GL_UNSIGNED_BYTE,buf);
  
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 256, 512, 0,
                GL_RGBA,GL_UNSIGNED_BYTE,buf);
  //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 256, 512, 0, GL_ALPHA8, GL_UNSIGNED_BYTE,buf);
  free(buf);
 }
 glBindTexture(GL_TEXTURE_2D, textures[0]);
     
 glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,(efx&2)?GL_LINEAR:GL_NEAREST);
 glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,(efx&2)?GL_LINEAR:GL_NEAREST);
 glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
 glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
 glEnable(GL_TEXTURE_2D);
 glClearColor(0.0f, 0.0f, 0.0f, 0.0f);	// Background color to black.
 glMatrixMode(GL_MODELVIEW);

 glClear(GL_COLOR_BUFFER_BIT);
 glLoadIdentity();
}
