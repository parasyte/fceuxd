/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Ben Parnell
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

#include "common.h"
#include "..\..\xstring.h"
#include "..\..\debugger.h"
#include "..\..\x6502.h"
#include "..\..\fce.h"
#include "..\..\nsf.h"
#include "..\..\cart.h"
#include "..\..\ines.h"
#include "..\..\tracer.h"
#include "..\..\memview.h"

extern readfunc ARead[0x10000];
int DbgPosX,DbgPosY;
int WP_edit=-1;
int ChangeWait=0,ChangeWait2=0;
uint8 debugger_open=0;
HWND hDebug;
static HFONT hFont,hNewFont;
static SCROLLINFO si;

int iaPC;
int iapoffset;
int step,stepout,jsrcount;
int u; //deleteme
int skipdebug; //deleteme
int GetNesFileAddress(int A){
	if((A < 0x8000) || (A > 0xFFFF))return -1;
	int result = &Page[A>>11][A]-PRGptr[0];
	if((result > PRGsize[0]) || (result < 0))return -1;
	else return result+16; //16 bytes for the header remember
}

int GetPRGAddress(int A){
	if((A < 0x8000) || (A > 0xFFFF))return -1;
	int result = &Page[A>>11][A]-PRGptr[0];
	if((result > PRGsize[0]) || (result < 0))return -1;
	else return result;
}

int GetRomAddress(int A){
	A-=16;
	uint8 *p = GetNesPRGPointer(A);
	int i;
	for(i = 16;i < 32;i++){
		if((&Page[i][i<<11] <= p) && (&Page[i][(i+1)<<11] > p))break;
	}
	if(i == 32)return -1; //not found

	return (i<<11) + (p-&Page[i][i<<11]);

}

uint8 *GetNesPRGPointer(int A){
	return PRGptr[0]+A;
}

uint8 *GetNesCHRPointer(int A){
	return CHRptr[0]+A;
}

uint8 GetMem(uint16 A) {
	if ((A >= 0x2000) && (A < 0x4000)) {
		switch (A&7) {
			case 0: return PPU[0];
			case 1: return PPU[1];
			case 2: return PPU[2]|(PPUGenLatch&0x1F);
			case 3: return PPU[3];
			case 4: return SPRAM[PPU[3]];
			case 5: return XOffset;
			case 6: return RefreshAddr&0xFF;
			case 7: return VRAMBuffer;
		}
	}
	else if ((A >= 0x4000) && (A < 0x6000)) return 0xFF; //fix me
	return ARead[A](A);
}

uint8 GetPPUMem(uint8 A) {
	uint16 tmp=RefreshAddr&0x3FFF;

	if (tmp<0x2000) return VPage[tmp>>10][tmp];
	if (tmp>=0x3F00) return PALRAM[tmp&0x1F];
	return vnapage[(tmp>>10)&0x3][tmp&0x3FF];
}


BOOL CenterWindow(HWND hwndDlg) {
    HWND hwndParent;
    RECT rect, rectP;
    int width, height;
    int screenwidth, screenheight;
    int x, y;

    //move the window relative to its parent
    hwndParent = GetParent(hwndDlg);

    GetWindowRect(hwndDlg, &rect);
    GetWindowRect(hwndParent, &rectP);

    width  = rect.right  - rect.left;
    height = rect.bottom - rect.top;

    x = ((rectP.right-rectP.left) -  width) / 2 + rectP.left;
    y = ((rectP.bottom-rectP.top) - height) / 2 + rectP.top;

    screenwidth  = GetSystemMetrics(SM_CXSCREEN);
    screenheight = GetSystemMetrics(SM_CYSCREEN);

    //make sure that the dialog box never moves outside of the screen
    if(x < 0) x = 0;
    if(y < 0) y = 0;
    if(x + width  > screenwidth)  x = screenwidth  - width;
    if(y + height > screenheight) y = screenheight - height;

    MoveWindow(hwndDlg, x, y, width, height, FALSE);

    return TRUE;
}

int NewBreak(HWND hwndDlg, int num, int enable) {
	unsigned int brk=0,ppu=0,sprite=0;
	char str[5];

	GetDlgItemText(hwndDlg,200,str,5);
	if (IsDlgButtonChecked(hwndDlg,106) == BST_CHECKED) ppu = 1;
	else if (IsDlgButtonChecked(hwndDlg,107) == BST_CHECKED) sprite = 1;
	if ((!ppu) && (!sprite)) {
		if (GI->type == GIT_NSF) { //NSF Breakpoint keywords
			if (strcmp(str,"LOAD") == 0) brk = (NSFHeader.LoadAddressLow | (NSFHeader.LoadAddressHigh<<8));
			if (strcmp(str,"INIT") == 0) brk = (NSFHeader.InitAddressLow | (NSFHeader.InitAddressHigh<<8));
			if (strcmp(str,"PLAY") == 0) brk = (NSFHeader.PlayAddressLow | (NSFHeader.PlayAddressHigh<<8));
		}
		else if (GI->type == GIT_FDS) { //FDS Breakpoint keywords
			if (strcmp(str,"NMI1") == 0) brk = (GetMem(0xDFF6) | (GetMem(0xDFF7)<<8));
			if (strcmp(str,"NMI2") == 0) brk = (GetMem(0xDFF8) | (GetMem(0xDFF9)<<8));
			if (strcmp(str,"NMI3") == 0) brk = (GetMem(0xDFFA) | (GetMem(0xDFFB)<<8));
			if (strcmp(str,"RST") == 0) brk = (GetMem(0xDFFC) | (GetMem(0xDFFD)<<8));
			if ((strcmp(str,"IRQ") == 0) || (strcmp(str,"BRK") == 0)) brk = (GetMem(0xDFFE) | (GetMem(0xDFFF)<<8));
		}
		else { //NES Breakpoint keywords
			if ((strcmp(str,"NMI") == 0) || (strcmp(str,"VBL") == 0)) brk = (GetMem(0xFFFA) | (GetMem(0xFFFB)<<8));
			if (strcmp(str,"RST") == 0) brk = (GetMem(0xFFFC) | (GetMem(0xFFFD)<<8));
			if ((strcmp(str,"IRQ") == 0) || (strcmp(str,"BRK") == 0)) brk = (GetMem(0xFFFE) | (GetMem(0xFFFF)<<8));
		}
	}
	if ((brk == 0) && (sscanf(str,"%4X",&brk) == EOF)) return 1;
	if (ppu) brk &= 0x3FFF;
	if (sprite) brk &= 0x00FF;
	watchpoint[num].address = brk;

	watchpoint[num].endaddress = 0;
	GetDlgItemText(hwndDlg,201,str,5);
	sscanf(str,"%4X",&brk);
	if (ppu) brk &= 0x3FFF;
	if (sprite) brk &= 0x00FF;
	if ((brk != 0) && (watchpoint[num].address < brk)) watchpoint[num].endaddress = brk;

	watchpoint[num].flags = 0;
	if (enable) watchpoint[num].flags|=WP_E;
	if (IsDlgButtonChecked(hwndDlg,102) == BST_CHECKED) watchpoint[num].flags|=WP_R;
	if (IsDlgButtonChecked(hwndDlg,103) == BST_CHECKED) watchpoint[num].flags|=WP_W;
	if (IsDlgButtonChecked(hwndDlg,104) == BST_CHECKED) watchpoint[num].flags|=WP_X;
	if (ppu) {
		watchpoint[num].flags|=BT_P;
		watchpoint[num].flags&=~WP_X; //disable execute flag!
	}
	if (sprite) {
		watchpoint[num].flags|=BT_S;
		watchpoint[num].flags&=~WP_X; //disable execute flag!
	}

	return 0;
}

