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

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include "main.h"
#include "unix-netplay.h"

#ifndef socklen_t
#define socklen_t int
#endif

char *netplayhost=0;
int Port=0xFCE;
int netplay=0; 

static uint32 incounter,outcounter;   
static uint32 magic;

int tport=0xFCE;
int uport=0xFCE;
 
static int Socket=-1;
static int UDPSocket=-1;

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

static void SendMentos(char *msg, int x)
{
 char buf[256+20+8];

 *(uint32 *)buf=~0;
 
 sprintf(buf+8,"<Player %d> %s",x+1,msg);
 en32(buf+4,strlen(buf+8));
 send(Socket,buf,8+1+strlen(buf+8),0);
 send(Socket,buf,8+1+strlen(buf+8),0);
 puts(buf+8);
}

int FCEUD_NetworkConnect(void)
{
 struct sockaddr_in sockin;    /* I want to play with fighting robots. */
 int TSocket;
 unsigned int remoteportudp;
  
 incounter=outcounter=0;
 
 if( (TSocket=socket(AF_INET,SOCK_STREAM,0))==-1)
 {
  puts("Error creating stream socket.");
  FCEUD_NetworkClose();
  return(0);
 }
 
 if( (UDPSocket=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP))==-1)
 {
  puts("Error creating datagram socket.");
  FCEUD_NetworkClose();
  return(0);
 }
  
 memset(&sockin,0,sizeof(sockin));
 sockin.sin_family=AF_INET;
 
 if(netplay==1)       /* Act as a server. */
 {
  int sockin_len;
  sockin.sin_addr.s_addr=INADDR_ANY;
  
  sockin_len=sizeof(sockin);
 
  sockin.sin_port=htons(tport);
  if(bind(TSocket,(struct sockaddr *)&sockin,sockin_len)==-1)
  {
   puts("Error binding to stream socket.");
   close(TSocket);
   FCEUD_NetworkClose();
   return(0);
  }
 
  sockin.sin_port=htons(uport);
  if(bind(UDPSocket,(struct sockaddr *)&sockin,sockin_len)==-1)
  {
   puts("Error binding to datagram socket.");
   close(TSocket);
   FCEUD_NetworkClose();
   return(0);
  }
  
  
  if(listen(TSocket,1)==-1)
  {
   puts("Error listening on socket.");
   close(TSocket);
   FCEUD_NetworkClose();
   return(0);
 }
 
  puts("*** Waiting for a connection...");
  if((Socket=accept(TSocket,(struct sockaddr *)&sockin,(int *)&sockin_len)) ==
                -1)
  {
    puts("Error accepting connection.");
    close(TSocket);
    FCEUD_NetworkClose();
    return(0);
  }
  {
   uint8 buf[12];
   struct timeval tv;

   gettimeofday(&tv,0);
   magic=tv.tv_sec+tv.tv_usec;

   en32(buf,uport);
   en32(buf+4,1);
   en32(buf+8,magic);
   send(Socket,buf,12,0);
 
   /* Get the UDP port the client is waiting for data on. */
   recv(Socket,buf,4,MSG_WAITALL);
   remoteportudp=de32(buf);
  }
 }
 else   /* We're a client... */
 {
  struct hostent *phostentb;
  unsigned long hadr;
   
  sockin.sin_port=htons(uport);
  sockin.sin_addr.s_addr=INADDR_ANY;

  if(bind(UDPSocket,(struct sockaddr *)&sockin,sizeof(sockin))==-1)
  {
   puts("Error binding to datagram socket.");
   close(TSocket);   
   FCEUD_NetworkClose();
   return(0);
  }
  
  hadr=inet_addr(netplayhost);
  
  if(hadr!=INADDR_NONE)
   sockin.sin_addr.s_addr=hadr;
  else
  {
   puts("*** Looking up host name...");
   if(!(phostentb=gethostbyname((const char *)netplayhost)))
   {
    puts("Error getting host network information.");
    close(TSocket);
    FCEUD_NetworkClose();
    return(0);
   }
   memcpy(&sockin.sin_addr,phostentb->h_addr,phostentb->h_length);
  }
  
  sockin.sin_port=htons(tport);
  puts("*** Connecting to remote host...");
  if(connect(TSocket,(struct sockaddr *)&sockin,sizeof(sockin))==-1)
  {
    puts("Error connecting to remote host.");
    close(TSocket);
    FCEUD_NetworkClose();
   return(0);
  }
  Socket=TSocket;
  puts("*** Sending initialization data to server...");
  {
   uint8 buf[12];
   
   /* Now, tell the server what local UDP port it should send to. */
   en32(buf,uport);
   send(Socket, buf, 4, 0);
  
   /* Get the UDP port from the server we should send data to. */
   recv(Socket, buf, 12, MSG_WAITALL);
   remoteportudp=de32(buf);
   magic=de32(buf+8);
  }
 }
 /* Associate remote host with UDPSocket. */

 sockin.sin_port=htons(remoteportudp);
 if(connect(UDPSocket,(struct sockaddr *)&sockin,sizeof(sockin))==-1)
 {
  puts("Errrorr");
  return(0);
 }
 puts("*** Connection established.");
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
	int check;

        if(send(UDPSocket,MakeUDP(&v,1),UDPHEADSIZE+1,0)<=0)
         return(0);
        outcounter++;

	if(!ioctl(fileno(stdin),FIONREAD,&check))
 	if(check)
 	{
 	  char buf[256];
	  fgets(buf,256,stdin);
	  if(strrchr(buf,'\n'))
	   *strrchr(buf,'\n')=0;
	  send(Socket,buf,strlen(buf)+1,0);
  	 }
        return(1);
}

