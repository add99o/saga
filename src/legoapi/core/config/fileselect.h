#pragma once

#include "nu2api/nucore/common.h"

struct nupad_s;

extern char FS_Title[64];
extern char FS_Path[256];
extern char FS_Filter[256];
extern char FS_FilterOut[256];
extern char FS_LastFileName[64];
extern u8 FS_Active;
extern u8 FS_RefreshDir;
extern u8 FS_ShowVolumes;
extern f32 FS_X, FS_Y, FS_Width, FS_W, FS_H;
extern void (*FS_Callback)(char *path, char *name);

i32 ProcessFileSel3(f32 elapsed, nupad_s *pad);
void ProcessFileSel(f32 elapsed, nupad_s *pad);
void RenderFileSel(void);
void RenderFileSel3(i32 draw_background);

extern "C" {
    void StartFileSel(char *title, char *path, char *filter, char *filter_out,
                      void (*callback)(char *path, char *name));
    void FileSelKill(void);
    i32 ProcessFileSel2(f32 elapsed, nupad_s *pad);
    void RenderFileSel2(i32 x, i32 y, i32 *width, i32 *height);
}
