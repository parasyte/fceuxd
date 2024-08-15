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

static int recv_tcpwrap(uint8 *buf, int len, int block);
static void NetStatAdd(char *text);

static HWND netwin=0;

static char *netstatt[64];
static int netstattcount=0;

static uint32 incounter,outcounter;
static uint32 magic;

static char netplayhost[256]={"starmen.net"};
static int remotetport=0xFCE;
static int localtport=0xFCE;
static int localuport=0xFCE;
static int netplaytype=1;
static int faterror;

static int netplayon=0;

static SOCKET Socket=INVALID_SOCKET;
static SOCKET UDPSocket=INVALID_SOCKET;

static int wsainit=0;

static void WSE(char *ahh)
{
 char tmp[256];
 sprintf(tmp,"*** Winsock: %s",ahh);
 NetStatAdd(tmp);
}

static void en32(uint8 *buf, uint32 morp)
{
 buf[0]=morp;
 buf[1]=morp>>8;
 buf[2]=morp>>16;
 buf[3]=morp>>24;
}

static uint32 de32(uint8 *morp)
{
 return(morp[0]|(morp[1]<<8)|(morp[2]<<16)|(morp[3]<<24));
}

void FCEUD_NetworkClose(void)
{
 NetStatAdd("*** Connection lost.");
 if(Socket!=INVALID_SOCKET)
 {
  closesocket(Socket);
  Socket=INVALID_SOCKET;
 }

 if(UDPSocket!=INVALID_SOCKET)
 {
  closesocket(UDPSocket);
  UDPSocket=INVALID_SOCKET;
 }

 if(wsainit)
 {
  WSACleanup();
  wsainit=0;
 }
 /* Make sure blocking is returned to normal once network play is stopped. */
 NoWaiting&=~2;
}

static void SendMentos(char *msg, int x)
{
 char buf[256+20+8];

 *(uint32 *)buf=~0;

 sprintf(buf+8,"<Player %d> %s",x+1,msg);
 en32(buf+4,strlen(buf+8));
 send(Socket,buf,8+1+strlen(buf+8),0);
 NetStatAdd(buf+8);
}

static BOOL CALLBACK NetCon(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg)
  {
   case WM_COMMAND:
                   if(HIWORD(wParam)==BN_CLICKED)
                   {
                    DestroyWindow(hwndDlg);
                    netwin=0;
                    FCEUD_NetworkClose();
                   }
                   else if(HIWORD(wParam)==EN_CHANGE && Socket!=INVALID_SOCKET)
                   {
                    char buf[256];
                    int t;

                    t=GetDlgItemText(hwndDlg,102,buf,256);
                    buf[255]=0;
                    if((t==255) || strrchr(buf,'\r'))
                    {
                     if(strrchr(buf,'\r'))
                      *strrchr(buf,'\r')=0;

                     if(!netplaytype)
                     {
                      SendMentos(buf,0);                      
                     }
                     else
                      send(Socket,buf,strlen(buf)+1,0);
                     SetDlgItemText(hwndDlg,102,"");
                    }
                   }
                   break;
   case WM_INITDIALOG:break;
  }

 return 0;
}

static void NetStatAdd(char *text)
{
 char textbuf[65536];
 int x;

 if(!netwin) return;
 if(netstattcount>=64) free(netstatt[netstattcount&63]);

 if(!(netstatt[netstattcount&63]=malloc(strlen(text)+1)))
  return;
 strcpy(netstatt[netstattcount&63],text);
 netstattcount++;

 textbuf[0]=0;
 if(netstattcount>=64)
 {
  x=netstattcount&63;
  for(;;)
  {
   strcat(textbuf,netstatt[x]);
   x=(x+1)&63;
   if(x==(netstattcount&63)) break;
   strcat(textbuf,"\r\n");
  }
 }
 else
  for(x=0;x<netstattcount;x++)
  {
   strcat(textbuf,netstatt[x]);
   if(x<(netstattcount-1))
    strcat(textbuf,"\r\n");
  }
 SetDlgItemText(netwin,101,textbuf);
 SendDlgItemMessage(netwin,101,EM_LINESCROLL,0,200);
}

