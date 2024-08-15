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

#include "mapinc.h"

static uint8 spec;
static DECLFW(Mapper160_write)
{

 if(A>=0xd000)
  spec=V;
// if(A>=0x0d000)
// printf("$%04x:$%02x\n",A,V);
 if(spec==4)
 {
  if(A==0x9001) 
  {
   VROM_BANK4(0x0000,V>>1);
   VROM_BANK4(0x1000,V>>1);
  }
 }
 else if(spec==1)
 {
  if(A>=0x9000 && A<=0x9007) VROM_BANK1((A&7)*1024,V);
 }
 switch(A)
 {
  case 0x8000:ROM_BANK8(0x8000,V);break;
  case 0x8001:ROM_BANK8(0xa000,V);break;
  case 0x8002:ROM_BANK8(0xc000,V);break;
 }
}

void Mapper160_init(void)
{
 SetWriteHandler(0x8000,0xffff,Mapper160_write);
}
