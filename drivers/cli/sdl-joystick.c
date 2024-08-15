/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Xodnizel
 *  Copyright (C) 2002 Paul Kuliniewicz
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

/* PK: SDL joystick input stuff */

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "sdl.h"
static SDL_Joystick *jo[4] = {NULL, NULL, NULL, NULL};

static void ConfigJoystick (int z);

#define JOY_A		0x01
#define JOY_B		0x02
#define JOY_SELECT	0x04
#define JOY_START	0x08
#define JOY_UP		0x10
#define JOY_DOWN	0x20
#define JOY_LEFT	0x40
#define JOY_RIGHT	0x80

/* Gets the current joystick position information. */
uint32 GetJSOr (void)
{
	static uint32 rtoggle=0; /* Rapid fire toggle thingy. */
	int n;			/* joystick index */
	int b;			/* button index */
	int *joym;		/* pointer to a joystick's button map */
	Sint16 pos;		/* axis position */
	uint32 ret = 0;		/* return value */


	if(joy[0]|joy[1]|joy[2]|joy[3])
	 SDL_JoystickUpdate();

	rtoggle^=1;
	for (n = 0; n < 4; n++)
	{
		if (joy[n] == 0)
			continue;
		joym = joyBMap[n];

		/* Axis information. */
		pos = SDL_JoystickGetAxis(jo[n], joyAMap[n][0]);
		if (pos <= -16383)
			ret |= JOY_LEFT << (n << 3);
		else if (pos >= 16363)
			ret |= JOY_RIGHT << (n << 3);
		pos = SDL_JoystickGetAxis(jo[n], joyAMap[n][1]);
		if (pos <= -16383)
			ret |= JOY_UP << (n << 3);
		else if (pos >= 16383)
			ret |= JOY_DOWN << (n << 3);

		/* Button information. */
		for (b = 0; b < 6; b++)
		{
		 if(joym[b])
		  if((b>=4 && rtoggle) || b<4)
			if (SDL_JoystickGetButton(jo[n], joym[b]-1))
				ret |= (1 << (b&3)) << (n << 3);
		}
	}

	return ret;
}

/* Cleanup opened joysticks. */
void KillJoysticks (void)
{
	int n;			/* joystick index */

	for (n = 0; n < 4; n++)
	{
		if (joy[n] != 0)
			SDL_JoystickClose(jo[n]);
	}
	SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
	return;
}

/* Initialize joysticks. */
int InitJoysticks (void)
{
	int n;			/* joystick index */

	if(!(joy[0]|joy[1]|joy[2]|joy[3]))
	 return(0);
	SDL_InitSubSystem(SDL_INIT_JOYSTICK);
	for (n = 0; n < 4; n++)
	{
		if (joy[n] == 0)
			continue;
		
		/* Open the joystick under SDL. */
		jo[n] = SDL_JoystickOpen(joy[n] - 1);
		if (jo[n] == NULL)
		{
			printf("Could not open joystick %d: %s.\n",
				joy[n] - 1, SDL_GetError());
			joy[n] = 0;
			continue;
		}
		printf("Physical Joystick #%d: %d axes, %d buttons\n", n+1, SDL_JoystickNumAxes(jo[n]), SDL_JoystickNumButtons(jo[n]));

		/* Check for a button map. */
		if (!(joyBMap[n][0]|joyBMap[n][1]|joyBMap[n][2]|joyBMap[n][3]))
		{
			ConfigJoystick(n);
		}
	}

	return (1);
}

static void WNoInput(void)
{
	uint8 t; 
	while(read(fileno(stdin),&t,1)!=-1);
}

/* Configure a joystick axis. */
void AConfig (int n, int a)
{
        Sint16 lastaxe[64];
        int numaxes;
        int axis;

	WNoInput();

        joyAMap[n][a] = a;

        numaxes=SDL_JoystickNumAxes(jo[n]);
        if(numaxes>64) numaxes=64;

        for(axis=0;axis<numaxes;axis++)	/* Pre-load. */
		lastaxe[axis]=SDL_JoystickGetAxis(jo[n], axis);

	while (1)
        {
                uint8 t;
                if (read(fileno(stdin), &t, 1) == -1)
                {
                        if (errno != EAGAIN)
                                break;
                }
                else
                        break;
               
                SDL_JoystickUpdate();

                for (axis=0;axis<numaxes;axis++) {
                	if (abs((Sint32)SDL_JoystickGetAxis(jo[n],axis)-lastaxe[axis]) > 8192) {
               			joyAMap[n][a] = axis;
					/* 4096 should be good enough to account for any jitter. */
                                	while (abs((Sint32)SDL_JoystickGetAxis(jo[n],axis)-lastaxe[axis]) > 4096) {
                                        	SDL_JoystickUpdate();
                                        	SDL_Delay(50);
                      			}
                                        goto endaconfig;
                	}
                }
                SDL_Delay(100);
        }

	endaconfig:
	WNoInput();
	return;
}

/* Configure a joystick button. */
void BConfig (int n, int b)
{
	WNoInput();
	joyBMap[n][b] = 0;
	while (1)
	{
		uint8 t;
		if (read(fileno(stdin), &t, 1) == -1)
		{
			if (errno != EAGAIN)
				break;
		}
		else
			break;
		
		SDL_JoystickUpdate();
		{
                       int buttons;
                       for (buttons = SDL_JoystickNumButtons(jo[n])-1;buttons >= 0;buttons--) {
                               if (SDL_JoystickGetButton(jo[n],buttons)) {
                                       joyBMap[n][b] = buttons+1;
                                       while (SDL_JoystickGetButton(jo[n], buttons)) {
                                               SDL_JoystickUpdate();
                                               SDL_Delay(50);
                                       }
                                       goto endbconfig;
                               }
                       }
		}
	   	SDL_Delay(100);
	}

	endbconfig:
	WNoInput();
	return;
}

/* Joystick button and axis configuration. */
void ConfigJoystick (int n)
{
	int sa;			/* buffer value */

	printf("\n\n Joystick button and axis configuration:\n\n");
	printf("   Select the joystick axes to use for the virtual d-pad.\n");
	printf("   If you do not wish to assign an axis, press Enter to skip\n");
	printf("   that axis.\n");
	printf("   Push the button to map to the virtual joystick.\n");
	printf("   If you do not wish to assign a button, press Enter to skip\n");
	printf("   that button.\n   Press enter to continue...\n");
	getchar();
	printf("****  Configuring joystick %d ****\n\n", n + 1);

	sa = fcntl(fileno(stdin), F_GETFL);
        fcntl(fileno(stdin), F_SETFL, O_NONBLOCK);

	printf("** Move axis to use for the x-axis (default 0).\n");
	AConfig(n,0);

	printf("** Move axis to use for the y-axis (default 1).\n");
	AConfig(n,1);

	printf("** Press button for \"Select\".\n");
	BConfig(n, 2);

	printf("** Press button for \"Start\".\n");
	BConfig(n, 3);

	printf("** Press button for \"B\".\n");
	BConfig(n, 1);

	printf("** Press button for \"A\".\n");
	BConfig(n, 0);
	        
        printf("** Press button for rapid fire \"B\".\n");
        BConfig(n, 5);
        
        printf("** Press button for rapid fire \"A\".\n");
        BConfig(n, 4);

	fcntl(fileno(stdin), F_SETFL, sa);
}