int FCEUD_NetworkConnect(void)
{
 WSADATA WSAData;
 SOCKADDR_IN sockin;    /* I want to play with fighting robots. */
 SOCKET TSocket;
 unsigned int remoteportudp;

 incounter=outcounter=0;
 faterror=0;

 if(!netwin)
  netwin=CreateDialog(fceu_hInstance,"NETMOO",0,NetCon);

 if(WSAStartup(MAKEWORD(1,1),&WSAData))
 {
  NetStatAdd("*** Error initializing WIndows Sockets.");
  return(0);
 }
 wsainit=1;

 if( (TSocket=socket(AF_INET,SOCK_STREAM,0))==INVALID_SOCKET)
 {
  WSE("Error creating stream socket.");
  FCEUD_NetworkClose();
  return(0);
 }

 if( (UDPSocket=socket(AF_INET,SOCK_DGRAM,0))==INVALID_SOCKET)
 {
  WSE("Error creating datagram socket.");
  FCEUD_NetworkClose();
  return(0);
 }

 memset(&sockin,0,sizeof(sockin));
 sockin.sin_family=AF_INET;

 if(!netplaytype)       /* Act as a server. */
 {
  int sockin_len;
  sockin.sin_addr.s_addr=INADDR_ANY;

  sockin_len=sizeof(sockin);

  sockin.sin_port=htons(localtport);
  if(bind(TSocket,(struct sockaddr *)&sockin,sockin_len)==SOCKET_ERROR)
  {
   WSE("Error binding to stream socket.");
   closesocket(TSocket);
   FCEUD_NetworkClose();
   return(0);
  }

  sockin.sin_port=htons(localtport+1);
  if(bind(UDPSocket,(struct sockaddr *)&sockin,sockin_len)==SOCKET_ERROR)
  {
   WSE("Error binding to datagram socket.");
   closesocket(TSocket);
   FCEUD_NetworkClose();
   return(0);
  }


  if(listen(TSocket,1)==SOCKET_ERROR)
  {
   WSE("Error listening on socket.");
   closesocket(TSocket);
   FCEUD_NetworkClose();
   return(0);
  }

  NetStatAdd("*** Waiting for a connection...");
  if((Socket=accept(TSocket,(struct sockaddr *)&sockin,(int *)&sockin_len)) ==
                INVALID_SOCKET)
  {
    WSE("Error accepting connection.");
    closesocket(TSocket);
    FCEUD_NetworkClose();
    return(0);
  }

  {
   uint8 buf[12];
   uint64 tmp;

   en32(buf,localtport+1);
   en32(buf+4,1);

   if(!QueryPerformanceCounter((LARGE_INTEGER*)&tmp))
    tmp=GetTickCount();   
   srand(tmp);
   magic=rand();
   en32(buf+8,magic);
   send(Socket,buf,12,0);

   /* Get the UDP port the client is waiting for data on. */
   recv_tcpwrap(buf,4,1);
   remoteportudp=de32(buf);
  }
 }
 else   /* We're a client... */
 {
  struct hostent *phostentb;
  unsigned long hadr;
  int sockin_len;


  sockin.sin_port=htons(localuport);
  sockin.sin_addr.s_addr=INADDR_ANY;
  sockin_len=sizeof(sockin);

  if(bind(UDPSocket,(struct sockaddr *)&sockin,sockin_len)==SOCKET_ERROR)
  {
   WSE("Error binding to datagram socket.");
   closesocket(TSocket);
   FCEUD_NetworkClose();
   return(0);
  }  

  hadr=inet_addr(netplayhost);

  if(hadr!=INADDR_NONE)
   sockin.sin_addr.s_addr=hadr;
  else
  {
   NetStatAdd("*** Looking up host name...");
   if(!(phostentb=gethostbyname((const char *)netplayhost)))
   {
    WSE("Error getting host network information.");
    closesocket(TSocket);
    FCEUD_NetworkClose();
    return(0);
   }
   memcpy((char *)&sockin.sin_addr,((PHOSTENT)phostentb)->h_addr,((PHOSTENT)phostentb)->h_length);
  }

  sockin.sin_port=htons(remotetport);
  NetStatAdd("*** Connecting to remote host...");
  if(connect(TSocket,(PSOCKADDR)&sockin,sizeof(sockin))==SOCKET_ERROR)
  {
    WSE("Error connecting to remote host.");
    closesocket(TSocket);
    FCEUD_NetworkClose();
    return(0);
  }
  Socket=TSocket;
  NetStatAdd("*** Sending initialization data to server...");
  {
   uint8 buf[12];

   /* Now, tell the server what local UDP port it should send to. */
   en32(buf,localuport);
   send(Socket, buf, 4, 0);

   /* Get the UDP port from the server we should send data to. */
   recv_tcpwrap(buf, 12, 1);
   remoteportudp=de32(buf);
   magic=de32(buf+8);
  }

 }

 /* Associate remote host with UDPSocket. */
 sockin.sin_port=htons(remoteportudp);
 connect(UDPSocket,(PSOCKADDR)&sockin,sizeof(sockin));

 NetStatAdd("*** Connection established.");
 return(1);
}


