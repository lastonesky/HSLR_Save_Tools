/* commdlg.h — Minimal stub for TCC build (HSLR Save Editor) */
#ifndef _COMMDLG_H_
#define _COMMDLG_H_

#include <windows.h>

/* GetOpenFileName / GetSaveFileName */
typedef struct tagOFNA {
    DWORD lStructSize;
    HWND hwndOwner;
    HINSTANCE hInstance;
    LPCSTR lpstrFilter;
    LPSTR lpstrCustomFilter;
    DWORD nMaxCustFilter;
    DWORD nFilterIndex;
    LPSTR lpstrFile;
    DWORD nMaxFile;
    LPSTR lpstrFileTitle;
    DWORD nMaxFileTitle;
    LPCSTR lpstrInitialDir;
    LPCSTR lpstrTitle;
    DWORD Flags;
    WORD nFileOffset;
    WORD nFileExtension;
    LPCSTR lpstrDefExt;
    LPARAM lCustData;
    UINT_PTR (CALLBACK *lpfnHook)(HWND, UINT, WPARAM, LPARAM);
    LPCSTR lpTemplateName;
} OPENFILENAMEA;

#define OFN_FILEMUSTEXIST  0x00001000
#define OFN_PATHMUSTEXIST  0x00000800
#define OFN_OVERWRITEPROMPT 0x00000200

BOOL WINAPI GetOpenFileNameA(LPOPENFILENAMEA);
BOOL WINAPI GetSaveFileNameA(LPOPENFILENAMEA);

#endif /* _COMMDLG_H_ */
