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

#ifdef NETWORK

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "svga.h"
#include "netplay.h"
#include        "fce.h"

int netplay=0;         /*      1 if we are a server */
                        /*      2 if we need a host  */

static uint8 netjoy[4]; /* Controller cache. */

static void NetError(void)
{
 FCEU_DispMessage("Network error/connection lost!");
 netplay=0;
 FCEUD_NetworkClose();
}

void KillNetplay(void)
{
 if(netplay)
 {
  FCEUD_NetworkClose();
  netplay=0;
 }
}

int InitNetplay(void)
{
         if(!FCEUD_NetworkConnect())
          {NetError();return 0;}
         netplay=FSettings.NetworkPlay;
	 memset(netjoy,0,sizeof(netjoy));

         FCEUGameInfo.input[0]=
         FCEUGameInfo.input[1]=SI_GAMEPAD;
//         FCEUGameInfo.inputfc=SIFC_4PLAYER;
         return 1;
}

void NetplayUpdate(uint8 *joyp)
{
 uint8 buf[5];

 if(netplay==1) /* Our side is the server side. */
 {
  FCEUD_GetDataFromClients(&netjoy[1]);
  netjoy[0]=joyp[0]|joyp[1]|joyp[2]|joyp[3];
  memcpy(buf,netjoy,4);
  buf[4]=CommandQueue;
  if(!FCEUD_SendDataToClients(buf))
   NetError();

  if(CommandQueue)
  {
   DoCommand(CommandQueue);
   CommandQueue=0;
  }

 }
 else           /* Our side is the client side(netplay==2). */
 {
  if(!FCEUD_SendDataToServer(joyp[0]|joyp[1]|joyp[2]|joyp[3],CommandQueue))
   NetError();
  if(!FCEUD_GetDataFromServer(buf))
   NetError();
  memcpy(netjoy,buf,4);
  if(buf[4]) DoCommand(buf[4]);
  CommandQueue=0;
 }
 *(uint32 *)joyp=*(uint32 *)netjoy;
}
#endif
