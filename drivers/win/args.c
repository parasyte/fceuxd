/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2003 Xodnizel
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

#include "../common/args.h"

char *ParseArgies(int argc, char *argv[])
{
        static char *cortab[5]={"none","gamepad","zapper","powerpad","arkanoid"};
        static int cortabi[5]={SI_NONE,SI_GAMEPAD,
                               SI_ZAPPER,SI_POWERPAD,SI_ARKANOID};
        static char *fccortab[5]={"none","arkanoid","shadow","4player","fkb"};
        static int fccortabi[5]={SIFC_NONE,SIFC_ARKANOID,SIFC_SHADOW,
                                 SIFC_4PLAYER,SIFC_FKB};
         
        int x;
        static char *inputa[2]={0,0};
        static char *fcexp=0;
 
        static ARGPSTRUCT FCEUArgs[]={
         {"-pal",0,&palyo,0},
         {"-input1",0,&inputa[0],0x4001},{"-input2",0,&inputa[1],0x4001},
         {"-fcexp",0,&fcexp,0x4001},
         {"-gg",0,&genie,0},
         {"-no8lim",0,&eoptions,0x8000|EO_NOSPRLIM},
         {"-nofs",0,&NoFourScore,0},   
         {"-clipsides",0,&eoptions,0x8000|EO_CLIPSIDES},  
         {"-nothrottle",0,&eoptions,0x8000|EO_NOTHROTTLE},
	};

       if(argc<=1) return(0);

       ParseArguments(argc-2, &argv[1], FCEUArgs);

       if(fcexp)
       {
        int y;
        for(y=0;y<5;y++) 
        {
         if(!strncmp(fccortab[y],fcexp,8))
         {
          UsrInputTypeFC=fccortabi[y];
          break;
         }
        }
        free(fcexp);
       }
       for(x=0;x<2;x++)
       {   
        int y;
         
        if(!inputa[x])
         continue;
          for(y=0;y<5;y++)
        {
         if(!strncmp(cortab[y],inputa[x],8))
         {
          UsrInputType[x]=cortabi[y];
          if(y==3)
          {
           powerpadside&=~(1<<x);
           powerpadside|=((((inputa[x][8])-'a')&1)^1)<<x;
          }
          free(inputa[x]);
         }
        }
       }
       return(argv[argc-1]);
}