static int CheckUDP(uint8 *packet, int32 len, int32 alt)
{
 uint32 crc; 
 uint32 repcrc;

 crc=FCEUI_CRC32(0,packet+4,len+8);
 repcrc=de32(packet);
 if(crc!=repcrc) return(0); /* CRC32 mismatch, bad packet. */
 packet+=4;

 if(de32(packet)!=magic) /* Magic number mismatch, bad or spoofed packet. */
  return(0);

 packet+=4;
 if(alt)
 {
  if(de32(packet)<incounter) /* Time warped packet. */
   return(0);
 }
 else
  if(de32(packet)!=incounter) /* Time warped packet. */
   return(0);

 return(1);
}


/* Be careful where these MakeXXX() functions are used. */
static uint8 *MakeUDP(uint8 *data, int32 len)
{
 /* UDP packet data header is 12 bytes in length. */
 static uint8 buf[12+32]; // arbitrary 32.

 en32(buf+4,magic);
 en32(buf+8,outcounter);
 memcpy(buf+12,data,len);
 en32(buf,FCEUI_CRC32(0,buf+4,8+len));
 return(buf);
}

static uint8 *MakeTCP(uint8 *data, int32 len)
{
 /* TCP packet data header is 4 bytes in length. */
 static uint8 buf[4+32]; // arbitrary 32.

 en32(buf,outcounter);
 memcpy(buf+4,data,len);
 return(buf);
}

#define UDPHEADSIZE 12
#define TCPHEADSIZE 4

/* 1 byte to server */
int FCEUD_SendDataToServer(uint8 v,uint8 cmd)
{
        if(send(UDPSocket,MakeUDP(&v,1),UDPHEADSIZE+1,0)<=0)
         return(0);
        outcounter++;
        return(1);
}


/* 5 bytes to all clients. */
int FCEUD_SendDataToClients(uint8 *data)
{
        if(send(UDPSocket,MakeUDP(data,5),UDPHEADSIZE+5,0)!=(UDPHEADSIZE+5))
         return(0);

        if(send(Socket,MakeTCP(data,5),TCPHEADSIZE+5,0)!=(TCPHEADSIZE+5))
         return(0);

        outcounter++;
        return(1);
}

static uint8 inq[1024];
static uint32 inqlen=0;
static uint32 inqread=0;

static int recv_tcpwrap(uint8 *exbuf, int len, int block)
{
 uint8 buf[128], *tmp;
 fd_set fdoo;
 int t;
 struct timeval popeye;

 popeye.tv_sec=0;
 popeye.tv_usec=100000;

 while(inqlen<len)
 {
  FD_ZERO(&fdoo);
  FD_SET(Socket,&fdoo);

  switch(select(0,&fdoo,0,0,&popeye))
  {
   case 0:
           BlockingCheck();
           continue;
   case SOCKET_ERROR:return(0);
  }

  t=recv(Socket,buf,128,0);
  if(t<=0) return(0);

  tmp=buf;
  while(t)
  {
   inq[(inqlen+inqread)&1023]=*tmp;
   inqlen++;
   t--;
   tmp++;
  }
  if(!block && inqlen<len) return(-1);
 }

 inqlen-=len;
 while(len)
 {
  *exbuf=inq[inqread];
  exbuf++;
  len--;
  inqread=(inqread+1)&1023;
 }
 return(1);
}

