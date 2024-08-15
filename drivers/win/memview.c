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
#include "..\..\types.h"
#include "..\..\memview.h"
#include "..\..\debugger.h"
#include "..\..\cdlogger.h"
#include "..\..\fce.h"
#include "..\..\cheat.h"
#include "..\..\cart.h"
#include "..\..\ines.h"
#include "cheat.h"

#define MODE_NES_MEMORY   0
#define MODE_NES_PPU      1
#define MODE_NES_FILE     2

int LoadTableFile();
void InputData(unsigned char input);
int GetMemViewData(int i);
void UpdateCaption();
int UpdateCheatColorCallB(char *name, uint32 a, uint8 v, int compare,int s,int type);
int DeleteCheatCallB(char *name, uint32 a, uint8 v, int compare,int s,int type);
int GetHexScreenCoordx(int offset);
int GetHexScreenCoordy(int offset);
int GetAddyFromCoord(int x,int y);
LRESULT CALLBACK MemViewCallB(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

HWND hMemView;
HDC mDC;
//int tempdummy;
//char dummystr[100];
HFONT hMemFont;
int CurOffset;
int MemFontHeight;
int MemFontWidth;
int ClientHeight;
int NoColors;
int EditingMode;
int EditingText;
int AddyWasText; //used by the GetAddyFromCoord() function.

char chartable[256];

//SCROLLINFO memsi;
//HBITMAP HDataBmp;
//HGDIOBJ HDataObj;
HDC HDataDC;
int CursorX=2, CursorY=9;
int CursorStartAddy, CursorEndAddy=-1;
//int CursorStartNibble=1, CursorEndNibble; //1 means that only half of the byte is selected
int TempData=-1;
int DataAmount;
int MaxSize;

COLORREF *BGColorList;
COLORREF *TextColorList;
int *OldValues; //this will be used for a speed hack
int OldCurOffset;

int lbuttondown, lbuttondownx, lbuttondowny;
int mousex, mousey;

extern iNES_HEADER head;

//undo structure
struct UNDOSTRUCT {
	int addr;
	int size;
	unsigned char *data;
	struct UNDOSTRUCT *last;
};

struct UNDOSTRUCT *undo_list=0;

void ApplyPatch(int addr,int size, uint8* data){
	struct UNDOSTRUCT *tmp=malloc(sizeof(struct UNDOSTRUCT));

	int i;

	//while(tmp != 0){tmp=tmp->next;x++;};
	//tmp = malloc(sizeof(struct UNDOSTRUCT));
	//sprintf(str,"%d",x);
	//MessageBox(hMemView,str,"info", MB_OK);
	tmp->addr = addr;
	tmp->size = size;
	tmp->data = malloc(sizeof(uint8)*size);
	tmp->last=undo_list;

	for(i = 0;i < size;i++){
		tmp->data[i] = GetFileData(addr+i);
		WriteFileData(addr+i,data[i]);
	}

	undo_list=tmp;

	UpdateColorTable();
	return;
}

void UndoLastPatch(){
	struct UNDOSTRUCT *tmp=undo_list;
	int i;
	if(undo_list == 0)return;
	//while(tmp->next != 0){tmp=tmp->next;}; //traverse to the one before the last one

	for(i = 0;i < tmp->size;i++){
		WriteFileData(tmp->addr+i,tmp->data[i]);
	}
	
	undo_list=undo_list->last;

	free(tmp->data);
	free(tmp);
	UpdateColorTable();
	return;
}

void FlushUndoBuffer(){
	struct UNDOSTRUCT *tmp;
	while(undo_list!= 0){
		tmp=undo_list;
		undo_list=undo_list->last;
		free(tmp->data);
		free(tmp);
	}
	UpdateColorTable();
	return;
}


int GetFileData(int offset){
	if(offset < 16) return *((unsigned char *)&head+offset);
	if(offset < 16+PRGsize[0])return PRGptr[0][offset-16];
	if(offset < 16+PRGsize[0]+CHRsize[0])return CHRptr[0][offset-16-PRGsize[0]];
	return -1;
}

int WriteFileData(int addr,int data){
	if (addr < 16)MessageBox(hMemView,"Sorry", "Go bug bbit if you really want to edit the header.", MB_OK);
	if((addr >= 16) && (addr < PRGsize[0]+16)) *(uint8 *)(GetNesPRGPointer(addr-16)) = data;
	if((addr >= PRGsize[0]+16) && (addr < CHRsize[0]+PRGsize[0]+16)) *(uint8 *)(GetNesCHRPointer(addr-16-PRGsize[0])) = data;

	return 0;
}

int GetRomFileSize(){ //todo: fix or remove this?
	return 0;
}

//should return -1, otherwise returns the line number it had the error on
int LoadTableFile(){
	char str[50];
	FILE *FP;
	int i, j, line, charcode1, charcode2;

	const char filter[]="Table Files (*.TBL)\0*.tbl\0";
	char nameo[2048]; //todo: possibly no need for this? can lpstrfilter point to loadedcdfile instead?
	OPENFILENAME ofn;
	StopSound();
	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize=sizeof(ofn);
	ofn.hInstance=fceu_hInstance;
	ofn.lpstrTitle="Load Table File...";
	ofn.lpstrFilter=filter;
	nameo[0]=0;
	ofn.lpstrFile=nameo;
	ofn.nMaxFile=256;
	ofn.Flags=OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY;
	ofn.hwndOwner = hCDLogger;
	if(!GetOpenFileName(&ofn))return -1;

	for(i = 0;i < 256;i++){
		chartable[i] = 0;
	}

	FP = fopen(nameo,"r");
	line = 0;
	while((fgets(str, 50, FP)) != NULL){/* get one line from the file */
		line++;
		//sprintf(str3,"str = '%s', str2 = '%s'",str,str2);
		//MessageBox(hMemView,str3,"debug1", MB_OK);
		
		/*for(j = 0,i = 0;str[j] != 0;j++){ //remove whitespace
			if(str[j] != ' '){
				str2[i] = str[j];
				i++;
			}
		}*/ //don't delete whitespace for now
		//strcpy(str2,str);

		//str[i] = 0;

		charcode1 = charcode2 = -1;

		if((str[0] >= 'a') && (str[0] <= 'f')) charcode1 = str[0]-('a'-0xA);
		if((str[0] >= 'A') && (str[0] <= 'F')) charcode1 = str[0]-('A'-0xA);
		if((str[0] >= '0') && (str[0] <= '9')) charcode1 = str[0]-'0';

		if((str[1] >= 'a') && (str[1] <= 'f')) charcode2 = str[1]-('a'-0xA);
		if((str[1] >= 'A') && (str[1] <= 'F')) charcode2 = str[1]-('A'-0xA);
		if((str[1] >= '0') && (str[1] <= '9')) charcode2 = str[1]-'0';
		
		//sprintf(str3,"str = '%s', str2 = '%s'",str,str2);
		//MessageBox(hMemView,str3,"debug2", MB_OK);
		
		if(charcode1 == -1){
			for(i = 0;i < 256;i++){
				chartable[i] = i;
			}
			fclose(FP);
			return line; //we have an error getting the first input
		}

		if(charcode2 != -1) charcode1 = (charcode1<<4)|charcode2;

		for(i = 0;i < strlen(str);i++)if(str[i] == '=')break;

		if(i == strlen(str)){
			for(i = 0;i < 256;i++){
				chartable[i] = i;
			}
			fclose(FP);
			return line; //error no '=' found
		}

		i++;
		if((tolower(str[i]) == 'r') && (tolower(str[i+1]) == 'e') && (tolower(str[i+2]) == 't'))
			charcode2 = 0x0D;
		else charcode2 = str[i];

		chartable[charcode1] = charcode2;
	}
	fclose(FP);
	return -1;

}

void UpdateMemoryView(int draw_all){
	int i, j;
	//LPVOID lpMsgBuf;
	//int curlength;
	char str[100];
	char str2[100];
	if (!hMemView) return;

/*
	if(draw_all){
		for(i = CurOffset;i < CurOffset+DataAmount;i+=16){
			MoveToEx(HDataDC,0,MemFontHeight*((i-CurOffset)/16),NULL);
			sprintf(str,"%06X: ",i);
			for(j = 0;j < 16;j++){
				sprintf(str2,"%02X ",GetMem(i+j));
				strcat(str,str2);
			}
			strcat(str," : ");
			k = strlen(str);
			for(j = 0;j < 16;j++){
				str[k+j] = GetMem(i+j);
				if(str[k+j] < 0x20)str[k+j] = 0x2E;
				if(str[k+j] > 0x7e)str[k+j] = 0x2E;
			}
			str[k+16] = 0;
			TextOut(HDataDC,0,0,str,strlen(str));
		}
	} else {*/
	for(i = CurOffset;i < CurOffset+DataAmount;i+=16){
		if((OldCurOffset != CurOffset) || draw_all){
			MoveToEx(HDataDC,0,MemFontHeight*((i-CurOffset)/16),NULL);
			SetTextColor(HDataDC,RGB(0,0,0));
			SetBkColor(HDataDC,RGB(255,255,255));
			sprintf(str,"%06X: ",i);
			TextOut(HDataDC,0,0,str,strlen(str));
		}
		for(j = 0;j < 16;j++){
			if((CursorEndAddy == -1) && (CursorStartAddy == i+j)){ //print up single highlighted text
				sprintf(str,"%02X",GetMemViewData(CursorStartAddy));
				OldValues[i+j-CurOffset] = -1; //set it to redraw this one next time
				MoveToEx(HDataDC,8*MemFontWidth+(j*3*MemFontWidth),MemFontHeight*((i-CurOffset)/16),NULL);
				if(TempData != -1){
					sprintf(str2,"%X",TempData);
					SetBkColor(HDataDC,RGB(255,255,255));
					SetTextColor(HDataDC,RGB(255,0,0));
					TextOut(HDataDC,0,0,str2,1);
					SetTextColor(HDataDC,RGB(255,255,255));
					SetBkColor(HDataDC,RGB(0,0,0));
					TextOut(HDataDC,0,0,&str[1],1);
				} else {
					SetTextColor(HDataDC,RGB(255,255,255));
					SetBkColor(HDataDC,RGB(0,0,0));
					TextOut(HDataDC,0,0,str,1);
					SetTextColor(HDataDC,RGB(0,0,0));
					SetBkColor(HDataDC,RGB(255,255,255));
					TextOut(HDataDC,0,0,&str[1],1);
				}
				TextOut(HDataDC,0,0," ",1);

				SetTextColor(HDataDC,RGB(255,255,255));
				SetBkColor(HDataDC,RGB(0,0,0));
				MoveToEx(HDataDC,(59+j)*MemFontWidth,MemFontHeight*((i-CurOffset)/16),NULL); //todo: try moving this above the for loop
				str[0] = chartable[GetMemViewData(i+j)];
				if(str[0] < 0x20)str[0] = 0x2E;
				if(str[0] > 0x7e)str[0] = 0x2E;
				str[1] = 0;
				TextOut(HDataDC,0,0,str,1);

				continue;
			}
			if((OldValues[i+j-CurOffset] != GetMemViewData(i+j)) || draw_all){
				MoveToEx(HDataDC,8*MemFontWidth+(j*3*MemFontWidth),MemFontHeight*((i-CurOffset)/16),NULL);
				SetTextColor(HDataDC,TextColorList[i+j-CurOffset]);//(8+j*3)*MemFontWidth
				SetBkColor(HDataDC,BGColorList[i+j-CurOffset]);
				sprintf(str,"%02X ",GetMemViewData(i+j));
				TextOut(HDataDC,0,0,str,strlen(str));

				MoveToEx(HDataDC,(59+j)*MemFontWidth,MemFontHeight*((i-CurOffset)/16),NULL); //todo: try moving this above the for loop
				str[0] = chartable[GetMemViewData(i+j)];
				if(str[0] < 0x20)str[0] = 0x2E;
				if(str[0] > 0x7e)str[0] = 0x2E;
				str[1] = 0;
				TextOut(HDataDC,0,0,str,1);
				if(CursorStartAddy != i+j)OldValues[i+j-CurOffset] = GetMemViewData(i+j);
			}
		}
		if(draw_all){
			MoveToEx(HDataDC,56*MemFontWidth,MemFontHeight*((i-CurOffset)/16),NULL);
			SetTextColor(HDataDC,RGB(0,0,0));
			SetBkColor(HDataDC,RGB(255,255,255));
			TextOut(HDataDC,0,0," : ",3);
		}/*
		for(j = 0;j < 16;j++){
			if((OldValues[i+j-CurOffset] != GetMem(i+j)) || draw_all){
				MoveToEx(HDataDC,(59+j)*MemFontWidth,MemFontHeight*((i-CurOffset)/16),NULL); //todo: try moving this above the for loop
				SetTextColor(HDataDC,TextColorList[i+j-CurOffset]);
				SetBkColor(HDataDC,BGColorList[i+j-CurOffset]);
				str[0] = GetMem(i+j);
				if(str[0] < 0x20)str[0] = 0x2E;
				if(str[0] > 0x7e)str[0] = 0x2E;
				str[1] = 0;
				TextOut(HDataDC,0,0,str,1);
				if(CursorStartAddy != i+j)OldValues[i+j-CurOffset] = GetMem(i+j);
			}
		}*/
	}
//	}

	SetTextColor(HDataDC,RGB(0,0,0));
	SetBkColor(HDataDC,RGB(255,255,255));

	MoveToEx(HDataDC,0,0,NULL);	
	OldCurOffset = CurOffset;
	return;
}

void UpdateCaption(){
	char str[50];
	char EditString[3][20] = {"RAM","PPU Memory","ROM"};

	if(CursorEndAddy == -1){
		sprintf(str,"Hex Editor - Editing %s Offset 0x%06x",EditString[EditingMode],CursorStartAddy);
	} else {
		sprintf(str,"Hex Editor - Editing %s Offset 0x%06x - 0x%06x, 0x%x bytes selected ",
			EditString[EditingMode],CursorStartAddy,CursorEndAddy,CursorEndAddy-CursorStartAddy);
	}
	SetWindowText(hMemView,str);
	return;
}

int GetMemViewData(int i){
	if(EditingMode == 0)return GetMem(i);
	if(EditingMode == 1)return VPage[(i)>>10][(i)];
	if(EditingMode == 2){ //todo: use getfiledata() here
		if(i < 16) return *((unsigned char *)&head+i);
		if(i < 16+PRGsize[0])return PRGptr[0][i-16];
		if(i < 16+PRGsize[0]+CHRsize[0])return CHRptr[0][i-16-PRGsize[0]];
	}
	return 0;
}

void UpdateColorTable(){
	struct UNDOSTRUCT *tmp;
	int i;
	if(!hMemView)return;
	for(i = 0;i < DataAmount;i++){
		if((i+CurOffset >= CursorStartAddy) && (i+CurOffset <= CursorEndAddy)){
			BGColorList[i] = RGB(0,0,0);
			TextColorList[i] = RGB(255,255,255);
			continue;
		}

		BGColorList[i] = RGB(255,255,255);
		TextColorList[i] = RGB(0,0,0);
	}
	
	if(EditingMode == 0)FCEUI_ListCheats(UpdateCheatColorCallB);

	if(EditingMode == 2){
		if(cdloggerdata) {
			for(i = 0;i < DataAmount;i++){
				if((CurOffset+i >= 16) && (CurOffset+i < 16+PRGsize[0])) {
					//if((cdloggerdata[i+iCurOffset-16]&3) == 0)TextColorList[i]=RGB(256,192,0);
					if((cdloggerdata[i+CurOffset-16]&3) == 1)TextColorList[i]=RGB(192,192,0);
					if((cdloggerdata[i+CurOffset-16]&3) == 2)TextColorList[i]=RGB(0,0,192);
				}
			}
		}

		tmp=undo_list;
		while(tmp!= 0){
			//if((tmp->addr < CurOffset+DataAmount) && (tmp->addr+tmp->size > CurOffset))
				for(i = tmp->addr;i < tmp->addr+tmp->size;i++){
					if((i > CurOffset) && (i < CurOffset+DataAmount))
						TextColorList[i-CurOffset] = RGB(255,0,0);
				}
			tmp=tmp->last;
		}
	}

	UpdateMemoryView(1); //anytime the colors change, the memory viewer needs to be completely redrawn
}

int UpdateCheatColorCallB(char *name, uint32 a, uint8 v, int compare,int s,int type) {
if((a > CurOffset) && (a < CurOffset+DataAmount))TextColorList[a-CurOffset] = RGB(0,0,255);
return 1;
}

int addrtodelete;    // This is a very ugly hackish method of doing this
int cheatwasdeleted; // but it works and that is all that matters here.
int DeleteCheatCallB(char *name, uint32 a, uint8 v, int compare,int s,int type){
	if(cheatwasdeleted == -1)return 1;
	cheatwasdeleted++;
	if(a == addrtodelete){
		FCEUI_DelCheat(cheatwasdeleted-1);
		cheatwasdeleted = -1;
		return 0;
	}
	return 1;
}

//input is expected to be an ASCII value
void InputData(unsigned char input){
	//CursorEndAddy = -1;
	int data, addr, i;
	//char str3[500];
	//sprintf(str3,"%02X",input);
	//MessageBox(hMemView,str3,"debug1", MB_OK);
	//return;

	if(!EditingText){
		if((input >= 'a') && (input <= 'f')) input-=('a'-0xA);
		if((input >= 'A') && (input <= 'F')) input-=('A'-0xA);
		if((input >= '0') && (input <= '9')) input-='0';
		if(input > 0xF)return;

		if(TempData != -1){
			addr = CursorStartAddy;
			data = input|(TempData<<4);
			if(EditingMode == 0)BWrite[addr](addr,data);
			if(EditingMode == 2)ApplyPatch(addr,1,(uint8 *)&data);
			CursorStartAddy++;
			TempData = -1;
		} else {
			TempData = input;
		}
	} else {
		for(i = 0;i < 256;i++)if(chartable[i] == input)break;
		if(i == 256)return;

		addr = CursorStartAddy;
		data = i;
		if(EditingMode == 0)BWrite[addr](addr,data);
		if(EditingMode == 2)ApplyPatch(addr,1,(uint8 *)&data);
		CursorStartAddy++;
	}
	return;
}

void ChangeMemViewFocus(int EditingMode, int Offset){
	SCROLLINFO si;
	if(!hMemView)return;
	MemViewCallB(hMemView,WM_COMMAND,300+EditingMode,0); //let the window handler do it for us
	CursorEndAddy = -1;
	CursorStartAddy = Offset;
	CurOffset = (Offset/16)*16;
	if(Offset > MaxSize)return; //this should never happen

	ZeroMemory(&si, sizeof(SCROLLINFO));
	si.fMask = SIF_POS;
	si.cbSize = sizeof(SCROLLINFO);
	si.nPos = CurOffset/16;
	SetScrollInfo(hMemView,SB_VERT,&si,TRUE);
	UpdateColorTable();
	return;
}


int GetHexScreenCoordx(int offset){
	return (8*MemFontWidth)+((offset%16)*3*MemFontWidth); //todo: add Curoffset to this and to below function
}

int GetHexScreenCoordy(int offset){
	return (offset/16)*MemFontHeight;
}

//0000E0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  : ................

//if the mouse is in the text field, this function will set AddyWasText to 1 otherwise it is 0
//if the mouse wasn't in any range, this function returns -1
int GetAddyFromCoord(int x,int y){
	if((y < 0) || (y > DataAmount*MemFontHeight)) return -1;

	if((x > 8*MemFontWidth) && (x < 55*MemFontWidth)){
		AddyWasText = 0;
		return ((y/MemFontHeight)*16)+((x-(8*MemFontWidth))/(3*MemFontWidth))+CurOffset;
	}

	if((x > 59*MemFontWidth) && (x < 75*MemFontWidth)){
		AddyWasText = 1;
		//MessageBeep(MB_OK);
		return ((y/MemFontHeight)*16)+((x-(59*MemFontWidth))/(MemFontWidth))+CurOffset;
	}
	
	return -1;
}

void KillMemView(){
	DeleteObject(hMemFont);
	ReleaseDC(hMemView,mDC);
	DestroyWindow(hMemView);
	UnregisterClass("MEMVIEW",fceu_hInstance);
	hMemView = 0;
	return;
}

LRESULT CALLBACK MemViewCallB(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam){
	HDC          hdc;
	PAINTSTRUCT ps ;
	TEXTMETRIC tm;
	SCROLLINFO si;
	int x, y, i;

	char str[100];

	switch (message) {
    //case WM_NCRBUTTONDOWN:
    //case WM_NCMBUTTONDOWN:
    //case WM_NCLBUTTONDOWN:
		case WM_ENTERMENULOOP:StopSound();return 0;
		case WM_INITMENUPOPUP:
			if(undo_list != 0)EnableMenuItem(GetMenu(hMemView),200,MF_BYCOMMAND | MF_ENABLED);
			else EnableMenuItem(GetMenu(hMemView),200,MF_BYCOMMAND | MF_GRAYED);
		return 0;

		case WM_CREATE:
			mDC = GetDC(hwnd);
			HDataDC = mDC;//deleteme
			hMemFont = CreateFont(13,8, /*Height,Width*/
				0,0, /*escapement,orientation*/
				400,FALSE,FALSE,FALSE, /*weight, italic,, underline, strikeout*/
				ANSI_CHARSET,OUT_DEVICE_PRECIS,CLIP_MASK, /*charset, precision, clipping*/
				DEFAULT_QUALITY, DEFAULT_PITCH, /*quality, and pitch*/
				"Courier"); /*font name*/
			SelectObject (HDataDC, hMemFont);
			SetTextAlign(HDataDC,TA_UPDATECP | TA_TOP | TA_LEFT);

			GetTextMetrics (HDataDC, &tm) ;
			MemFontWidth = 8;
			MemFontHeight = 13;

			MaxSize = 0x10000;
			//Allocate Memory for color lists
			DataAmount = 0x100;
			TextColorList = malloc(DataAmount*sizeof(COLORREF));
			BGColorList = malloc(DataAmount*sizeof(COLORREF));
			OldValues = malloc(DataAmount*sizeof(int));
			EditingText = EditingMode = 0;
			
			//set the default table
			for(i = 0;i < 256;i++){
				chartable[i] = i;
			}
			UpdateColorTable(); //draw it
			return 0;
		case WM_PAINT:
			hdc = BeginPaint(hwnd, &ps);
			EndPaint(hwnd, &ps);
			UpdateMemoryView(1);
			return 0;
		case WM_VSCROLL:
			
				StopSound();
				ZeroMemory(&si, sizeof(SCROLLINFO));
				si.fMask = SIF_ALL;
				si.cbSize = sizeof(SCROLLINFO);
				GetScrollInfo(hwnd,SB_VERT,&si);
				switch(LOWORD(wParam)) {
					case SB_ENDSCROLL:
					case SB_TOP:
					case SB_BOTTOM: break;
					case SB_LINEUP: si.nPos--; break;
					case SB_LINEDOWN:si.nPos++; break;
					case SB_PAGEUP: si.nPos-=si.nPage; break;
					case SB_PAGEDOWN: si.nPos+=si.nPage; break;
					case SB_THUMBPOSITION: //break;
					case SB_THUMBTRACK: si.nPos = si.nTrackPos; break;
				}
				if (si.nPos < si.nMin) si.nPos = si.nMin;
				if ((si.nPos+si.nPage) > si.nMax) si.nPos = si.nMax-si.nPage;
				CurOffset = si.nPos*16;
				SetScrollInfo(hwnd,SB_VERT,&si,TRUE);
				UpdateColorTable();
			return 0;
		case WM_CHAR:
				InputData(wParam);
				UpdateColorTable();
				UpdateCaption();
			return 0;

		case WM_KEYDOWN:
				//if((wParam >= 0x30) && (wParam <= 0x39))InputData(wParam-0x30);
				//if((wParam >= 0x41) && (wParam <= 0x46))InputData(wParam-0x41+0xA);

				if(wParam == VK_LEFT)CursorStartAddy--;
				if(wParam == VK_RIGHT)CursorStartAddy++;
				if(wParam == VK_UP)CursorStartAddy-=16;
				if(wParam == VK_DOWN)CursorStartAddy+=16;

				
				if((wParam == 0x5a) && (GetKeyState(VK_CONTROL) & 0x8000)){ //if ctrl+z is pressed
					UndoLastPatch();
					return 0;
				}

				if(CursorStartAddy < 0)CursorStartAddy = 0;
				if(CursorStartAddy > MaxSize)CursorStartAddy = MaxSize; //todo: fix this up when I add support for editing more stuff

				if((wParam == VK_DOWN) || (wParam == VK_UP) ||
					(wParam == VK_RIGHT) || (wParam == VK_LEFT)){
					CursorEndAddy = -1;
					TempData = -1;
					if(CursorStartAddy < CurOffset) CurOffset = (CursorStartAddy/16)*16;
					if(CursorStartAddy > CurOffset+DataAmount-0x10)CurOffset = ((CursorStartAddy-DataAmount+0x10)/16)*16;
				}

				
				if(wParam == VK_PRIOR)CurOffset-=DataAmount;
				if(wParam == VK_NEXT)CurOffset+=DataAmount;
				if(CurOffset < 0)CurOffset = 0;
				if(CurOffset > MaxSize)CurOffset = MaxSize;
			/*
				if((wParam == VK_PRIOR) || (wParam == VK_NEXT)){
					ZeroMemory(&si, sizeof(SCROLLINFO));
					si.fMask = SIF_ALL;
					si.cbSize = sizeof(SCROLLINFO);
					GetScrollInfo(hwnd,SB_VERT,&si);
					if(wParam == VK_PRIOR)si.nPos-=si.nPage;
					if(wParam == VK_NEXT)si.nPos+=si.nPage;
					if (si.nPos < si.nMin) si.nPos = si.nMin;
					if ((si.nPos+si.nPage) > si.nMax) si.nPos = si.nMax-si.nPage;
					CurOffset = si.nPos*16;
				}
				*/
				
				//This updates the scroll bar to curoffset
				ZeroMemory(&si, sizeof(SCROLLINFO));
				si.fMask = SIF_POS;
				si.cbSize = sizeof(SCROLLINFO);
				si.nPos = CurOffset/16;
				SetScrollInfo(hwnd,SB_VERT,&si,TRUE);
				UpdateColorTable();
				UpdateCaption();
				return 0;
		case WM_LBUTTONDOWN:
				lbuttondown = 1;
				SetCapture(hwnd);
				x = GET_X_LPARAM(lParam);
				y = GET_Y_LPARAM(lParam);
				if((i = GetAddyFromCoord(x,y)) == -1)return 0;
				EditingText = AddyWasText;
				lbuttondownx = x;
				lbuttondowny = y;
				CursorStartAddy = i;
				CursorEndAddy = -1;
				UpdateCaption();
				UpdateColorTable();
				return 0;
		case WM_MOUSEMOVE:
				mousex = x = GET_X_LPARAM(lParam); 
				mousey = y = GET_Y_LPARAM(lParam); 
				if(lbuttondown){
					CursorEndAddy = GetAddyFromCoord(x,y);
					EditingText = AddyWasText;
					CursorStartAddy = GetAddyFromCoord(lbuttondownx,lbuttondowny);
					//the below reverses them if they're not right
					if(CursorEndAddy == CursorStartAddy)CursorEndAddy = -1;
					if((CursorEndAddy < CursorStartAddy) && (CursorEndAddy != -1)){ 
						i = CursorStartAddy;
						CursorStartAddy = CursorEndAddy;
						CursorEndAddy = i;
					}
					UpdateCaption();
					UpdateColorTable();
				}
				return 0;
		case WM_LBUTTONUP:
				lbuttondown = 0;
				if(CursorEndAddy == CursorStartAddy)CursorEndAddy = -1;
				if((CursorEndAddy < CursorStartAddy) && (CursorEndAddy != -1)){ //this reverses them if they're not right
					i = CursorStartAddy;
					CursorStartAddy = CursorEndAddy;
					CursorEndAddy = i;
				}
				UpdateCaption();
				UpdateColorTable();
				ReleaseCapture();
				return 0;
		case WM_RBUTTONDOWN:
				x = GET_X_LPARAM(lParam);
				y = GET_Y_LPARAM(lParam);
				i = GetAddyFromCoord(x,y);
				if(i == -1)return 0;
				if(EditingMode == 0){
					addrtodelete = i;
					cheatwasdeleted = 0;
					FCEUI_ListCheats(DeleteCheatCallB);
					if(cheatwasdeleted != -1)FCEUI_AddCheat("",i,GetMem(i),-1,1);
					if(hCheat)RedoCheatsLB(hCheat);
					UpdateColorTable();
				}
				return 0;
		case WM_MOUSEWHEEL:
				i = (short)HIWORD(wParam);///WHEEL_DELTA;
				ZeroMemory(&si, sizeof(SCROLLINFO));
				si.fMask = SIF_ALL;
				si.cbSize = sizeof(SCROLLINFO);
				GetScrollInfo(hwnd,SB_VERT,&si);
				if(i < 0)si.nPos+=si.nPage;
				if(i > 0)si.nPos-=si.nPage;
				//sprintf(str,"%d",i);
				//MessageBox(hMemView,str, "mouse wheel dance!", MB_OK);
				if (si.nPos < si.nMin) si.nPos = si.nMin;
				if ((si.nPos+si.nPage) > si.nMax) si.nPos = si.nMax-si.nPage;
				CurOffset = si.nPos*16;
				SetScrollInfo(hwnd,SB_VERT,&si,TRUE);
				UpdateColorTable();
			return 0;

		case WM_SIZE:
			StopSound();
			ClientHeight = HIWORD (lParam) ;
			if(DataAmount != ((ClientHeight/MemFontHeight)*16)){
				DataAmount = ((ClientHeight/MemFontHeight)*16);
				if(DataAmount+CurOffset > MaxSize)CurOffset = MaxSize-DataAmount;
				TextColorList = realloc(TextColorList,DataAmount*sizeof(COLORREF));
				BGColorList = realloc(BGColorList,DataAmount*sizeof(COLORREF));
				OldValues = realloc(OldValues,(DataAmount)*sizeof(int));
				for(i = 0;i < DataAmount;i++)OldValues[i] = -1;
			}
			//Set vertical scroll bar range and page size
			ZeroMemory(&si, sizeof(SCROLLINFO));
			si.cbSize = sizeof (si) ;
			si.fMask  = (SIF_RANGE|SIF_PAGE) ;
			si.nMin   = 0 ;
			si.nMax   = MaxSize/16 ;
			si.nPage  = ClientHeight/MemFontHeight;
			SetScrollInfo (hwnd, SB_VERT, &si, TRUE);
			UpdateColorTable();
			return 0 ;

		case WM_COMMAND:
			StopSound();
			switch(wParam)
			{
				case 100:
					FlushUndoBuffer();
					iNesSave();
					UpdateColorTable();
				return 0;

				case 101:
				return 0;

				case 102:
					if((i = LoadTableFile()) != -1){
						sprintf(str,"Error Loading Table File At Line %d",i);
						MessageBox(hMemView,str,"error", MB_OK);
					}
					UpdateColorTable();
				return 0;

				case 200:
					UndoLastPatch();
				return 0;

				case 300:
				case 301:
				case 302:
					EditingMode = wParam-300;
					for(i = 0;i < 3;i++){
						if(EditingMode == i)CheckMenuItem(GetMenu(hMemView),300+i,MF_CHECKED);
						else CheckMenuItem(GetMenu(hMemView),300+i,MF_UNCHECKED);
					}
					if(EditingMode == 0)MaxSize = 0xFFFF;
					if(EditingMode == 1)MaxSize = 0x3FFF;
					if(EditingMode == 2)MaxSize = 16+CHRsize[0]+PRGsize[0]; //todo: add trainer size
					if(DataAmount+CurOffset > MaxSize)CurOffset = MaxSize-DataAmount;
					if(CursorEndAddy > MaxSize)CursorEndAddy = -1;
					if(CursorStartAddy > MaxSize)CursorStartAddy= MaxSize-1;

					//Set vertical scroll bar range and page size
					ZeroMemory(&si, sizeof(SCROLLINFO));
					si.cbSize = sizeof (si) ;
					si.fMask  = (SIF_RANGE|SIF_PAGE) ;
					si.nMin   = 0 ;
					si.nMax   = MaxSize/16 ;
					si.nPage  = ClientHeight/MemFontHeight;
					SetScrollInfo (hwnd, SB_VERT, &si, TRUE);

					for(i = 0;i < DataAmount;i++)OldValues[i] = -1;
					
					UpdateColorTable();
					return 0;
			}

		case WM_MOVE:
			StopSound();
			return 0;

		case WM_DESTROY :
			KillMemView();
			//ReleaseDC (hwnd, mDC) ;
			//DestroyWindow(hMemView);
			//UnregisterClass("MEMVIEW",fceu_hInstance);
			//hMemView = 0;
			return 0;
	 }
	return DefWindowProc (hwnd, message, wParam, lParam) ;
}



void DoMemView() {
	WNDCLASSEX     wndclass ;
	//static RECT al;

	if (!GI) {
		FCEUD_PrintError("You must have a game loaded before you can use the Memory Viewer.");
		return;
	}
	if (GI->type==GIT_NSF) {
		FCEUD_PrintError("Sorry, you can't yet use the Memory Viewer with NSFs.");
		return;
	}

	if (!hMemView){
		memset(&wndclass,0,sizeof(wndclass));
		wndclass.cbSize=sizeof(WNDCLASSEX);
		wndclass.style         = CS_HREDRAW | CS_VREDRAW ;
		wndclass.lpfnWndProc   = MemViewCallB ;
		wndclass.cbClsExtra    = 0 ;
		wndclass.cbWndExtra    = 0 ;
		wndclass.hInstance     = fceu_hInstance;
		wndclass.hIcon         = LoadIcon(fceu_hInstance, "FCEUXD_ICON");
		wndclass.hIconSm       = LoadIcon(fceu_hInstance, "FCEUXD_ICON");
		wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW) ;
		wndclass.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH) ;
		wndclass.lpszMenuName  = "MEMVIEWMENU" ; //TODO: add a menu
		wndclass.lpszClassName = "MEMVIEW" ;

		if(!RegisterClassEx(&wndclass)) {FCEUD_PrintError("Error Registering MEMVIEW Window Class."); return;}

		hMemView = CreateWindowEx(0,"MEMVIEW","Memory Editor",
                        //WS_OVERLAPPEDWINDOW|WS_CLIPSIBLINGS,  /* Style */
						WS_SYSMENU|WS_THICKFRAME|WS_VSCROLL,
                        CW_USEDEFAULT,CW_USEDEFAULT,625,242,  /* X,Y ; Width, Height */
                        NULL,NULL,fceu_hInstance,NULL ); 
		ShowWindow (hMemView, SW_SHOW) ;
		UpdateCaption();
		//hMemView = CreateDialog(fceu_hInstance,"MEMVIEW",NULL,MemViewCallB);
	}
	if (hMemView) {
		SetWindowPos(hMemView,HWND_TOP,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE|SWP_NOOWNERZORDER);
		//UpdateMemView(0);
		//MemViewDoBlit();
	}
}
