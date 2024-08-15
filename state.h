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

void SaveState(void);
void LoadState(void);

extern uint8 StateName[2048];
extern uint8 StateFile[2048];
extern int CurrentState;
extern int SaveStateStatus[10];
void CheckStates(void);
void SaveStateRefresh(void);

typedef struct {
           void *v;
           uint32 s;
	   char *desc;
} SFORMAT;

void ResetExState(void);
void AddExState(void *v, uint32 s, int type, char *desc);

#define FCEUSTATE_RLSB            0x80000000