int AddBreak(HWND hwndDlg) {
	if (numWPs == 64) return 1;
	if (NewBreak(hwndDlg,numWPs,1)) return 2;
	numWPs++;
	return 0;
}

BOOL CALLBACK AddbpCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	char str[8]={0};
	int tmp;

	switch(uMsg) {
		case WM_INITDIALOG:
			CenterWindow(hwndDlg);
			SendDlgItemMessage(hwndDlg,200,EM_SETLIMITTEXT,4,0);
			SendDlgItemMessage(hwndDlg,201,EM_SETLIMITTEXT,4,0);
			if (WP_edit >= 0) {
				SetWindowText(hwndDlg,"Edit Breakpoint...");

				sprintf(str,"%04X",watchpoint[WP_edit].address);
				SetDlgItemText(hwndDlg,200,str);
				sprintf(str,"%04X",watchpoint[WP_edit].endaddress);
				if (strcmp(str,"0000") != 0) SetDlgItemText(hwndDlg,201,str);
				if (watchpoint[WP_edit].flags&WP_R) CheckDlgButton(hwndDlg, 102, BST_CHECKED);
				if (watchpoint[WP_edit].flags&WP_W) CheckDlgButton(hwndDlg, 103, BST_CHECKED);
				if (watchpoint[WP_edit].flags&WP_X) CheckDlgButton(hwndDlg, 104, BST_CHECKED);

				if (watchpoint[WP_edit].flags&BT_P) {
					CheckDlgButton(hwndDlg, 106, BST_CHECKED);
					EnableWindow(GetDlgItem(hwndDlg,104),FALSE);
				}
				else if (watchpoint[WP_edit].flags&BT_S) {
					CheckDlgButton(hwndDlg, 107, BST_CHECKED);
					EnableWindow(GetDlgItem(hwndDlg,104),FALSE);
				}
				else CheckDlgButton(hwndDlg, 105, BST_CHECKED);
			}
			else CheckDlgButton(hwndDlg, 105, BST_CHECKED);
			break;
		case WM_CLOSE:
		case WM_QUIT:
			break;
		case WM_COMMAND:
			switch(HIWORD(wParam)) {
				case BN_CLICKED:
					switch(LOWORD(wParam)) {
						case 100:
							if (WP_edit >= 0) {
								NewBreak(hwndDlg,WP_edit,(BOOL)(watchpoint[WP_edit].flags&WP_E));
								EndDialog(hwndDlg,1);
								break;
							}
							if ((tmp=AddBreak(hwndDlg)) == 1) {
								MessageBox(hwndDlg, "Too many breakpoints, please delete one and try again", "Breakpoint Error", MB_OK);
								goto endaddbrk;
							}
							if (tmp == 2) goto endaddbrk;
							EndDialog(hwndDlg,1);
							break;
						case 101:
							endaddbrk:
							EndDialog(hwndDlg,0);
							break;
						case 105: //CPU Mem
							EnableWindow(GetDlgItem(hwndDlg,104),TRUE);
							break;
						case 106: //PPU Mem
						case 107: //Sprtie Mem
							EnableWindow(GetDlgItem(hwndDlg,104),FALSE);
							break;
					}
					break;
			}
        	break;
	}
	return FALSE; //TRUE;
}


char *BinToASM(int addr, uint8 *opcode) {
	static char str[64]={0},chr[5]={0};
	uint16 tmp,tmp2;

	switch (opcode[0]) {
		#include "dasm.h"
	}
	return str;
}

void Disassemble(HWND hWnd, int id, int scrollid, unsigned int addr) {
	char str[1024]={0},chr[25]={0};
	int size,i,j;
	uint8 opcode[3];

	si.nPos = addr;
	SetScrollInfo(GetDlgItem(hWnd,scrollid),SB_CTL,&si,TRUE);

	for (i = 0; i < 20; i++) {
		if (addr > 0xFFFF) break;
		sprintf(chr, "$%04X:", addr);
		strcat(str,chr);
		if ((size = opsize[GetMem(addr)]) == 0) {
			sprintf(chr, "%02X        UNDEFINED", GetMem(addr++));
			strcat(str,chr);
		}
		else {
			if ((addr+size) > 0x10000) { //should this be 0xFFFF?
				while (addr < 0x10000) {
					sprintf(chr, "%02X        OVERFLOW\r\n", GetMem(addr++));
					strcat(str,chr);
				}
				break;
			}
			for (j = 0; j < size; j++) {
				sprintf(chr, "%02X ", opcode[j] = GetMem(addr++));
				strcat(str,chr);
			}
			while (size < 3) {
				strcat(str,"   "); //pad output to align ASM
				size++;
			}
			strcat(strcat(str," "),BinToASM(addr,opcode));
		}
		strcat(str,"\r\n");
	}
	SetDlgItemText(hWnd,id,str);
}

