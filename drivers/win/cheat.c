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
#include "cheat.h"
#include "..\..\memview.h"

int selcheat;
int ChtPosX,ChtPosY;
static HFONT hFont,hNewFont;

static int scrollindex;
static int scrollnum;
static int scrollmax;

int lbfocus=0;
int searchdone;
static int knownvalue=0;


uint16 StrToU16(char *s) {
	unsigned int ret=0;
	sscanf(s,"%4x",&ret);
	return ret;
}

uint8 StrToU8(char *s) {
	unsigned int ret=0;
	sscanf(s,"%2x",&ret);
	return ret;
}

char *U16ToStr(uint16 a) {
	static char str[5];
	sprintf(str,"%04X",a);
	return str;
}

char *U8ToStr(uint8 a) {
	static char str[3];
	sprintf(str,"%02X",a);
	return str;
}

static HWND hwndLB;
//int RedoCheatsCallB(char *name, uint32 a, uint8 v, int s) { //bbit edited: this commented out line was changed to the below for the new fceud
int RedoCheatsCallB(char *name, uint32 a, uint8 v, int compare,int s,int type) {
	char str[259] = { 0 };

	//strcpy(str,(s?"* ":"  "));
	//strcat(str,name);
	if(name[0] == 0)sprintf(str,"%s%04X=%02X",s?"* ":"  ",(int)a,(int)v);
	else sprintf(str,"%s%s",s?"* ":"  ",name);

	SendDlgItemMessage(hwndLB,101,LB_ADDSTRING,0,(LPARAM)(LPSTR)str);
	return 1;
}

void RedoCheatsLB(HWND hwndDlg) {
	SendDlgItemMessage(hwndDlg,101,LB_RESETCONTENT,0,0);
	hwndLB=hwndDlg;
	FCEUI_ListCheats(RedoCheatsCallB);
}

int ShowResultsCallB(uint32 a, uint8 last, uint8 current) {
	char temp[16];

	sprintf(temp,"$%04X:  %02X | %02X",(unsigned int)a,last,current);
	SendDlgItemMessage(hwndLB,106,LB_ADDSTRING,0,(LPARAM)(LPSTR)temp);
	return 1;
}

void ShowResults(HWND hwndDlg) {
	int n=FCEUI_CheatSearchGetCount();
	int t;
	char str[20];

	scrollnum=n;
	scrollindex=-32768;

	hwndLB=hwndDlg;
	SendDlgItemMessage(hwndDlg,106,LB_RESETCONTENT,0,0);
	FCEUI_CheatSearchGetRange(0,16,ShowResultsCallB);

	t=-32768+n-17;
	if (t<-32768) t=-32768;
	scrollmax=t;
	SendDlgItemMessage(hwndDlg,107,SBM_SETRANGE,-32768,t);
	SendDlgItemMessage(hwndDlg,107,SBM_SETPOS,-32768,1);

	sprintf(str,"%d Possibilities",n);
	SetDlgItemText(hwndDlg,203,str);
}

void EnableCheatButtons(HWND hwndDlg, int enable) {
	EnableWindow(GetDlgItem(hwndDlg,105),enable);
	EnableWindow(GetDlgItem(hwndDlg,305),enable);
	EnableWindow(GetDlgItem(hwndDlg,306),enable);
	EnableWindow(GetDlgItem(hwndDlg,307),enable);
	EnableWindow(GetDlgItem(hwndDlg,308),enable);
	EnableWindow(GetDlgItem(hwndDlg,309),enable);
}

extern void StopSound(void);