/* 5 bytes from server, blocking. */
int FCEUD_GetDataFromServer(uint8 *data)
{
  uint8 buf[128];

  NoWaiting&=~2;

  for(;;)
  {   
   fd_set funfun;
   struct timeval popeye;

   popeye.tv_sec=0;
   popeye.tv_usec=100000;

   FD_ZERO(&funfun);
   FD_SET(Socket,&funfun);
   FD_SET(UDPSocket,&funfun);

   switch(select(0,&funfun,0,0,&popeye))
   {
    case 0:
            BlockingCheck();
            continue;
    case SOCKET_ERROR:return(0);
   }

  if(FD_ISSET(Socket,&funfun))
   if(recv_tcpwrap(buf,TCPHEADSIZE+5,0)>0)
   {    
    if(de32(buf)==0xFFFFFFFF)
    {
     char tbuf[512],*tmp;
     uint32 l;

     l=de32(buf+4)+1;

     tbuf[0]=buf[8];
     recv_tcpwrap(tbuf+1,l-1,1);
     tmp=tbuf;
     while(*tmp)
     {
      if(*tmp<32) *tmp=' ';
      tmp++;
     }
     NetStatAdd(tbuf);
    }
    else
    {
     if(de32(buf)==incounter) /* New packet, keep. */
     {
      memcpy(data,buf+TCPHEADSIZE,5);
      incounter++;      
      if(inqlen>=(TCPHEADSIZE+5))
        NoWaiting|=2;
      return(1);
     }
    }
   }

   if(FD_ISSET(UDPSocket,&funfun))
    if(recv(UDPSocket,buf,UDPHEADSIZE+5,0)==(UDPHEADSIZE+5))
    {
     if(CheckUDP(buf, 5, 0))
     {    
      unsigned long beefie;
      memcpy(data,buf+UDPHEADSIZE,5);
      incounter++;


      if(!ioctlsocket(UDPSocket,FIONREAD,&beefie))
       if(beefie)       /* Winsock sucks. */
      //if(beefie==(UDPHEADSIZE+5))
      //if(recv(UDPSocket,buf,UDPHEADSIZE+5,MSG_PEEK)==(UDPHEADSIZE+5))
      // if(CheckUDP(buf, 5, 0))
        NoWaiting|=2;
      return(1);
     }
    }
   if(!BlockingCheck())
    return(0);
  }
  return 0;
}

/* 1 byte per client, nonblocking. */
int FCEUD_GetDataFromClients(uint8 *data)
{
 unsigned long beefie;

 beefie=0;
 if(!ioctlsocket(Socket,FIONREAD,&beefie))
  if(beefie)
  {
   int t=0;
   uint8 buf[256];
   do
   {
    recv(Socket,buf+t,1,0);
    if(!buf[t]) break;
    if((buf[t])<32) buf[t]=' ';
    t++;
   } while(t<256);
   buf[255]=0;
   SendMentos(buf,1);
  }

 while(!ioctlsocket(UDPSocket,FIONREAD,&beefie))
 {
  uint8 buf[128];
  if(!beefie) return(1);
  if(recv(UDPSocket,buf,UDPHEADSIZE+1,0)==SOCKET_ERROR)
   return(0);
  if(CheckUDP(buf,1,1))
  {
   *data=buf[UDPHEADSIZE];
   incounter=de32(buf+8)+1;
  }
 }

 return(0);
}

BOOL CALLBACK NetConCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg) {
   case WM_INITDIALOG:                

                CheckDlgButton(hwndDlg,300,netplayon?BST_CHECKED:BST_UNCHECKED);
                CheckRadioButton(hwndDlg,301,302,301+netplaytype);

                SetDlgItemInt(hwndDlg,100,localuport,0);
                if(netplayhost[0])
                 SetDlgItemText(hwndDlg,101,netplayhost);
                SetDlgItemInt(hwndDlg,102,remotetport,0);

                SetDlgItemInt(hwndDlg,200,localtport,0);
                break;
   case WM_CLOSE:
   case WM_QUIT: goto gornk;
   case WM_COMMAND:
                if(!(wParam>>16))
                switch(wParam&0xFFFF)
                {
                 case 1:
                        gornk:

                        localuport=GetDlgItemInt(hwndDlg,100,0,0);
                        remotetport=GetDlgItemInt(hwndDlg,102,0,0);
                        localtport=GetDlgItemInt(hwndDlg,200,0,0);

                        if(IsDlgButtonChecked(hwndDlg,300)==BST_CHECKED)
                         netplayon=1;
                        else
                         netplayon=0;

                        if(IsDlgButtonChecked(hwndDlg,301)==BST_CHECKED)
                         netplaytype=0;
                        else
                         netplaytype=1;

                        GetDlgItemText(hwndDlg,101,netplayhost,255);
                        netplayhost[255]=0;

                        EndDialog(hwndDlg,0);
                        break;
               }
              }
  return 0;
}


static void ConfigNetplay(void)
{
 DialogBox(fceu_hInstance,"NETPLAYCONFIG",hAppWnd,NetConCallB);

 if(netplayon)
  FCEUI_SetNetworkPlay(netplaytype+1);
 else
  FCEUI_SetNetworkPlay(0);
}