char *DisassembleLine(int addr) {
	static char str[64]={0},chr[25]={0};
	char *c;
	int size,j;
	uint8 opcode[3];

	sprintf(str, "$%04X:", addr);
	if ((size = opsize[GetMem(addr)]) == 0) {
		sprintf(chr, "%02X        UNDEFINED", GetMem(addr++));
		strcat(str,chr);
	}
	else {
		if ((addr+size) > 0x10000) {
			sprintf(chr, "%02X        OVERFLOW", GetMem(addr));
			strcat(str,chr);
		}
		else {
			for (j = 0; j < size; j++) {
				sprintf(chr, "%02X ", opcode[j] = GetMem(addr++));
				strcat(str,chr);
			}
			while (size < 3) {
				strcat(str,"   "); //pad output to align ASM
				size++;
			}
			strcat(strcat(str," "),BinToASM(addr,opcode));
		}
	}
	if ((c=strchr(str,'='))) *(c-1) = 0;
	if ((c=strchr(str,'@'))) *(c-1) = 0;
	return str;
}

char *DisassembleData(int addr, uint8 *opcode) {
	static char str[64]={0},chr[25]={0};
	char *c;
	int size,j;

	sprintf(str, "$%04X:", addr);
	if ((size = opsize[opcode[0]]) == 0) {
		sprintf(chr, "%02X        UNDEFINED", opcode[0]);
		strcat(str,chr);
	}
	else {
		if ((addr+size) > 0x10000) {
			sprintf(chr, "%02X        OVERFLOW", opcode[0]);
			strcat(str,chr);
		}
		else {
			for (j = 0; j < size; j++) {
				sprintf(chr, "%02X ", opcode[j]);
				addr++;
				strcat(str,chr);
			}
			while (size < 3) {
				strcat(str,"   "); //pad output to align ASM
				size++;
			}
			strcat(strcat(str," "),BinToASM(addr,opcode));
		}
	}
	if ((c=strchr(str,'='))) *(c-1) = 0;
	if ((c=strchr(str,'@'))) *(c-1) = 0;
	return str;
}


int GetEditHex(HWND hwndDlg, int id) {
	char str[9];
	int tmp;
	GetDlgItemText(hwndDlg,id,str,9);
	tmp = strtol(str,NULL,16);
	return tmp;
}

int *GetEditHexData(HWND hwndDlg, int id){
	static int data[31];
	char str[60];
	int i,j, k;

	GetDlgItemText(hwndDlg,id,str,60);
	memset(data,0,30*sizeof(int));
	j=0;
	for(i = 0;i < 60;i++){
		if(str[i] == 0)break;
		if((str[i] >= '0') && (str[i] <= '9'))j++;
		if((str[i] >= 'A') && (str[i] <= 'F'))j++;
		if((str[i] >= 'a') && (str[i] <= 'f'))j++;
	}

	j=j&1;
	for(i = 0;i < 60;i++){
		if(str[i] == 0)break;
		k = -1;
		if((str[i] >= '0') && (str[i] <= '9'))k=str[i]-'0';
		if((str[i] >= 'A') && (str[i] <= 'F'))k=(str[i]-'A')+10;
		if((str[i] >= 'a') && (str[i] <= 'f'))k=(str[i]-'a')+10;
		if(k != -1){
			if(j&1)data[j>>1] |= k;
			else data[j>>1] |= k<<4;
			j++;
		}
	}
	data[j>>1]=-1;
	return data;
}

/*
int GetEditStack(HWND hwndDlg) {
	char str[85];
	int tmp;
	GetDlgItemText(hwndDlg,308,str,85);
	sscanf(str,"%2x,%2x,%2x,%2x,\r\n",&tmp);
	return tmp;
}
*/

void UpdateRegs(HWND hwndDlg) {
	X.A = GetEditHex(hwndDlg,304);
	X.X = GetEditHex(hwndDlg,305);
	X.Y = GetEditHex(hwndDlg,306);
	X.PC = GetEditHex(hwndDlg,307);
}

void UpdateDebugger() {
	char str[256]={0},chr[8];
	int tmp,ret,i;

	Disassemble(hDebug, 300, 301, X.PC);

	sprintf(str, "%02X", X.A);
	SetDlgItemText(hDebug, 304, str);
	sprintf(str, "%02X", X.X);
	SetDlgItemText(hDebug, 305, str);
	sprintf(str, "%02X", X.Y);
	SetDlgItemText(hDebug, 306, str);
	sprintf(str, "%04X", (int)X.PC);
	SetDlgItemText(hDebug, 307, str);

	sprintf(str, "%04X", (int)RefreshAddr);
	SetDlgItemText(hDebug, 310, str);
	sprintf(str, "%02X", PPU[3]);
	SetDlgItemText(hDebug, 311, str);

	sprintf(str, "Scanline: %d", scanline);
	SetDlgItemText(hDebug, 501, str);

	tmp = X.S|0x0100;
	sprintf(str, "Stack $%04X", tmp);
	SetDlgItemText(hDebug, 403, str);
	tmp = ((tmp+1)|0x0100)&0x01FF;
	sprintf(str, "%02X", GetMem(tmp));
	for (i = 1; i < 20; i++) {
		tmp = ((tmp+1)|0x0100)&0x01FF;  //increment and fix pointer to $0100-$01FF range
		if ((i%4) == 0) sprintf(chr, ",\r\n%02X", GetMem(tmp));
		else sprintf(chr, ",%02X", GetMem(tmp));
		strcat(str,chr);
	}
	SetDlgItemText(hDebug, 308, str);

	GetDlgItemText(hDebug,309,str,5);
	if (((ret = sscanf(str,"%4X",&tmp)) == EOF) || (ret != 1)) tmp = 0;
	sprintf(str,"%04X",tmp);
	SetDlgItemText(hDebug,309,str);

	CheckDlgButton(hDebug, 200, BST_UNCHECKED);
	CheckDlgButton(hDebug, 201, BST_UNCHECKED);
	CheckDlgButton(hDebug, 202, BST_UNCHECKED);
	CheckDlgButton(hDebug, 203, BST_UNCHECKED);
	CheckDlgButton(hDebug, 204, BST_UNCHECKED);
	CheckDlgButton(hDebug, 205, BST_UNCHECKED);
	CheckDlgButton(hDebug, 206, BST_UNCHECKED);
	CheckDlgButton(hDebug, 207, BST_UNCHECKED);

	tmp = X.P;
	if (tmp & N_FLAG) CheckDlgButton(hDebug, 200, BST_CHECKED);
	if (tmp & V_FLAG) CheckDlgButton(hDebug, 201, BST_CHECKED);
	if (tmp & U_FLAG) CheckDlgButton(hDebug, 202, BST_CHECKED);
	if (tmp & B_FLAG) CheckDlgButton(hDebug, 203, BST_CHECKED);
	if (tmp & D_FLAG) CheckDlgButton(hDebug, 204, BST_CHECKED);
	if (tmp & I_FLAG) CheckDlgButton(hDebug, 205, BST_CHECKED);
	if (tmp & Z_FLAG) CheckDlgButton(hDebug, 206, BST_CHECKED);
	if (tmp & C_FLAG) CheckDlgButton(hDebug, 207, BST_CHECKED);
}

