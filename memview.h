void DoMemView();
void UpdateMemoryView(int draw_all);
void UpdateColorTable();
void ChangeMemViewFocus(int EditingMode,int Offset);

void ApplyPatch(int addr,int size, uint8* data);
void UndoLastPatch();
int GetFileData(int offset);
int WriteFileData(int offset,int data);
int GetRomFileSize();
void FlushUndoBuffer();

extern HWND hMemView;
extern int EditingMode;
