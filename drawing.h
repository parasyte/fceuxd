static void DrawDips(void)
{
  uint32 *dest;
  int y,x;

  dest=(uint32 *)(XBuf+272*12+164);
  for(y=24;y;y--,dest+=(272-72)>>2)
  {
   for(x=72>>2;x;x--,dest++)
    *dest=0x80808080;
  }

  dest=(uint32 *)(XBuf+272*(12+4)+164+6 );
  for(y=16;y;y--,dest+=(272>>2)-16)
   for(x=8;x;x--)
   {
    *dest=0x81818181;
    dest+=2;
   }   

  dest=(uint32 *)(XBuf+272*(12+4)+164+6 );
  for(x=0;x<8;x++,dest+=2)
  {
   uint32 *da=dest+(272>>2);

   if(!((vsdip>>x)&1))
    da+=(272>>2)*10;
    
   for(y=4;y;y--,da+=272>>2)
    *da=0x80808080;
   
  }
}

static void DrawMessage(void)
{
 if(howlong)
  {
   uint8 *t;
   howlong--;
   t=XBuf+(FSettings.LastSLine-29)*272+32;
   if(t>=XBuf)
    DrawTextTrans(t,272,errmsg,132);
  }
}

uint8 sstat[2541] =
{
0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x81,0x81,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x80,0x80,0x80,
0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x81,0x81,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x83,0x80,0x80,0x81,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x83,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x83,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x83,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x83,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x81,0x81,0x81,0x81,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x80,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x81,0x81,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x83,0x80,0x80,0x81,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x81,0x83,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x81,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x81,0x81,0x83,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x83,0x80,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x81,0x81,0x81,0x81,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x81,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
0x80,0x80,0x80,0x80,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x81,0x81,0x81,0x81,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x80,0x80,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x81,0x81,0x81,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x80,0x81,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x81,0x81,0x83,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x81,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x83,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x81,0x81,0x81,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x80,0x80,0x81,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x81,0x81,0x83,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x83,0x83,
0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x81,0x81,0x81,0x81,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x80,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x83,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x83,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x83,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,
0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x81,0x81,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x83,0x80,0x80,0x81,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x81,0x81,0x83,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x83,0x80,0x80,0x81,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x81,0x81,0x83,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,
0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x81,0x81,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x80,0x80,0x81,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x81,0x81,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x83,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x81,0x83,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,
0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x81,0x81,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x83,0x80,0x80,0x81,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x81,0x81,0x83,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80
};