char *BreakToText(unsigned int num) {
	static char str[20],chr[8];

	sprintf(str, "$%04X", watchpoint[num].address);
	if (watchpoint[num].endaddress) {
		sprintf(chr, "-$%04X", watchpoint[num].endaddress);
		strcat(str,chr);
	}
	if (watchpoint[num].flags&WP_E) strcat(str,": E"); else strcat(str,": -");
	if (watchpoint[num].flags&BT_P) strcat(str,"P"); else if (watchpoint[num].flags&BT_S) strcat(str,"S"); else strcat(str,"C");
	if (watchpoint[num].flags&WP_R) strcat(str,"R"); else strcat(str,"-");
	if (watchpoint[num].flags&WP_W) strcat(str,"W"); else strcat(str,"-");
	if (watchpoint[num].flags&WP_X) strcat(str,"X"); else strcat(str,"-");

	return str;
}

void AddBreakList() {
	SendDlgItemMessage(hDebug,302,LB_INSERTSTRING,-1,(LPARAM)(LPSTR)BreakToText(numWPs-1));
}

void EditBreakList() {
	if (WP_edit >= 0) {
		SendDlgItemMessage(hDebug,302,LB_DELETESTRING,WP_edit,0);
		SendDlgItemMessage(hDebug,302,LB_INSERTSTRING,WP_edit,(LPARAM)(LPSTR)BreakToText(WP_edit));
		SendDlgItemMessage(hDebug,302,LB_SETCURSEL,WP_edit,0);
	}
}

void FillBreakList(HWND hwndDlg) {
	unsigned int i;

	for (i = 0; i < numWPs; i++) {
		SendDlgItemMessage(hwndDlg,302,LB_INSERTSTRING,-1,(LPARAM)(LPSTR)BreakToText(i));
	}
}

void EnableBreak(int sel) {
	watchpoint[sel].flags^=WP_E;
	SendDlgItemMessage(hDebug,302,LB_DELETESTRING,sel,0);
	SendDlgItemMessage(hDebug,302,LB_INSERTSTRING,sel,(LPARAM)(LPSTR)BreakToText(sel));
	SendDlgItemMessage(hDebug,302,LB_SETCURSEL,sel,0);
}

void DeleteBreak(int sel) {
	int i;

	for (i = sel; i < numWPs; i++) {
		watchpoint[i].address = watchpoint[i+1].address;
		watchpoint[i].endaddress = watchpoint[i+1].endaddress;
		watchpoint[i].flags = watchpoint[i+1].flags;
	}
	numWPs--;
	SendDlgItemMessage(hDebug,302,LB_DELETESTRING,sel,0);
	EnableWindow(GetDlgItem(hDebug,102),FALSE);
	EnableWindow(GetDlgItem(hDebug,103),FALSE);
}

void KillDebugger() {
	SendDlgItemMessage(hDebug,302,LB_RESETCONTENT,0,0);
	numWPs = 0;
	step = 0;
	stepout = 0;
	jsrcount = 0;
	userpause = 0;
}


int AddAsmHistory(HWND hwndDlg, int id, char *str) {
	int index;
	index = SendDlgItemMessage(hwndDlg,id,CB_FINDSTRINGEXACT,-1,(LPARAM)(LPSTR)str);
	if (index == CB_ERR) {
		SendDlgItemMessage(hwndDlg,id,CB_INSERTSTRING,-1,(LPARAM)(LPSTR)str);
		return 0;
	}
	return 1;
}

int ParseASM(unsigned char *output, int addr, char *str) {
	unsigned char opcode[3] = { 0,0,0 };
	char astr[128],ins[4];

	if ((!strlen(str)) || (strlen(str) > 0x127)) return 1;

	strcpy(astr,str);
	str_ucase(astr);
	sscanf(astr,"%3s",ins); //get instruction
	if (strlen(ins) != 3) return 1;
	strcpy(astr,strstr(astr,ins)+3); //heheh, this is probably a bad idea, but let's do it anyway!
	if ((astr[0] != ' ') && (astr[0] != 0)) return 1;

	//remove all whitespace
	str_strip(astr,STRIP_SP|STRIP_TAB|STRIP_CR|STRIP_LF);

	//repair syntax
	chr_replace(astr,'[','(');	//brackets
	chr_replace(astr,']',')');
	chr_replace(astr,'{','(');
	chr_replace(astr,'}',')');
	chr_replace(astr,';',0);	//comments
	str_replace(astr,"0X","$");	//miscellaneous

	//This does the following:
	// 1) Sets opcode[0] on success, else returns 1.
	// 2) Parses text in *astr to build the rest of the assembled
	//    data in 'opcode', else returns 1 on error.
	#include "asm.h"

	memcpy(output,opcode,3);
	return 0;
}

