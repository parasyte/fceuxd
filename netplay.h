#ifdef NETWORK
int InitNetplay(void);
void KillNetplay(void);
void NetplayUpdate(uint8 *joyp);

extern int netplay;
#endif