uint8 fontdata2[2048] =
{
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7e,0x81,0xa5,0x81,0xbd,0x99,0x81,0x7e,0x7e,0xff,0xdb,0xff,0xc3,0xe7,0xff,0x7e,0x36,0x7f,0x7f,0x7f,0x3e,0x1c,0x08,0x00,0x08,0x1c,0x3e,0x7f,0x3e,0x1c,0x08,0x00,0x1c,0x3e,0x1c,0x7f,0x7f,0x3e,0x1c,0x3e,0x08,0x08,0x1c,0x3e,0x7f,0x3e,0x1c,0x3e,0x00,0x00,0x18,0x3c,0x3c,0x18,0x00,0x00,0xff,0xff,0xe7,0xc3,0xc3,0xe7,0xff,0xff,0x00,0x3c,0x66,0x42,0x42,0x66,0x3c,0x00,0xff,0xc3,0x99,0xbd,0xbd,0x99,0xc3,0xff,0xf0,0xe0,0xf0,0xbe,0x33,0x33,0x33,0x1e,0x3c,0x66,0x66,0x66,0x3c,0x18,0x7e,0x18,0xfc,0xcc,0xfc,0x0c,0x0c,0x0e,0x0f,0x07,0xfe,0xc6,0xfe,0xc6,0xc6,0xe6,0x67,0x03,0x99,0x5a,0x3c,0xe7,0xe7,0x3c,0x5a,0x99,0x01,0x07,0x1f,0x7f,0x1f,0x07,0x01,0x00,0x40,0x70,0x7c,0x7f,0x7c,0x70,0x40,0x00,0x18,0x3c,0x7e,0x18,0x18,0x7e,0x3c,0x18,0x66,0x66,0x66,0x66,0x66,0x00,0x66,0x00,0xfe,0xdb,0xdb,0xde,0xd8,0xd8,0xd8,0x00,0x7c,0xc6,0x1c,0x36,0x36,0x1c,0x33,0x1e,0x00,0x00,0x00,0x00,0x7e,0x7e,0x7e,0x00,0x18,0x3c,0x7e,0x18,0x7e,0x3c,0x18,0xff,0x18,0x3c,0x7e,0x18,0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x18,0x7e,0x3c,0x18,0x00,0x00,0x18,0x30,0x7f,0x30,0x18,0x00,0x00,0x00,0x0c,0x06,0x7f,0x06,0x0c,0x00,0x00,0x00,0x00,0x03,0x03,0x03,0x7f,0x00,0x00,0x00,0x24,0x66,0xff,0x66,0x24,0x00,0x00,0x00,0x18,0x3c,0x7e,0xff,0xff,0x00,0x00,0x00,0xff,0xff,0x7e,0x3c,0x18,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0c,0x1e,0x1e,0x0c,0x0c,0x00,0x0c,0x00,0x36,0x36,0x36,0x00,0x00,0x00,0x00,0x00,0x36,0x36,0x7f,0x36,0x7f,0x36,0x36,0x00,0x0c,0x3e,0x03,0x1e,0x30,0x1f,0x0c,0x00,0x00,0x63,0x33,0x18,0x0c,0x66,0x63,0x00,0x1c,0x36,0x1c,0x6e,0x3b,0x33,0x6e,0x00,0x06,0x06,0x03,0x00,0x00,0x00,0x00,0x00,0x18,0x0c,0x06,0x06,0x06,0x0c,0x18,0x00,0x06,0x0c,0x18,0x18,0x18,0x0c,0x06,0x00,0x00,0x66,0x3c,0xff,0x3c,0x66,0x00,0x00,0x00,0x0c,0x0c,0x3f,0x0c,0x0c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0c,0x0c,0x06,0x00,0x00,0x00,0x3f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0c,0x0c,0x00,0x60,0x30,0x18,0x0c,0x06,0x03,0x01,0x00,0x3e,0x63,0x73,0x7b,0x6f,0x67,0x3e,0x00,0x0c,0x0e,0x0c,0x0c,0x0c,0x0c,0x3f,0x00,0x1e,0x33,0x30,0x1c,0x06,0x33,0x3f,0x00,0x1e,0x33,0x30,0x1c,0x30,0x33,0x1e,0x00,0x38,0x3c,0x36,0x33,0x7f,0x30,0x78,0x00,0x3f,0x03,0x1f,0x30,0x30,0x33,0x1e,0x00,0x1c,0x06,0x03,0x1f,0x33,0x33,0x1e,0x00,0x3f,0x33,0x30,0x18,0x0c,0x0c,0x0c,0x00,0x1e,0x33,0x33,0x1e,0x33,0x33,0x1e,0x00,0x1e,0x33,0x33,0x3e,0x30,0x18,0x0e,0x00,0x00,0x0c,0x0c,0x00,0x00,0x0c,0x0c,0x00,0x00,0x0c,0x0c,0x00,0x00,0x0c,0x0c,0x06,0x18,0x0c,0x06,0x03,0x06,0x0c,0x18,0x00,0x00,0x00,0x3f,0x00,0x00,0x3f,0x00,0x00,0x06,0x0c,0x18,0x30,0x18,0x0c,0x06,0x00,0x1e,0x33,0x30,0x18,0x0c,0x00,0x0c,0x00,
0x3e,0x63,0x7b,0x7b,0x7b,0x03,0x1e,0x00,0x0c,0x1e,0x33,0x33,0x3f,0x33,0x33,0x00,0x3f,0x66,0x66,0x3e,0x66,0x66,0x3f,0x00,0x3c,0x66,0x03,0x03,0x03,0x66,0x3c,0x00,0x1f,0x36,0x66,0x66,0x66,0x36,0x1f,0x00,0x7f,0x46,0x16,0x1e,0x16,0x46,0x7f,0x00,0x7f,0x46,0x16,0x1e,0x16,0x06,0x0f,0x00,0x3c,0x66,0x03,0x03,0x73,0x66,0x7c,0x00,0x33,0x33,0x33,0x3f,0x33,0x33,0x33,0x00,0x1e,0x0c,0x0c,0x0c,0x0c,0x0c,0x1e,0x00,0x78,0x30,0x30,0x30,0x33,0x33,0x1e,0x00,0x67,0x66,0x36,0x1e,0x36,0x66,0x67,0x00,0x0f,0x06,0x06,0x06,0x46,0x66,0x7f,0x00,0x63,0x77,0x7f,0x7f,0x6b,0x63,0x63,0x00,0x63,0x67,0x6f,0x7b,0x73,0x63,0x63,0x00,0x1c,0x36,0x63,0x63,0x63,0x36,0x1c,0x00,0x3f,0x66,0x66,0x3e,0x06,0x06,0x0f,0x00,0x1e,0x33,0x33,0x33,0x3b,0x1e,0x38,0x00,0x3f,0x66,0x66,0x3e,0x36,0x66,0x67,0x00,0x1e,0x33,0x07,0x0e,0x38,0x33,0x1e,0x00,0x3f,0x2d,0x0c,0x0c,0x0c,0x0c,0x1e,0x00,0x33,0x33,0x33,0x33,0x33,0x33,0x3f,0x00,0x33,0x33,0x33,0x33,0x33,0x1e,0x0c,0x00,0x63,0x63,0x63,0x6b,0x7f,0x77,0x63,0x00,0x63,0x63,0x36,0x1c,0x1c,0x36,0x63,0x00,0x33,0x33,0x33,0x1e,0x0c,0x0c,0x1e,0x00,0x7f,0x63,0x31,0x18,0x4c,0x66,0x7f,0x00,0x1e,0x06,0x06,0x06,0x06,0x06,0x1e,0x00,0x03,0x06,0x0c,0x18,0x30,0x60,0x40,0x00,0x1e,0x18,0x18,0x18,0x18,0x18,0x1e,0x00,0x08,0x1c,0x36,0x63,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,
0x0c,0x0c,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x1e,0x30,0x3e,0x33,0x6e,0x00,0x07,0x06,0x06,0x3e,0x66,0x66,0x3b,0x00,0x00,0x00,0x1e,0x33,0x03,0x33,0x1e,0x00,0x38,0x30,0x30,0x3e,0x33,0x33,0x6e,0x00,0x00,0x00,0x1e,0x33,0x3f,0x03,0x1e,0x00,0x1c,0x36,0x06,0x0f,0x06,0x06,0x0f,0x00,0x00,0x00,0x6e,0x33,0x33,0x3e,0x30,0x1f,0x07,0x06,0x36,0x6e,0x66,0x66,0x67,0x00,0x0c,0x00,0x0e,0x0c,0x0c,0x0c,0x1e,0x00,0x30,0x00,0x30,0x30,0x30,0x33,0x33,0x1e,0x07,0x06,0x66,0x36,0x1e,0x36,0x67,0x00,0x0e,0x0c,0x0c,0x0c,0x0c,0x0c,0x1e,0x00,0x00,0x00,0x33,0x7f,0x7f,0x6b,0x63,0x00,0x00,0x00,0x1f,0x33,0x33,0x33,0x33,0x00,0x00,0x00,0x1e,0x33,0x33,0x33,0x1e,0x00,0x00,0x00,0x3b,0x66,0x66,0x3e,0x06,0x0f,0x00,0x00,0x6e,0x33,0x33,0x3e,0x30,0x78,0x00,0x00,0x3b,0x6e,0x66,0x06,0x0f,0x00,0x00,0x00,0x3e,0x03,0x1e,0x30,0x1f,0x00,0x08,0x0c,0x3e,0x0c,0x0c,0x2c,0x18,0x00,0x00,0x00,0x33,0x33,0x33,0x33,0x6e,0x00,0x00,0x00,0x33,0x33,0x33,0x1e,0x0c,0x00,0x00,0x00,0x63,0x6b,0x7f,0x7f,0x36,0x00,0x00,0x00,0x63,0x36,0x1c,0x36,0x63,0x00,0x00,0x00,0x33,0x33,0x33,0x3e,0x30,0x1f,0x00,0x00,0x3f,0x19,0x0c,0x26,0x3f,0x00,0x38,0x0c,0x0c,0x07,0x0c,0x0c,0x38,0x00,0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x00,0x07,0x0c,0x0c,0x38,0x0c,0x0c,0x07,0x00,0x6e,0x3b,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x1c,0x36,0x63,0x63,0x7f,0x00,
0x1e,0x33,0x03,0x33,0x1e,0x18,0x30,0x1e,0x00,0x33,0x00,0x33,0x33,0x33,0x7e,0x00,0x38,0x00,0x1e,0x33,0x3f,0x03,0x1e,0x00,0x7e,0xc3,0x3c,0x60,0x7c,0x66,0xfc,0x00,0x33,0x00,0x1e,0x30,0x3e,0x33,0x7e,0x00,0x07,0x00,0x1e,0x30,0x3e,0x33,0x7e,0x00,0x0c,0x0c,0x1e,0x30,0x3e,0x33,0x7e,0x00,0x00,0x00,0x1e,0x03,0x03,0x1e,0x30,0x1c,0x7e,0xc3,0x3c,0x66,0x7e,0x06,0x3c,0x00,0x33,0x00,0x1e,0x33,0x3f,0x03,0x1e,0x00,0x07,0x00,0x1e,0x33,0x3f,0x03,0x1e,0x00,0x33,0x00,0x0e,0x0c,0x0c,0x0c,0x1e,0x00,0x3e,0x63,0x1c,0x18,0x18,0x18,0x3c,0x00,0x07,0x00,0x0e,0x0c,0x0c,0x0c,0x1e,0x00,0x63,0x1c,0x36,0x63,0x7f,0x63,0x63,0x00,0x0c,0x0c,0x00,0x1e,0x33,0x3f,0x33,0x00,0x38,0x00,0x3f,0x06,0x1e,0x06,0x3f,0x00,0x00,0x00,0xfe,0x30,0xfe,0x33,0xfe,0x00,0x7c,0x36,0x33,0x7f,0x33,0x33,0x73,0x00,0x1e,0x33,0x00,0x1e,0x33,0x33,0x1e,0x00,0x00,0x33,0x00,0x1e,0x33,0x33,0x1e,0x00,0x00,0x07,0x00,0x1e,0x33,0x33,0x1e,0x00,0x1e,0x33,0x00,0x33,0x33,0x33,0x7e,0x00,0x00,0x07,0x00,0x33,0x33,0x33,0x7e,0x00,0x00,0x33,0x00,0x33,0x33,0x3e,0x30,0x1f,0xc3,0x18,0x3c,0x66,0x66,0x3c,0x18,0x00,0x33,0x00,0x33,0x33,0x33,0x33,0x1e,0x00,0x18,0x18,0x7e,0x03,0x03,0x7e,0x18,0x18,0x1c,0x36,0x26,0x0f,0x06,0x67,0x3f,0x00,0x33,0x33,0x1e,0x3f,0x0c,0x3f,0x0c,0x0c,0x1f,0x33,0x33,0x5f,0x63,0xf3,0x63,0xe3,0x70,0xd8,0x18,0x3c,0x18,0x18,0x1b,0x0e,
0x38,0x00,0x1e,0x30,0x3e,0x33,0x7e,0x00,0x1c,0x00,0x0e,0x0c,0x0c,0x0c,0x1e,0x00,0x00,0x38,0x00,0x1e,0x33,0x33,0x1e,0x00,0x00,0x38,0x00,0x33,0x33,0x33,0x7e,0x00,0x00,0x1f,0x00,0x1f,0x33,0x33,0x33,0x00,0x3f,0x00,0x33,0x37,0x3f,0x3b,0x33,0x00,0x3c,0x36,0x36,0x7c,0x00,0x7e,0x00,0x00,0x1c,0x36,0x36,0x1c,0x00,0x3e,0x00,0x00,0x0c,0x00,0x0c,0x06,0x03,0x33,0x1e,0x00,0x00,0x00,0x00,0x3f,0x03,0x03,0x00,0x00,0x00,0x00,0x00,0x3f,0x30,0x30,0x00,0x00,0xc3,0x63,0x33,0x7b,0xcc,0x66,0x33,0xf0,0xc3,0x63,0x33,0xdb,0xec,0xf6,0xf3,0xc0,0x18,0x18,0x00,0x18,0x18,0x18,0x18,0x00,0x00,0xcc,0x66,0x33,0x66,0xcc,0x00,0x00,0x00,0x33,0x66,0xcc,0x66,0x33,0x00,0x00,0x44,0x11,0x44,0x11,0x44,0x11,0x44,0x11,0xaa,0x55,0xaa,0x55,0xaa,0x55,0xaa,0x55,0xdb,0xee,0xdb,0x77,0xdb,0xee,0xdb,0x77,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x1f,0x18,0x18,0x18,0x18,0x18,0x1f,0x18,0x1f,0x18,0x18,0x18,0x6c,0x6c,0x6c,0x6c,0x6f,0x6c,0x6c,0x6c,0x00,0x00,0x00,0x00,0x7f,0x6c,0x6c,0x6c,0x00,0x00,0x1f,0x18,0x1f,0x18,0x18,0x18,0x6c,0x6c,0x6f,0x60,0x6f,0x6c,0x6c,0x6c,0x6c,0x6c,0x6c,0x6c,0x6c,0x6c,0x6c,0x6c,0x00,0x00,0x7f,0x60,0x6f,0x6c,0x6c,0x6c,0x6c,0x6c,0x6f,0x60,0x7f,0x00,0x00,0x00,0x6c,0x6c,0x6c,0x6c,0x7f,0x00,0x00,0x00,0x18,0x18,0x1f,0x18,0x1f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x1f,0x18,0x18,0x18,
0x18,0x18,0x18,0x18,0xf8,0x00,0x00,0x00,0x18,0x18,0x18,0x18,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0xf8,0x18,0x18,0x18,0x00,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0x18,0x18,0x18,0x18,0xff,0x18,0x18,0x18,0x18,0x18,0xf8,0x18,0xf8,0x18,0x18,0x18,0x6c,0x6c,0x6c,0x6c,0xec,0x6c,0x6c,0x6c,0x6c,0x6c,0xec,0x0c,0xfc,0x00,0x00,0x00,0x00,0x00,0xfc,0x0c,0xec,0x6c,0x6c,0x6c,0x6c,0x6c,0xef,0x00,0xff,0x00,0x00,0x00,0x00,0x00,0xff,0x00,0xef,0x6c,0x6c,0x6c,0x6c,0x6c,0xec,0x0c,0xec,0x6c,0x6c,0x6c,0x00,0x00,0xff,0x00,0xff,0x00,0x00,0x00,0x6c,0x6c,0xef,0x00,0xef,0x6c,0x6c,0x6c,0x18,0x18,0xff,0x00,0xff,0x00,0x00,0x00,0x6c,0x6c,0x6c,0x6c,0xff,0x00,0x00,0x00,0x00,0x00,0xff,0x00,0xff,0x18,0x18,0x18,0x00,0x00,0x00,0x00,0xff,0x6c,0x6c,0x6c,0x6c,0x6c,0x6c,0x6c,0xfc,0x00,0x00,0x00,0x18,0x18,0xf8,0x18,0xf8,0x00,0x00,0x00,0x00,0x00,0xf8,0x18,0xf8,0x18,0x18,0x18,0x00,0x00,0x00,0x00,0xfc,0x6c,0x6c,0x6c,0x6c,0x6c,0x6c,0x6c,0xff,0x6c,0x6c,0x6c,0x18,0x18,0xff,0x18,0xff,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x1f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xf8,0x18,0x18,0x18,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x00,0x00,0x00,0xff,0xff,0xff,0xff,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xff,0xff,0xff,0xff,0x00,0x00,0x00,0x00,
0x00,0x00,0x6e,0x3b,0x13,0x3b,0x6e,0x00,0x00,0x1e,0x33,0x1f,0x33,0x1f,0x03,0x03,0x00,0x3f,0x33,0x03,0x03,0x03,0x03,0x00,0x00,0x7f,0x36,0x36,0x36,0x36,0x36,0x00,0x3f,0x33,0x06,0x0c,0x06,0x33,0x3f,0x00,0x00,0x00,0x7e,0x1b,0x1b,0x1b,0x0e,0x00,0x00,0x66,0x66,0x66,0x66,0x3e,0x06,0x03,0x00,0x6e,0x3b,0x18,0x18,0x18,0x18,0x00,0x3f,0x0c,0x1e,0x33,0x33,0x1e,0x0c,0x3f,0x1c,0x36,0x63,0x7f,0x63,0x36,0x1c,0x00,0x1c,0x36,0x63,0x63,0x36,0x36,0x77,0x00,0x38,0x0c,0x18,0x3e,0x33,0x33,0x1e,0x00,0x00,0x00,0x7e,0xdb,0xdb,0x7e,0x00,0x00,0x60,0x30,0x7e,0xdb,0xdb,0x7e,0x06,0x03,0x1c,0x06,0x03,0x1f,0x03,0x06,0x1c,0x00,0x1e,0x33,0x33,0x33,0x33,0x33,0x33,0x00,0x00,0x3f,0x00,0x3f,0x00,0x3f,0x00,0x00,0x0c,0x0c,0x3f,0x0c,0x0c,0x00,0x3f,0x00,0x06,0x0c,0x18,0x0c,0x06,0x00,0x3f,0x00,0x18,0x0c,0x06,0x0c,0x18,0x00,0x3f,0x00,0x70,0xd8,0xd8,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x1b,0x1b,0x0e,0x0c,0x0c,0x00,0x3f,0x00,0x0c,0x0c,0x00,0x00,0x6e,0x3b,0x00,0x6e,0x3b,0x00,0x00,0x1c,0x36,0x36,0x1c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x00,0x00,0x00,0xf0,0x30,0x30,0x30,0x37,0x36,0x3c,0x38,0x1e,0x36,0x36,0x36,0x36,0x00,0x00,0x00,0x0e,0x18,0x0c,0x06,0x1e,0x00,0x00,0x00,0x00,0x00,0x3c,0x3c,0x3c,0x3c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

static void DrawState(void)
{
 uint8 *XBaf;
 int x,y,z;


 XBaf=XBuf+4+(FSettings.LastSLine-44)*272;
 if(XBaf>=XBuf)
 for(z=1;z<11;z++)
 {
  if(SaveStateStatus[z%10]) 
   {
          for(y=0;y<13;y++)
           for(x=0;x<21;x++)
           XBaf[y*272+x+z*21+z]=sstat[y*21+x+(z-1)*21*12];           
   } else {
          for(y=0;y<13;y++)
           for(x=0;x<21;x++)
            if(sstat[y*21+x+(z-1)*21*12]!=0x83)
             XBaf[y*272+x+z*21+z]=sstat[y*21+x+(z-1)*21*12];
   }
  if(CurrentState==z%10)
   {
    for(x=0;x<21;x++)
     XBaf[x+z*21+z*1]=132;
    for(x=1;x<12;x++)
     {
     XBaf[272*x+z*21+z*1]=
     XBaf[272*x+z*21+z*1+20]=132;
     }
    for(x=0;x<21;x++)
     XBaf[3264+x+z*21+z*1]=132;
   }
  } 
 StateShow--;
}

void DrawTextTrans(uint8 *dest, uint32 width, uint8 *textmsg, uint8 fgcolor)
{
	uint8 length=strlen(textmsg);
	uint8 x;
	uint8 y;
	uint8 z;

	for(x=0;x<length;x++)    
         for(y=0;y<8;y++)       
          for(z=0;z<8;z++) 
           if((fontdata2[(textmsg[x]<<3)+y]>>z)&1) dest[y*width+(x<<3)+z]=fgcolor;
}