BOOL CALLBACK AssemblerCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	int romaddr,count,i,j;
	char str[128],*dasm;
	static int patchlen,applied,saved,lastundo;
	static uint8 patchdata[64][3],undodata[64*3];
	uint8 *ptr;

	switch(uMsg) {
		case WM_INITDIALOG:
			CenterWindow(hwndDlg);

			//set font
			SendDlgItemMessage(hwndDlg,101,WM_SETFONT,(WPARAM)hNewFont,FALSE);
			SendDlgItemMessage(hwndDlg,102,WM_SETFONT,(WPARAM)hNewFont,FALSE);

			//set limits
			SendDlgItemMessage(hwndDlg,100,CB_LIMITTEXT,20,0);

			SetDlgItemText(hwndDlg,101,DisassembleLine(iaPC));
			SetFocus(GetDlgItem(hwndDlg,100));

			patchlen = 0;
			applied = 0;
			saved = 0;
			lastundo = 0;
			break;
		case WM_CLOSE:
		case WM_QUIT:
			EndDialog(hwndDlg,0);
			break;
		case WM_COMMAND:
			switch (HIWORD(wParam)) {
				case BN_CLICKED:
					switch (LOWORD(wParam)) {
						case 201: //Apply
							if (patchlen) {
								ptr = GetNesPRGPointer(GetNesFileAddress(iaPC)-16);
								count = 0;
								for (i = 0; i < patchlen; i++) {
									for (j = 0; j < opsize[patchdata[i][0]]; j++) {
										if (count == lastundo) undodata[lastundo++] = ptr[count];
										ptr[count++] = patchdata[i][j];
									}
								}
								SetWindowText(hwndDlg, "Inline Assembler  *Patches Applied*");
								//MessageBeep(MB_OK);
								applied = 1;
							}
							break;
						case 202: //Save...
							if (applied) {
								count = romaddr = GetNesFileAddress(iaPC);
								for (i = 0; i < patchlen; i++) count += opsize[patchdata[i][0]];
								if (patchlen) sprintf(str,"Write patch data to file at addresses 0x%06X - 0x%06X?",romaddr,count-1);
								else sprintf(str,"Undo all previously applied patches?");
								if (MessageBox(hwndDlg, str, "Save changes to file?", MB_YESNO|MB_ICONINFORMATION) == IDYES) {
									if (iNesSave()) {
										saved = 1;
										applied = 0;
									}
									else MessageBox(hwndDlg, "Unable to save changes to file", "Error saving to file", MB_OK);
								}
							}
							break;
						case 203: //Undo
							if ((count = SendDlgItemMessage(hwndDlg,102,LB_GETCOUNT,0,0))) {
								SendDlgItemMessage(hwndDlg,102,LB_DELETESTRING,count-1,0);
								patchlen--;
								count = 0;
								for (i = 0; i < patchlen; i++) count += opsize[patchdata[i][0]];
								if (count < lastundo) {
									ptr = GetNesPRGPointer(GetNesFileAddress(iaPC)-16);
									j = opsize[patchdata[patchlen][0]];
									for (i = count; i < (count+j); i++) {
										ptr[i] = undodata[i];
									}
									lastundo -= j;
									applied = 1;
								}
								SetDlgItemText(hwndDlg,101,DisassembleLine(iaPC+count));
							}
							break;
						case 300: //Hidden default button
							count = 0;
							for (i = 0; i < patchlen; i++) count += opsize[patchdata[i][0]];
							GetDlgItemText(hwndDlg,100,str,21);
							if (!ParseASM(patchdata[patchlen],(iaPC+count),str)) {
								count = iaPC;
								for (i = 0; i <= patchlen; i++) count += opsize[patchdata[i][0]];
								if (count > 0x10000) { //note: don't use 0xFFFF!
									MessageBox(hwndDlg, "Patch data cannot exceed address 0xFFFF", "Address error", MB_OK);
									break;
								}
								SetDlgItemText(hwndDlg,100,"");
								if (count < 0x10000) SetDlgItemText(hwndDlg,101,DisassembleLine(count));
								else SetDlgItemText(hwndDlg,101,"OVERFLOW");
								dasm = DisassembleData((count-opsize[patchdata[patchlen][0]]),patchdata[patchlen]);
								SendDlgItemMessage(hwndDlg,102,LB_INSERTSTRING,-1,(LPARAM)(LPSTR)dasm);
								AddAsmHistory(hwndDlg,100,dasm+16);
								SetWindowText(hwndDlg, "Inline Assembler");
								patchlen++;
							}
							else { //ERROR!
								SetWindowText(hwndDlg, "Inline Assembler  *Syntax Error*");
								MessageBeep(MB_ICONEXCLAMATION);
							}
							break;
					}
					SetFocus(GetDlgItem(hwndDlg,100)); //set focus to combo box after anything is pressed!
					break;
			}
			break;
	}
	return FALSE;
}

BOOL CALLBACK PatcherCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	unsigned char str[64];
	uint8 *c;
	int i;
	int *p;

	switch(uMsg) {
		case WM_INITDIALOG:
			CenterWindow(hwndDlg);

			//set limits
			SendDlgItemMessage(hwndDlg,102,EM_SETLIMITTEXT,6,0);
			SendDlgItemMessage(hwndDlg,109,EM_SETLIMITTEXT,30,0);
			UpdatePatcher(hwndDlg);

			if(iapoffset != -1){
				CheckDlgButton(hwndDlg, 101, BST_CHECKED);
				sprintf(str,"%X",iapoffset);
				SetDlgItemText(hwndDlg,102,str);
			}

			SetFocus(GetDlgItem(hwndDlg,100));
			break;
		case WM_CLOSE:
		case WM_QUIT:
			EndDialog(hwndDlg,0);
			break;
		case WM_COMMAND:
			switch(HIWORD(wParam)) {
				case BN_CLICKED:
					switch(LOWORD(wParam)) {
						case 103: //todo: maybe get rid of this button and cause iapoffset to update every time you change the text
							if(IsDlgButtonChecked(hwndDlg,101) == BST_CHECKED){
								iapoffset = GetEditHex(hwndDlg,102);
							} else iapoffset = GetNesFileAddress(GetEditHex(hwndDlg,102));
							if((iapoffset < 16) && (iapoffset != -1)){
								MessageBox(hDebug, "Sorry, iNes Header editing isn't supported", "Error", MB_OK);
								iapoffset = -1;
							}
							if((iapoffset > PRGsize[0]) && (iapoffset != -1)){
								MessageBox(hDebug, "Error: .Nes offset outside of PRG rom", "Error", MB_OK);
								iapoffset = -1;
							}
							UpdatePatcher(hwndDlg);
							break;
						case 110:
							p = GetEditHexData(hwndDlg,109);
							i=0;
							c = GetNesPRGPointer(iapoffset-16);
							while(p[i] != -1){
								c[i] = p[i];
								i++;
							}
							UpdatePatcher(hwndDlg);
							break;
						case 111:
							if(!iNesSave())MessageBox(NULL,"Error Saving","Error",MB_OK);
							break;
					}
					break;
			}
			break;
	}
	return FALSE;
}