BOOL CALLBACK CheatConsoleCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	LOGFONT lf;
	RECT wrect;

	char str[259] = { 0 },str2[259] = { 0 };

	char *name;
	uint32 a;
	uint8 v;
	int s;

	switch (uMsg) {
		case WM_INITDIALOG:
			StopSound();
			SetWindowPos(hwndDlg,0,ChtPosX,ChtPosY,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOOWNERZORDER);

			//setup font
			hFont = (HFONT)SendMessage(hwndDlg, WM_GETFONT, 0, 0);
			GetObject(hFont, sizeof(LOGFONT), &lf);
			strcpy(lf.lfFaceName,"Courier");
			hNewFont = CreateFontIndirect(&lf);

			SendDlgItemMessage(hwndDlg,102,WM_SETFONT,(WPARAM)hNewFont,FALSE);
			SendDlgItemMessage(hwndDlg,103,WM_SETFONT,(WPARAM)hNewFont,FALSE);
			SendDlgItemMessage(hwndDlg,105,WM_SETFONT,(WPARAM)hNewFont,FALSE);
			SendDlgItemMessage(hwndDlg,106,WM_SETFONT,(WPARAM)hNewFont,FALSE);
			SendDlgItemMessage(hwndDlg,108,WM_SETFONT,(WPARAM)hNewFont,FALSE);
			SendDlgItemMessage(hwndDlg,109,WM_SETFONT,(WPARAM)hNewFont,FALSE);
			SendDlgItemMessage(hwndDlg,110,WM_SETFONT,(WPARAM)hNewFont,FALSE);

			//text limits
			SendDlgItemMessage(hwndDlg,102,EM_SETLIMITTEXT,4,0);
			SendDlgItemMessage(hwndDlg,103,EM_SETLIMITTEXT,2,0);
			SendDlgItemMessage(hwndDlg,104,EM_SETLIMITTEXT,256,0);
			SendDlgItemMessage(hwndDlg,105,EM_SETLIMITTEXT,2,0);
			SendDlgItemMessage(hwndDlg,108,EM_SETLIMITTEXT,2,0);
			SendDlgItemMessage(hwndDlg,109,EM_SETLIMITTEXT,2,0);
			SendDlgItemMessage(hwndDlg,110,EM_SETLIMITTEXT,2,0);

			//disable or enable buttons
			EnableWindow(GetDlgItem(hwndDlg,105),FALSE);
			EnableWindow(GetDlgItem(hwndDlg,302),FALSE);
			EnableWindow(GetDlgItem(hwndDlg,303),FALSE);
			if (scrollnum) {
				EnableCheatButtons(hwndDlg,TRUE);
				ShowResults(hwndDlg);
				sprintf(str,"%d Possibilities",(int)FCEUI_CheatSearchGetCount());
				SetDlgItemText(hwndDlg,203,str);
			}
			else EnableCheatButtons(hwndDlg,FALSE);

			//misc setup
			RedoCheatsLB(hwndDlg);
			searchdone=0;
			SetDlgItemText(hwndDlg,105,(LPTSTR)U8ToStr(knownvalue));
			break;

		case WM_NCACTIVATE:
			if ((CheatStyle) && (scrollnum)) {
				if ((!wParam) && (searchdone)) {
					searchdone=0;
					FCEUI_CheatSearchSetCurrentAsOriginal();
				}
				ShowResults(hwndDlg);
			}
			break;

		case WM_CLOSE:
		case WM_QUIT:
			CheatWindow=0;
			hCheat = 0;
			if (CheatStyle) DestroyWindow(hwndDlg);
			else EndDialog(hwndDlg,0);
			DeleteObject(hFont);
			DeleteObject(hNewFont);
			if (searchdone) FCEUI_CheatSearchSetCurrentAsOriginal();
			break;

		case WM_MOVE:
			GetWindowRect(hwndDlg,&wrect);
			ChtPosX = wrect.left;
			ChtPosY = wrect.top;
			break;

		case WM_VSCROLL:
			if (scrollnum > 16) {
				StopSound();
 				switch (LOWORD(wParam)) {
					case SB_TOP:
						scrollindex=-32768;
						SendDlgItemMessage(hwndDlg,107,SBM_SETPOS,scrollindex,1);
						SendDlgItemMessage(hwndDlg,106,LB_RESETCONTENT,16,0);
						FCEUI_CheatSearchGetRange(scrollindex+32768,scrollindex+32768+16,ShowResultsCallB);
						break;
					case SB_BOTTOM:
						scrollindex=scrollmax;
						SendDlgItemMessage(hwndDlg,107,SBM_SETPOS,scrollindex,1);
						SendDlgItemMessage(hwndDlg,106,LB_RESETCONTENT,16,0);
						FCEUI_CheatSearchGetRange(scrollindex+32768,scrollindex+32768+16,ShowResultsCallB);
						break;
					case SB_LINEUP:
						if (scrollindex > -32768) {
							scrollindex--;
							SendDlgItemMessage(hwndDlg,107,SBM_SETPOS,scrollindex,1);
							SendDlgItemMessage(hwndDlg,106,LB_RESETCONTENT,16,0);
							FCEUI_CheatSearchGetRange(scrollindex+32768,scrollindex+32768+16,ShowResultsCallB);
						}
						break;
					case SB_PAGEUP:
						scrollindex-=17;
						if(scrollindex<-32768) scrollindex=-32768;
						SendDlgItemMessage(hwndDlg,107,SBM_SETPOS,scrollindex,1);
						SendDlgItemMessage(hwndDlg,106,LB_RESETCONTENT,16,0);
						FCEUI_CheatSearchGetRange(scrollindex+32768,scrollindex+32768+16,ShowResultsCallB);
						break;

					case SB_LINEDOWN:
						if (scrollindex<scrollmax) {
							scrollindex++;
							SendDlgItemMessage(hwndDlg,107,SBM_SETPOS,scrollindex,1);
							SendDlgItemMessage(hwndDlg,106,LB_RESETCONTENT,0,0);
							FCEUI_CheatSearchGetRange(scrollindex+32768,scrollindex+32768+16,ShowResultsCallB);
						}
						break;
					case SB_PAGEDOWN:
						scrollindex+=17;
						if (scrollindex>scrollmax) scrollindex=scrollmax;
						SendDlgItemMessage(hwndDlg,107,SBM_SETPOS,scrollindex,1);
						SendDlgItemMessage(hwndDlg,106,LB_RESETCONTENT,0,0);
						FCEUI_CheatSearchGetRange(scrollindex+32768,scrollindex+32768+16,ShowResultsCallB);
						break;
					case SB_THUMBPOSITION:
					case SB_THUMBTRACK:
						scrollindex=(short int)HIWORD(wParam);
						SendDlgItemMessage(hwndDlg,107,SBM_SETPOS,scrollindex,1);
						SendDlgItemMessage(hwndDlg,106,LB_RESETCONTENT,0,0);
						FCEUI_CheatSearchGetRange(32768+scrollindex,32768+scrollindex+16,ShowResultsCallB);
						break;
				}

			}
			break;

		case WM_VKEYTOITEM:
			if (lbfocus) {
				int real;

				real=SendDlgItemMessage(hwndDlg,106,LB_GETCURSEL,0,0);
				switch (LOWORD(wParam)) {
					case VK_UP:
						// mmmm....recursive goodness
						if (real == 0) SendMessage(hwndDlg,WM_VSCROLL,SB_LINEUP,0);
						return -1;
						break;
					case VK_DOWN:
						if (real == 16) {
							SendMessage(hwndDlg,WM_VSCROLL,SB_LINEDOWN,0);
							SendDlgItemMessage(hwndDlg,106,LB_SETCURSEL,real,0);
						}
						return -1;
						break;
					case VK_PRIOR:
						SendMessage(hwndDlg,WM_VSCROLL,SB_PAGEUP,0);
						break;
					case VK_NEXT:
						SendMessage(hwndDlg,WM_VSCROLL,SB_PAGEDOWN,0);
						break;
					case VK_HOME:
						SendMessage(hwndDlg,WM_VSCROLL,SB_TOP,0);
						break;
					case VK_END:
						SendMessage(hwndDlg,WM_VSCROLL,SB_BOTTOM,0);
						break;
				}
				return -2;
			}
			break;

		case WM_COMMAND:
			switch (HIWORD(wParam)) {
				case BN_CLICKED:
					switch (LOWORD(wParam)) {
						case 301: //Add
							GetDlgItemText(hwndDlg,102,str,5);
							a=StrToU16(str);
							GetDlgItemText(hwndDlg,103,str,3);
							v=StrToU8(str);
							GetDlgItemText(hwndDlg,104,str,256);
//							if (FCEUI_AddCheat(str,a,v)) { //bbit edited: replaced this with the line below
							if (FCEUI_AddCheat(str,a,v,-1,1)) {
							if(str[0] == 0)sprintf(str,"%04X=%02X",(int)a,(int)v); //bbit edited: added this line to give your cheat a name if you didn't supply one
								strcpy(str2,"* ");
								strcat(str2,str);
								SendDlgItemMessage(hwndDlg,101,LB_ADDSTRING,0,(LPARAM)(LPSTR)str2);
								selcheat = (SendDlgItemMessage(hwndDlg,101,LB_GETCOUNT,0,0) - 1);
								SendDlgItemMessage(hwndDlg,101,LB_SETCURSEL,selcheat,0);

								SetDlgItemText(hwndDlg,102,(LPTSTR)U16ToStr(a));
								SetDlgItemText(hwndDlg,103,(LPTSTR)U8ToStr(v));

								EnableWindow(GetDlgItem(hwndDlg,302),TRUE);
								EnableWindow(GetDlgItem(hwndDlg,303),TRUE);
							}
							if(hMemView)UpdateColorTable(); //if the memory viewer is open then update any blue freeze locations in it as well
							break;
						case 302: //Delete
							if (selcheat >= 0) {
								FCEUI_DelCheat(selcheat);
								SendDlgItemMessage(hwndDlg,101,LB_DELETESTRING,selcheat,0);
								selcheat=-1;
								SetDlgItemText(hwndDlg,102,(LPTSTR)"");
								SetDlgItemText(hwndDlg,103,(LPTSTR)"");
								SetDlgItemText(hwndDlg,104,(LPTSTR)"");
							}
							EnableWindow(GetDlgItem(hwndDlg,302),FALSE);
							EnableWindow(GetDlgItem(hwndDlg,303),FALSE);
							if(hMemView)UpdateColorTable(); //if the memory viewer is open then update any blue freeze locations in it as well
							break;
						case 303: //Update
							if (selcheat < 0) break;

							GetDlgItemText(hwndDlg,102,str,5);
							a=StrToU16(str);
							GetDlgItemText(hwndDlg,103,str,3);
							v=StrToU8(str);
							GetDlgItemText(hwndDlg,104,str,256);
//							FCEUI_SetCheat(selcheat,str,a,v,-1); //bbit edited: replaced this with the line below
							FCEUI_SetCheat(selcheat,str,a,v,-1,-1,0);
//							FCEUI_GetCheat(selcheat,&name,&a,&v,&s); //bbit edited: replaced this with the line below
							FCEUI_GetCheat(selcheat,&name,&a,&v,NULL,&s,NULL);
							strcpy(str2,(s?"* ":"  "));
							if(str[0] == 0)sprintf(str,"%04X=%02X",(int)a,(int)v); //bbit edited: added this line to give your cheat a name if you didn't supply one
							strcat(str2,str);

							SendDlgItemMessage(hwndDlg,101,LB_DELETESTRING,selcheat,0);
							SendDlgItemMessage(hwndDlg,101,LB_INSERTSTRING,selcheat,(LPARAM)(LPSTR)str2);
							SendDlgItemMessage(hwndDlg,101,LB_SETCURSEL,selcheat,0);

							SetDlgItemText(hwndDlg,102,(LPTSTR)U16ToStr(a));
							SetDlgItemText(hwndDlg,103,(LPTSTR)U8ToStr(v));
							if(hMemView)UpdateColorTable(); //if the memory viewer is open then update any blue freeze locations in it as well
							break;
						case 304: //Reset
							FCEUI_CheatSearchBegin();
							ShowResults(hwndDlg);
							EnableCheatButtons(hwndDlg,TRUE);
							break;
						case 305: //Known Value
							searchdone=1;
							GetDlgItemText(hwndDlg,105,str,3);
							knownvalue=StrToU8(str);
							FCEUI_CheatSearchEnd(4,knownvalue,0);
							ShowResults(hwndDlg);
							break;
						case 306: //Equal
							searchdone=1;
							FCEUI_CheatSearchEnd(2,0,0);
							ShowResults(hwndDlg);
							break;
						case 307: //Not Equal
							searchdone=1;
							if (IsDlgButtonChecked(hwndDlg,401) == BST_CHECKED) {
								GetDlgItemText(hwndDlg,108,str,3);
								FCEUI_CheatSearchEnd(2,0,StrToU8(str));
							}
							else FCEUI_CheatSearchEnd(3,0,0);
							ShowResults(hwndDlg);
							break;
						case 308: //Greater Than
							searchdone=1;
							if (IsDlgButtonChecked(hwndDlg,402) == BST_CHECKED) {
								GetDlgItemText(hwndDlg,109,str,3);
								FCEUI_CheatSearchEnd(7,0,StrToU8(str));
							}
							else FCEUI_CheatSearchEnd(5,0,0);
							ShowResults(hwndDlg);
							break;
						case 309: //Less Than
							searchdone=1;
							if (IsDlgButtonChecked(hwndDlg,403) == BST_CHECKED) {
								GetDlgItemText(hwndDlg,110,str,3);
								FCEUI_CheatSearchEnd(8,0,StrToU8(str));
							}
							else FCEUI_CheatSearchEnd(6,0,0);
							ShowResults(hwndDlg);
							break;
					}
					break;
				case LBN_DBLCLK:
					switch (LOWORD(wParam)) {
						case 101:
//							FCEUI_GetCheat(selcheat,&name,&a,&v,&s); //bbit edited: replaced this with the line below
							FCEUI_GetCheat(selcheat,&name,&a,&v,NULL,&s,NULL);
//							FCEUI_SetCheat(selcheat,0,-1,-1,s^=1);//bbit edited: replaced this with the line below
							FCEUI_SetCheat(selcheat,0,-1,-1,-1,s^=1,0);
							if(name[0] == 0)sprintf(str,"%s%04X=%02X",s?"* ":"  ",(unsigned int)a,(unsigned int)v);
							else sprintf(str,"%s%s",s?"* ":"  ",name);
							//strcpy(str,(s?"* ":"  "));
							//strcat(str,name);
							SendDlgItemMessage(hwndDlg,101,LB_DELETESTRING,selcheat,0);
							SendDlgItemMessage(hwndDlg,101,LB_INSERTSTRING,selcheat,(LPARAM)(LPSTR)str);
							SendDlgItemMessage(hwndDlg,101,LB_SETCURSEL,selcheat,0);
							break;
					}
					break;
				case LBN_SELCHANGE:
					switch (LOWORD(wParam)) {
						case 101:
							selcheat = SendDlgItemMessage(hwndDlg,101,LB_GETCURSEL,0,0);
							if (selcheat < 0) break;

							FCEUI_GetCheat(selcheat,&name,&a,&v,NULL,&s,NULL);
							SetDlgItemText(hwndDlg,104,(LPTSTR)name);
							SetDlgItemText(hwndDlg,102,(LPTSTR)U16ToStr(a));
							SetDlgItemText(hwndDlg,103,(LPTSTR)U8ToStr(v));

							EnableWindow(GetDlgItem(hwndDlg,302),TRUE);
							EnableWindow(GetDlgItem(hwndDlg,303),TRUE);
							break;
						case 106:
							lbfocus=1;
							SendDlgItemMessage(hwndDlg,106,LB_GETTEXT,SendDlgItemMessage(hwndDlg,106,LB_GETCURSEL,0,0),(LPARAM)(LPCTSTR)str);
							strcpy(str2,str+1);
							str2[4] = 0;
							SetDlgItemText(hwndDlg,102,(LPTSTR)str2);
							strcpy(str2,str+13);
							SetDlgItemText(hwndDlg,103,(LPTSTR)str2);
							break;
					}
					break;
				case LBN_SELCANCEL:
					switch(LOWORD(wParam)) {
						case 101:
							EnableWindow(GetDlgItem(hwndDlg,302),FALSE);
							EnableWindow(GetDlgItem(hwndDlg,303),FALSE);
							break;
						case 106:
							lbfocus=0;
							break;
					}
					break;

			}
			break;
	}
	return 0;
}


void ConfigCheats(HWND hParent) {
	if (!GI) {
		FCEUD_PrintError("You must have a game loaded before you can manipulate cheats.");
		return;
	}
	if (GI->type==GIT_NSF) {
		FCEUD_PrintError("Sorry, you can't cheat with NSFs.");
		return;
	}

	if (!CheatWindow) {
		selcheat=-1;
		CheatWindow=1;
		if (CheatStyle) hCheat = CreateDialog(fceu_hInstance,"CHEATCONSOLE",NULL,CheatConsoleCallB);
		else DialogBox(fceu_hInstance,"CHEATCONSOLE",hParent,CheatConsoleCallB);
	}
}
