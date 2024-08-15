/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 1998 BERO
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
        {
	 uint8 *S=PALRAM+cc;
	 uint32 pixdata;

	 pixdata=ppulut1[C[0]]|ppulut2[C[8]];

	 P[0]=S[pixdata&3];
	 pixdata>>=2;
	 P[1]=S[pixdata&3];
	 pixdata>>=2;
         P[2]=S[pixdata&3];
         pixdata>>=2;
         P[3]=S[pixdata&3];
         pixdata>>=2;
         P[4]=S[pixdata&3];
         pixdata>>=2;
         P[5]=S[pixdata&3];
         pixdata>>=2;
         P[6]=S[pixdata&3];
         pixdata>>=2;
         P[7]=S[pixdata];

	#ifdef OLDnonono
	 uint8 *S=PALRAM+cc;
	 register uint8 c1,c2;
         
	 c1=((C[0]>>1)&0x55)|(C[8]&0xAA);
         c2=(C[0]&0x55)|((C[8]<<1)&0xAA);

         P[6]=S[c1&3];
         P[7]=S[c2&3];
         P[4]=S[(c1>>2)&3];
         P[5]=S[(c2>>2)&3];
         P[2]=S[(c1>>4)&3];
         P[3]=S[(c2>>4)&3];

         P[0]=S[c1>>6];
         P[1]=S[c2>>6];
	#endif
        }