extern void StopSound();
extern char *iNesShortFName();

BOOL CALLBACK DebuggerCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	LOGFONT lf;
	RECT wrect;
	char str[256]={0},*ptr,dotdot[4];
	int tmp,tmp2;
	int mouse_x,mouse_y;
	int ret,i;
	//FILE *fp;

	//these messages get handled at any time
	switch(uMsg) {
		case WM_INITDIALOG:
			SetWindowPos(hwndDlg,0,DbgPosX,DbgPosY,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOOWNERZORDER);

			si.cbSize = sizeof(SCROLLINFO);
			si.fMask = SIF_ALL;
			si.nMin = 0;
			si.nMax = 0x10000;
			si.nPos = 0;
			si.nPage = 20;
			SetScrollInfo(GetDlgItem(hwndDlg,301),SB_CTL,&si,TRUE);

			//setup font
			hFont = (HFONT)SendMessage(hwndDlg, WM_GETFONT, 0, 0);
			GetObject(hFont, sizeof(LOGFONT), &lf);
			strcpy(lf.lfFaceName,"Courier");
			hNewFont = CreateFontIndirect(&lf);

			SendDlgItemMessage(hwndDlg,300,WM_SETFONT,(WPARAM)hNewFont,FALSE);
			SendDlgItemMessage(hwndDlg,304,WM_SETFONT,(WPARAM)hNewFont,FALSE);
			SendDlgItemMessage(hwndDlg,305,WM_SETFONT,(WPARAM)hNewFont,FALSE);
			SendDlgItemMessage(hwndDlg,306,WM_SETFONT,(WPARAM)hNewFont,FALSE);
			SendDlgItemMessage(hwndDlg,307,WM_SETFONT,(WPARAM)hNewFont,FALSE);
			SendDlgItemMessage(hwndDlg,308,WM_SETFONT,(WPARAM)hNewFont,FALSE);
			SendDlgItemMessage(hwndDlg,309,WM_SETFONT,(WPARAM)hNewFont,FALSE);
			SendDlgItemMessage(hwndDlg,310,WM_SETFONT,(WPARAM)hNewFont,FALSE);
			SendDlgItemMessage(hwndDlg,311,WM_SETFONT,(WPARAM)hNewFont,FALSE);

			//text limits
			SendDlgItemMessage(hwndDlg,304,EM_SETLIMITTEXT,2,0);
			SendDlgItemMessage(hwndDlg,305,EM_SETLIMITTEXT,2,0);
			SendDlgItemMessage(hwndDlg,306,EM_SETLIMITTEXT,2,0);
			SendDlgItemMessage(hwndDlg,307,EM_SETLIMITTEXT,4,0);
			SendDlgItemMessage(hwndDlg,308,EM_SETLIMITTEXT,83,0);
			SendDlgItemMessage(hwndDlg,309,EM_SETLIMITTEXT,4,0);
			SendDlgItemMessage(hwndDlg,310,EM_SETLIMITTEXT,4,0);
			SendDlgItemMessage(hwndDlg,311,EM_SETLIMITTEXT,2,0);

			//I'm lazy, disable the controls which I can't mess with right now
			SendDlgItemMessage(hwndDlg,310,EM_SETREADONLY,TRUE,0);
			SendDlgItemMessage(hwndDlg,311,EM_SETREADONLY,TRUE,0);

			debugger_open = 1;
			FillBreakList(hwndDlg);
			break;
		case WM_CLOSE:
		case WM_QUIT:
			exitdebug:
			debugger_open = 0;
			DeleteObject(hNewFont);
			DestroyWindow(hwndDlg);
			break;
		case WM_MOVING:
			StopSound();
			break;
		case WM_MOVE:
			GetWindowRect(hwndDlg,&wrect);
			DbgPosX = wrect.left;
			DbgPosY = wrect.top;
			break;
		case WM_COMMAND:
			if ((HIWORD(wParam) == BN_CLICKED) && (LOWORD(wParam) == 100)) goto exitdebug;
			break;
	}

	//these messages only get handled when a game is loaded
	if (GI) {
		switch(uMsg) {
			case WM_VSCROLL:
				if (userpause) UpdateRegs(hwndDlg);
				if (lParam) {
					StopSound();
					GetScrollInfo((HWND)lParam,SB_CTL,&si);
					switch(LOWORD(wParam)) {
						case SB_ENDSCROLL:
						case SB_TOP:
						case SB_BOTTOM: break;
						case SB_LINEUP: si.nPos--; break;
						case SB_LINEDOWN:
							if ((tmp=opsize[GetMem(si.nPos)])) si.nPos+=tmp;
							else si.nPos++;
							break;
						case SB_PAGEUP: si.nPos-=si.nPage; break;
						case SB_PAGEDOWN: si.nPos+=si.nPage; break;
						case SB_THUMBPOSITION: //break;
						case SB_THUMBTRACK: si.nPos = si.nTrackPos; break;
					}
					if (si.nPos < si.nMin) si.nPos = si.nMin;
					if ((si.nPos+si.nPage) > si.nMax) si.nPos = si.nMax-si.nPage;
					SetScrollInfo((HWND)lParam,SB_CTL,&si,TRUE);
					Disassemble(hDebug, 300, 301, si.nPos);
				}
				break;

			case WM_MOUSEWHEEL: //just handle page up/down and mousewheel messages together
				i = (short)HIWORD(wParam);///WHEEL_DELTA;
				GetScrollInfo((HWND)lParam,SB_CTL,&si);
				if(i < 0)si.nPos+=si.nPage;
				if(i > 0)si.nPos-=si.nPage;
				if (si.nPos < si.nMin) si.nPos = si.nMin;
				if ((si.nPos+si.nPage) > si.nMax) si.nPos = si.nMax-si.nPage;
				SetScrollInfo((HWND)lParam,SB_CTL,&si,TRUE);
				Disassemble(hDebug, 300, 301, si.nPos);
				break;

			case WM_KEYDOWN:
				MessageBox(hwndDlg,"Die!","I'm dead!",MB_YESNO|MB_ICONINFORMATION);
				//i=0;
				//if(uMsg == WM_KEYDOWN){
				//	if(wParam == VK_PRIOR) i = -1;
				//	if(wParam == VK_NEXT) i = 1;
				//}
				break;

			case WM_MOUSEMOVE:
				mouse_x = GET_X_LPARAM(lParam);
				mouse_y = GET_Y_LPARAM(lParam);
				if ((mouse_x > 8) && (mouse_x < 22) && (mouse_y > 10) && (mouse_y < 282)) {
					if ((tmp=((mouse_y - 10) / 13)) > 19) tmp = 19;
					i = si.nPos;
					while (tmp > 0) {
						if ((tmp2=opsize[GetMem(i)]) == 0) tmp2++;
						if ((i+=tmp2) > 0xFFFF) {
							i = 0xFFFF;
							break;
						}
						tmp--;
					}
					if (i >= 0x8000) {
						dotdot[0] = 0;
						if (!(ptr = iNesShortFName())) ptr = "...";
						if (strlen(ptr) > 20) strcpy(dotdot,"...");
						if (GetNesFileAddress(i) == -1) sprintf(str,"CPU Address $%04X, Error retreiving ROM File Address!",i);
						else sprintf(str,"CPU Address $%04X, Offset 0x%06X in file \"%.20s%s\"",i,GetNesFileAddress(i),ptr,dotdot);
						SetDlgItemText(hwndDlg,502,str);
					}
					else SetDlgItemText(hwndDlg,502,"");
				}
				else SetDlgItemText(hwndDlg,502,"");
				break;
			case WM_LBUTTONDOWN:
				mouse_x = GET_X_LPARAM(lParam);
				mouse_y = GET_Y_LPARAM(lParam);
				if ((userpause > 0) && (mouse_x > 8) && (mouse_x < 22) && (mouse_y > 10) && (mouse_y < 282)) {
					if ((tmp=((mouse_y - 10) / 13)) > 19) tmp = 19;
					i = si.nPos;
					while (tmp > 0) {
						if ((tmp2=opsize[GetMem(i)]) == 0) tmp2++;
						if ((i+=tmp2) > 0xFFFF) {
							i = 0xFFFF;
							break;
						}
						tmp--;
					}
					//DoPatcher(GetNesFileAddress(i),hwndDlg);
					iaPC=i;
					if (iaPC >= 0x8000) {
						DialogBox(fceu_hInstance,"ASSEMBLER",hwndDlg,AssemblerCallB);
						UpdateDebugger();
					}
				}
				break;
			case WM_RBUTTONDOWN:
				mouse_x = GET_X_LPARAM(lParam);
				mouse_y = GET_Y_LPARAM(lParam);
				if ((userpause > 0) && (mouse_x > 8) && (mouse_x < 22) && (mouse_y > 10) && (mouse_y < 282)
					&& (hMemView)) {
					if ((tmp=((mouse_y - 10) / 13)) > 19) tmp = 19;
					i = si.nPos;
					while (tmp > 0) {
						if ((tmp2=opsize[GetMem(i)]) == 0) tmp2++;
						if ((i+=tmp2) > 0xFFFF) {
							i = 0xFFFF;
							break;
						}
						tmp--;
					}
					ChangeMemViewFocus(2,GetNesFileAddress(i));
					//iaPC=i;
					//if (iaPC >= 0x8000) {
					//	DialogBox(fceu_hInstance,"ASSEMBLER",hwndDlg,AssemblerCallB);
					//	UpdateDebugger();
					//}
				}  else {StopSound();}
				break;
			case WM_INITMENUPOPUP:
			case WM_INITMENU:
//			case WM_ENTERIDLE:
				StopSound();
				break;
			case WM_COMMAND:
				switch(HIWORD(wParam)) {
					case BN_CLICKED:
						switch(LOWORD(wParam)) {
							case 101: //Add
								StopSound();
								childwnd = 1;
								if (DialogBox(fceu_hInstance,"ADDBP",hwndDlg,AddbpCallB)) AddBreakList();
								childwnd = 0;
								UpdateDebugger();
								break;
							case 102: //Delete
								DeleteBreak(SendDlgItemMessage(hwndDlg,302,LB_GETCURSEL,0,0));
								break;
							case 103: //Edit
								StopSound();
								WP_edit = SendDlgItemMessage(hwndDlg,302,LB_GETCURSEL,0,0);
								if (DialogBox(fceu_hInstance,"ADDBP",hwndDlg,AddbpCallB)) EditBreakList();
								WP_edit = -1;
								UpdateDebugger();
								break;
							case 104: //Run
								if (userpause > 0) {
									UpdateRegs(hwndDlg);
									userpause = 0;
									UpdateDebugger();
								}
								break;
							case 105: //Step Into
								if (userpause > 0)
									UpdateRegs(hwndDlg);
								step = 1;
								userpause = 0;
								UpdateDebugger();
								break;
							case 106: //Step Out
								if (userpause > 0) {
									UpdateRegs(hwndDlg);
									if ((stepout) && (MessageBox(hwndDlg,"Step Out is currently in process. Cancel it and setup a new Step Out watch?","Step Out Already Active",MB_YESNO|MB_ICONINFORMATION) != IDYES)) break;
									if (GetMem(X.PC) == 0x20) jsrcount = 1;
									else jsrcount = 0;
									stepout = 1;
									userpause = 0;
									UpdateDebugger();
								}
								break;
							case 107: //Step Over
								if (userpause) {
									UpdateRegs(hwndDlg);
									if (GetMem(tmp=X.PC) == 0x20) {
										if ((watchpoint[64].flags) && (MessageBox(hwndDlg,"Step Over is currently in process. Cancel it and setup a new Step Over watch?","Step Over Already Active",MB_YESNO|MB_ICONINFORMATION) != IDYES)) break;
										watchpoint[64].address = (tmp+3);
										watchpoint[64].flags = WP_E|WP_X;
									}
									else step = 1;
									userpause = 0;
								}
								break;
							case 108: //Seek PC
								if (userpause) {
									UpdateRegs(hwndDlg);
									UpdateDebugger();
								}
								break;
							case 109: //Seek To:
								if (userpause) UpdateRegs(hwndDlg);
								GetDlgItemText(hwndDlg,309,str,5);
								if (((ret = sscanf(str,"%4X",&tmp)) == EOF) || (ret != 1)) tmp = 0;
								sprintf(str,"%04X",tmp);
								SetDlgItemText(hwndDlg,309,str);
								Disassemble(hDebug, 300, 301, tmp);
								break;

							case 200: X.P^=N_FLAG; UpdateDebugger(); break;
							case 201: X.P^=V_FLAG; UpdateDebugger(); break;
							case 202: X.P^=U_FLAG; UpdateDebugger(); break;
							case 203: X.P^=B_FLAG; UpdateDebugger(); break;
							case 204: X.P^=D_FLAG; UpdateDebugger(); break;
							case 205: X.P^=I_FLAG; UpdateDebugger(); break;
							case 206: X.P^=Z_FLAG; UpdateDebugger(); break;
							case 207: X.P^=C_FLAG; UpdateDebugger(); break;

							case 602: DoPatcher(-1,hwndDlg); break;
							//case 603: DoTracer(hwndDlg); break;
						}
						//UpdateDebugger();
						break;
					case LBN_DBLCLK:
						switch(LOWORD(wParam)) {
							case 302: EnableBreak(SendDlgItemMessage(hwndDlg,302,LB_GETCURSEL,0,0)); break;
						}
						break;
					case LBN_SELCANCEL:
						switch(LOWORD(wParam)) {
							case 302:
								EnableWindow(GetDlgItem(hwndDlg,102),FALSE);
								EnableWindow(GetDlgItem(hwndDlg,103),FALSE);
								break;
						}
						break;
					case LBN_SELCHANGE:
						switch(LOWORD(wParam)) {
							case 302:
								EnableWindow(GetDlgItem(hwndDlg,102),TRUE);
								EnableWindow(GetDlgItem(hwndDlg,103),TRUE);
								break;
						}
						break;
				}
				break;/*
				default:
				if(
				(uMsg == 312) ||
				(uMsg == 309) ||
				(uMsg == 308) ||
				(uMsg == 307) ||
				(uMsg == 311) ||
				(uMsg == 71) ||
				(uMsg == 310) ||
				(uMsg == 20) ||
				(uMsg == 13) ||
				(uMsg == 133) ||
				(uMsg == 70) ||
				(uMsg == 24) ||
				(uMsg == 296) ||
				(uMsg == 295) ||
				(uMsg == 15) ||
				(uMsg == 272) ||
				(uMsg == 49) ||
				(uMsg == 3)
				)break;*/

					/*
				if(skipdebug)break;
				if(
				(uMsg == 32) ||
				(uMsg == 127))break;
				fp = fopen("debug.txt","a");
				u++;
				sprintf(str,"%d = %06d",u,uMsg);
				skipdebug=1;
				SetWindowText(hDebug,str);
				skipdebug=0;
				fprintf(fp,"%s\n",str);
				fclose(fp);
				break;*/
		}
	}


	return FALSE; //TRUE;
}

