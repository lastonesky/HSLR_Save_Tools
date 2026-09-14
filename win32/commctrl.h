/* commctrl.h — Minimal stub for TCC build (HSLR Save Editor) */
#ifndef _COMMCTRL_H_
#define _COMMCTRL_H_

#include <windows.h>

/* SNDMSG may be missing in TCC */
#ifndef SNDMSG
#define SNDMSG SendMessage
#endif

/* Common control classes */
#define ICC_LISTVIEW_CLASSES 0x00000001
#define ICC_TAB_CLASSES      0x00000008
#define ICC_BAR_CLASSES      0x00000004
#define ICC_WIN95_CLASSES    0x000000FF

typedef struct tagINITCOMMONCONTROLSEX {
    DWORD dwSize;
    DWORD dwICC;
} INITCOMMONCONTROLSEX;

BOOL WINAPI InitCommonControlsEx(const INITCOMMONCONTROLSEX *);

/* InitCommonControls */
void WINAPI InitCommonControls(void);

/* ---- Tab Control ---- */
#define WC_TABCTRLA "SysTabControl32"

#define TCM_FIRST           0x1300
#define TCM_INSERTITEM      (TCM_FIRST + 62)
#define TCM_ADJUSTRECT      (TCM_FIRST + 40)
#define TCN_FIRST           (-552)
#define TCN_SELCHANGE       (TCN_FIRST - 1)
#define TCN_SELCHANGING     (TCN_FIRST - 2)

#define TCS_FIXEDWIDTH      0x00000400

typedef struct tagTCITEMA {
    UINT mask;
    DWORD dwState;
    DWORD dwStateMask;
    LPSTR pszText;
    int cchTextMax;
    int iImage;
    LPARAM lParam;
} TCITEMA;

typedef struct tagTCITEMW {
    UINT mask;
    DWORD dwState;
    DWORD dwStateMask;
    LPWSTR pszText;
    int cchTextMax;
    int iImage;
    LPARAM lParam;
} TCITEMW;

#define TCIF_TEXT    0x0001
#define TCIF_IMAGE   0x0002

#define TabCtrl_InsertItem(hwnd, i, pitem) \
    (int)SNDMSG((hwnd), TCM_INSERTITEM, (WPARAM)(int)(i), (LPARAM)(const TCITEMA *)(pitem))
#define TabCtrl_GetCurSel(hwnd) \
    (int)SNDMSG((hwnd), TCM_GETCURSEL, 0, 0)
#define TabCtrl_AdjustRect(hwnd, fLarger, prc) \
    (int)SNDMSG((hwnd), TCM_ADJUSTRECT, (WPARAM)(BOOL)(fLarger), (LPARAM)(LPRECT)(prc))

/* ---- ListView Control ---- */
#define WC_LISTVIEWA        "SysListView32"

#define LVM_FIRST           0x1000
#define LVM_GETITEMCOUNT    (LVM_FIRST + 4)
#define LVM_DELETEALLITEMS  (LVM_FIRST + 9)
#define LVM_INSERTITEMA     (LVM_FIRST + 7)
#define LVM_SETITEMTEXTA    (LVM_FIRST + 45)
#define LVM_GETITEMTEXTA    (LVM_FIRST + 45)
#define LVM_SETEXTENDEDLISTVIEWSTYLE (LVM_FIRST + 54)
#define LVM_GETNEXTITEM     (LVM_FIRST + 12)

#define LVS_REPORT          0x0001
#define LVS_SHOWSELALWAYS   0x0008
#define LVS_EX_FULLROWSELECT 0x00000020
#define LVS_EX_GRIDLINES    0x00000001

#define LVIF_TEXT    0x0001

#define LVNI_SELECTED 0x0002

typedef struct tagLVITEMA {
    UINT mask;
    int iItem;
    int iSubItem;
    UINT state;
    UINT stateMask;
    LPSTR pszText;
    int cchTextMax;
    int iImage;
    LPARAM lParam;
} LVITEMA;

typedef struct tagLVCOLUMNA {
    UINT mask;
    int fmt;
    int cx;
    LPSTR pszText;
    int cchTextMax;
    int iSubItem;
} LVCOLUMNA;

#define LVCF_TEXT   0x0001
#define LVCF_WIDTH  0x0002
#define LVCF_FMT    0x0004
#define LVCFMT_LEFT 0x0000

#define ListView_InsertItem(hwnd, pitem) \
    (int)SNDMSG((hwnd), LVM_INSERTITEMA, 0, (LPARAM)(const LVITEMA *)(pitem))
#define ListView_DeleteAllItems(hwnd) \
    (BOOL)SNDMSG((hwnd), LVM_DELETEALLITEMS, 0, 0)
#define ListView_SetItemText(hwnd, i, iSubItem, psz) \
    SNDMSG((hwnd), LVM_SETITEMTEXTA, (WPARAM)(int)(i), \
           ((LPARAM)(LPSTR)(psz) | ((LPARAM)(int)(iSubItem) << 16)))
#define ListView_GetItemText(hwnd, i, iSubItem, psz, cchMax) \
    SNDMSG((hwnd), LVM_GETITEMTEXTA, (WPARAM)(int)(i), \
           ((LPARAM)(LPSTR)(psz) | ((LPARAM)(int)(cchMax) << 16)))
#define ListView_GetItemCount(hwnd) \
    (int)SNDMSG((hwnd), LVM_GETITEMCOUNT, 0, 0)
#define ListView_GetNextItem(hwnd, i, flags) \
    (int)SNDMSG((hwnd), LVM_GETNEXTITEM, (WPARAM)(int)(i), MAKELPARAM((flags), 0))
#define ListView_InsertColumn(hwnd, iCol, pcol) \
    (int)SNDMSG((hwnd), LVM_INSERTCOLUMNA, (WPARAM)(int)(iCol), (LPARAM)(const LVCOLUMNA *)(pcol))

/* NM_LISTVIEW notification */
typedef struct tagNMLISTVIEW {
    NMHDR hdr;
    int iItem;
    int iSubItem;
    UINT uNewState;
    UINT uOldState;
    UINT uChanged;
    POINT ptAction;
    LPARAM lParam;
} NMLISTVIEW;

#define LVN_ITEMCHANGED (-101)

/* ---- Status Bar ---- */
#define STATUSCLASSNAMEA    "msctls_statusbar32"
#define SB_SETTEXTA         (WM_USER + 1)
#define SBARS_SIZEGRIP      0x0100

/* ---- ListView Column Insert ---- */
#ifndef LVM_INSERTCOLUMNA
#define LVM_INSERTCOLUMNA   (LVM_FIRST + 27)
#endif

/* ---- Notification macros ---- */
#define ListView_InsertColumn(hwnd, iCol, pcol) \
    (int)SNDMSG((hwnd), LVM_INSERTCOLUMNA, (WPARAM)(int)(iCol), (LPARAM)(const LVCOLUMNA *)(pcol))

#endif /* _COMMCTRL_H_ */