/* 5 bytes to all clients. */
int FCEUD_SendDataToClients(uint8 *data)
{ 
        if(send(UDPSocket,MakeUDP(data,5),UDPHEADSIZE+5,0)<=0)
         return(0);

        if(send(Socket,MakeTCP(data,5),TCPHEADSIZE+5,0)<=0)
         return(0);
 
        outcounter++; 
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

   switch(select((UDPSocket>Socket)?(UDPSocket+1):(Socket+1),&funfun,0,0,&popeye))
   {
    case 0:
            //BlockingCheck();
           continue;
    case -1:return(0);
   }

  if(FD_ISSET(Socket,&funfun))
   if(recv(Socket,buf,TCPHEADSIZE+5,MSG_WAITALL)>0)
   {
    if(de32(buf)==0xFFFFFFFF)
    {
     char tbuf[512],*tmp;
     uint32 l;
   
     l=de32(buf+4)+1;
   
     tbuf[0]=buf[8];
     recv(Socket,tbuf+1,l-1,MSG_WAITALL);
     tmp=tbuf;
     while(*tmp)
     {
      if(*tmp<32) *tmp=' ';
      tmp++;
     }
     puts(tbuf);
    }
    else
    {
     if(de32(buf)==incounter) /* New packet, keep. */
     {
      unsigned long beefie;
      memcpy(data,buf+TCPHEADSIZE,5);
      incounter++;   
  
      if(!ioctl(Socket,FIONREAD,&beefie))
       if(beefie)
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
      
      if(!ioctl(UDPSocket,FIONREAD,&beefie))
       if(beefie)  
        NoWaiting|=2;
      return(1);
     }
    }
//   if(!BlockingCheck())
//    return(0);
  }     
  return 0;
}

/* 1 byte per client, nonblocking. */
int FCEUD_GetDataFromClients(uint8 *data)
{  
 uint8 buf[128];
 unsigned long beefie;
  
 beefie=0;
 if(!ioctl(Socket,FIONREAD,&beefie))
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

 while(!ioctl(UDPSocket,FIONREAD,&beefie))
 {
  if(!beefie) return(1);
  if(recv(UDPSocket,buf,UDPHEADSIZE+1,0)==-1)
   return(0);
  if(CheckUDP(buf,1,1))
  {
   *data=buf[UDPHEADSIZE];
   incounter=de32(buf+8)+1;
  }
 }
 return(0);
}

void FCEUD_NetworkClose(void)
{
 if(Socket>0)
  close(Socket);
 Socket=-1;
}