extern void iNESGI(int h);

void DoPatcher(int address,HWND hParent){
	StopSound();
	iapoffset=address;
	if(GameInterface==iNESGI)DialogBox(fceu_hInstance,"ROMPATCHER",hParent,PatcherCallB);
	else MessageBox(hDebug, "Sorry, The Patcher only works on INES rom images", "Error", MB_OK);
	UpdateDebugger();
}

void UpdatePatcher(HWND hwndDlg){
	unsigned char str[75];
	uint8 *p;
	if(iapoffset != -1){
		EnableWindow(GetDlgItem(hwndDlg,109),TRUE);
		EnableWindow(GetDlgItem(hwndDlg,110),TRUE);

		if(GetRomAddress(iapoffset) != -1)sprintf(str,"Current Data at NES ROM Address: %04X, .NES file Address: %04X",GetRomAddress(iapoffset),iapoffset);
		else sprintf(str,"Current Data at .NES file Address: %04X",iapoffset);

		SetDlgItemText(hwndDlg,104,str);

		sprintf(str,"%04X",GetRomAddress(iapoffset));
		SetDlgItemText(hwndDlg,107,str);

		if(GetRomAddress(iapoffset) != -1)SetDlgItemText(hwndDlg,107,DisassembleLine(GetRomAddress(iapoffset)));
		else SetDlgItemText(hwndDlg,107,"Not Currently Loaded in ROM for disassembly");

		p = GetNesPRGPointer(iapoffset-16);
		sprintf(str,"%02X %02X %02X %02X %02X %02X %02X %02X",
			p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]);
		SetDlgItemText(hwndDlg,105,str);

	} else {
		SetDlgItemText(hwndDlg,104,"No Offset Selected");
		SetDlgItemText(hwndDlg,105,"");
		SetDlgItemText(hwndDlg,107,"");
		EnableWindow(GetDlgItem(hwndDlg,109),FALSE);
		EnableWindow(GetDlgItem(hwndDlg,110),FALSE);
	}
	if(FCEUGameInfo.type != GIT_CART)EnableWindow(GetDlgItem(hwndDlg,111),FALSE);
	else EnableWindow(GetDlgItem(hwndDlg,111),TRUE);
}

void DoDebug(uint8 halt) {
	if (!debugger_open) hDebug = CreateDialog(fceu_hInstance,"DEBUGGER",NULL,DebuggerCallB);
	if (hDebug) {
		SetWindowPos(hDebug,HWND_TOP,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE|SWP_NOOWNERZORDER);
		if (GI) UpdateDebugger();
	}
}
