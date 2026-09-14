/*
 * hslr_gui.c — Win32 GUI for HSLR Save Editor
 *
 * Implements all 7 tab pages:
 *  1. Basic Info (基础信息)
 *  2. Record Attrs (存档属性)
 *  3. Battle Attrs (战场属性)
 *  4. Equipment (装备)
 *  5. Items (物品)
 *  6. Skills (技能)
 *  7. Roster (全角色一览)
 */
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hslr_data.h"
#include "cJSON.h"

#pragma comment(lib, "comctl32.lib")

/* ============================================================
 * Window/control IDs
 * ============================================================ */
#define ID_TAB          1000
#define ID_CHAR_COMBO   1001
#define ID_BTN_OPEN     1002
#define ID_BTN_SAVE     1003
#define ID_BTN_REFRESH  1004
#define ID_BTN_BATCH    1005

/* Tab page 1 — Basic Info */
#define ID_ED_LEVEL     1100
#define ID_ED_EXP       1101

/* Tab page 2 — Record Attrs (BaseAttr) */
#define ID_ED_BASE_STR   1200
#define ID_ED_BASE_DEX   1201
#define ID_ED_BASE_MIND  1202
#define ID_ED_BASE_CON   1203
#define ID_ED_BASE_HP    1204
#define ID_ED_BASE_MP    1205

/* Record Attrs (PermFightAttr) */
#define ID_ED_PERM_STR   1210
#define ID_ED_PERM_DEX   1211
#define ID_ED_PERM_MIND  1212
#define ID_ED_PERM_CON   1213
#define ID_ED_PERM_MAXHP 1214
#define ID_ED_PERM_MAXMP 1215
#define ID_ED_PERM_PATK  1216
#define ID_ED_PERM_MATK  1217
#define ID_ED_PERM_DEF   1218
#define ID_ED_PERM_SPD   1219
#define ID_ED_PERM_CRIT  1220
#define ID_ED_PERM_DODGE 1221

/* Record Attrs (FightAttr — read-only) */
#define ID_ED_FIGHT_HP    1230
#define ID_ED_FIGHT_MAXHP 1231
#define ID_ED_FIGHT_MP    1232
#define ID_ED_FIGHT_MAXMP 1233
#define ID_ED_FIGHT_STR   1234
#define ID_ED_FIGHT_DEX   1235
#define ID_ED_FIGHT_MIND  1236
#define ID_ED_FIGHT_CON   1237
#define ID_ED_FIGHT_PATK  1238
#define ID_ED_FIGHT_MATK  1239
#define ID_ED_FIGHT_DEF   1240
#define ID_ED_FIGHT_SPD   1241
#define ID_ED_FIGHT_MOVE  1242
#define ID_ED_FIGHT_CRIT  1243
#define ID_ED_FIGHT_DODGE 1244
#define ID_ED_FIGHT_FRES  1245
#define ID_ED_FIGHT_WRES  1246
#define ID_ED_FIGHT_ARES  1247
#define ID_ED_FIGHT_ERES  1248
#define ID_ED_FIGHT_MRES  1249

/* Tab page 3 — Battle Attrs */
#define ID_ED_BAT_HP    1300
#define ID_ED_BAT_MAXHP 1301
#define ID_ED_BAT_MP    1302
#define ID_ED_BAT_MAXMP 1303
#define ID_ED_BAT_LV    1304
#define ID_ED_BAT_EXP   1305
#define ID_ED_BAT_STR   1306
#define ID_ED_BAT_DEX   1307
#define ID_ED_BAT_MIND  1308
#define ID_ED_BAT_CON   1309
#define ID_ED_BAT_PATK  1310
#define ID_ED_BAT_MATK  1311
#define ID_ED_BAT_DEF   1312
#define ID_ED_BAT_SPD   1313
#define ID_ED_BAT_MOVE  1314
#define ID_ED_BAT_CRIT  1315
#define ID_ED_BAT_DODGE 1316
#define ID_BTN_FULL_HEAL 1320
#define ID_BTN_MAX_BATTLE 1321
#define ID_BTN_MAX_LEVEL  1322

/* Tab page 4 — Equipment */
#define ID_CB_EQUIP0    1400
#define ID_CB_EQUIP1    1401
#define ID_CB_EQUIP2    1402
#define ID_CB_EQUIP3    1403
#define ID_CB_EQUIP4    1404
#define ID_CB_EQUIP5    1405
#define ID_BTN_CLR_EQUIP 1410
#define ID_TXT_EQUIP_INFO 1411

/* Tab page 5 — Items */
#define ID_LV_BAG       1500
#define ID_CB_BAG_ADD   1501
#define ID_ED_BAG_QTY   1502
#define ID_BTN_BAG_ADD  1503
#define ID_BTN_BAG_DEL  1504
#define ID_LV_STORAGE   1510
#define ID_CB_ST_ADD    1511
#define ID_ED_ST_QTY    1512
#define ID_BTN_ST_ADD   1513
#define ID_BTN_ST_DEL   1514
#define ID_TXT_ITEM_INFO 1520

/* Tab page 6 — Skills */
#define ID_ED_SKILL_NRL   1600
#define ID_ED_SKILL_MAGIC 1601
#define ID_ED_SKILL_SP    1602

/* Tab page 7 — Roster */
#define ID_LV_ROSTER    1700

/* Status bar */
#define ID_STATUS       1800

/* ============================================================
 * Global state
 * ============================================================ */
static EditorState g_ed;

/* Window handles */
static HWND g_hwndMain;
static HWND g_hwndTab;
static HWND g_hwndCharCombo;

/* Tab page parent windows */
static HWND g_tabPages[7];
static int  g_currentTab;

/* Tab page 2 controls */
static HWND g_edBase[6];      /* Str,Dex,Mind,Con,Hp,Mp */
static HWND g_edPerm[12];     /* Str,Dex,Mind,Con,MaxHp,MaxMp,PAtk,MAtk,Def,Spd,Crit,Dodge */
static HWND g_edFight[20];    /* Hp,MaxHp,Mp,MaxMp,Str,Dex,Mind,Con,PAtk,MAtk,Def,Spd,Move,Crit,Dodge,FRes,WRes,ARes,ERes,MRes */

/* Tab page 3 controls */
static HWND g_edBat[17];      /* Hp,MaxHp,Mp,MaxMp,Lv,Exp,Str,Dex,Mind,Con,PAtk,MAtk,Def,Spd,Move,Crit,Dodge */

/* Tab page 4 controls */
static HWND g_cbEquip[6];
static HWND g_txtEquipInfo;

/* Tab page 5 controls */
static HWND g_lvBag;
static HWND g_cbBagAdd;
static HWND g_edBagQty;
static HWND g_lvStorage;
static HWND g_cbStAdd;
static HWND g_edStQty;
static HWND g_txtItemInfo;

/* Tab page 7 */
static HWND g_lvRoster;

/* Status bar */
static HWND g_hwndStatus;

/* ============================================================
 * Forward declarations
 * ============================================================ */
static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
static void CreateTabPages(HWND parent);
static void ShowTabPage(int index);
static void RefreshCurrentChar(void);
static void ApplyUIToCurrent(void);
static void SetStatus(const char *msg, int is_error);
static void BuildCharCombo(void);
static void RefreshBagList(void);
static void RefreshStorageList(void);
static void RefreshRoster(void);
static void RefreshEquipChoices(void);
static void UpdateEquipInfo(void);
static void UpdateItemInfo(int item_id);
static void LoadEquipFromData(void);
static void LoadBagFromData(void);
static void LoadSkillsFromData(void);
static void LoadStorageFromData(void);
static HWND CreateLabel(HWND parent, const char *text, int x, int y, int w, int h);
static HWND CreateEdit(HWND parent, int id, int x, int y, int w, int h, int readonly);
static HWND CreateButton(HWND parent, int id, const char *text, int x, int y, int w, int h);

/* ============================================================
 * WinMain — Entry point
 * ============================================================ */
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow)
{
    WNDCLASSEXA wc;
    MSG msg;
    HICON hIcon;
    char item_csv[MAX_PATH];
    char exe_dir[MAX_PATH];

    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_WIN95_CLASSES | ICC_TAB_CLASSES | ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&icc);

    memset(&g_ed, 0, sizeof(g_ed));
    g_currentTab = 0;

    /* Find exe directory for CSV loading */
    GetModuleFileNameA(NULL, exe_dir, MAX_PATH);
    {
        char *slash = strrchr(exe_dir, '\\');
        if (slash) *slash = '\0';
    }
    snprintf(item_csv, MAX_PATH, "%s\\data\\items.csv", exe_dir);
    if (GetFileAttributesA(item_csv) == INVALID_FILE_ATTRIBUTES) {
        /* Try relative to current directory */
        snprintf(item_csv, MAX_PATH, "data\\items.csv");
    }
    item_table_load(&g_ed, item_csv);

    /* Register window class */
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = "HSLREditorClass";
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    RegisterClassExA(&wc);

    /* Create main window */
    g_hwndMain = CreateWindowExA(
        0, "HSLREditorClass",
        "幻世录重制版 存档编辑器 v1 (TCC/C原生版)",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 860, 960,
        NULL, NULL, hInst, NULL);

    if (!g_hwndMain) return 0;

    ShowWindow(g_hwndMain, nShow);
    UpdateWindow(g_hwndMain);

    /* Message loop */
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

/* ============================================================
 * Helper: create common controls
 * ============================================================ */
static HWND CreateLabel(HWND parent, const char *text, int x, int y, int w, int h)
{
    wchar_t *wtext;
    int wlen;
    HWND hwnd;

    utf8_to_wide(text, &wtext, &wlen);
    hwnd = CreateWindowExW(0, L"STATIC", wtext ? wtext : L"",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        x, y, w, h, parent, NULL, GetModuleHandle(NULL), NULL);
    free(wtext);
    return hwnd;
}

static HWND CreateEdit(HWND parent, int id, int x, int y, int w, int h, int readonly)
{
    DWORD style = WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL;
    if (readonly) style |= ES_READONLY;
    return CreateWindowExA(NULL, "EDIT", "", style,
        x, y, w, h, parent, (HMENU)(INT_PTR)id, GetModuleHandle(NULL), NULL);
}

static HWND CreateButton(HWND parent, int id, const char *text, int x, int y, int w, int h)
{
    wchar_t *wtext;
    int wlen;
    HWND hwnd;

    utf8_to_wide(text, &wtext, &wlen);
    hwnd = CreateWindowExW(0, L"BUTTON", wtext ? wtext : L"",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        x, y, w, h, parent, (HMENU)(INT_PTR)id, GetModuleHandle(NULL), NULL);
    free(wtext);
    return hwnd;
}

/* ============================================================
 * Create all tab pages and child controls
 * ============================================================ */
static void CreateTabPages(HWND parent)
{
    RECT rc;
    int tw, th;
    int i;
    int pw, ph;
    int cx, cy;
    HWND pg;

    GetClientRect(g_hwndTab, &rc);
    TabCtrl_AdjustRect(g_hwndTab, FALSE, &rc);
    tw = rc.left;
    th = rc.top;
    pw = rc.right - rc.left;
    ph = rc.bottom - rc.top;

    /* Add tabs */
    {
        TCITEMW tie;
        const wchar_t *tab_names[] = {
            L" 基础信息 ", L" 存档属性 ", L" 战场属性 ",
            L" 装备 ", L" 物品 ", L" 技能 ", L" 全角色一览 "
        };
        for (i = 0; i < 7; i++) {
            memset(&tie, 0, sizeof(tie));
            tie.mask = TCIF_TEXT;
            tie.pszText = (LPWSTR)tab_names[i];
            TabCtrl_InsertItem(g_hwndTab, i, &tie);
        }
    }

    /* Create page windows */
    for (i = 0; i < 7; i++) {
        g_tabPages[i] = CreateWindowExA(0, "HSLRPage", "",
            WS_CHILD | WS_VISIBLE, tw, th, pw, ph,
            g_hwndTab, NULL, GetModuleHandle(NULL), NULL);
    }

    /* ========== Tab 0: Basic Info ========== */
    pg = g_tabPages[0];
    cx = 10; cy = 10;
    CreateLabel(pg, "等级:", cx, cy, 60, 20);
    g_edBase[0] = CreateEdit(pg, ID_ED_LEVEL, cx+65, cy, 80, 20, 0);
    CreateLabel(pg, "经验:", cx+160, cy, 60, 20);
    g_edBase[1] = CreateEdit(pg, ID_ED_EXP, cx+225, cy, 80, 20, 0);

    /* ========== Tab 1: Record Attrs ========== */
    pg = g_tabPages[1];
    cx = 10; cy = 5;

    CreateLabel(pg, "基础属性 (BaseAttr) ★核心★", cx, cy, 600, 20);
    cy += 22;
    {
        const char *labels[] = {"力量","敏捷","智力","体质","基础HP","基础MP"};
        int ids[] = {ID_ED_BASE_STR, ID_ED_BASE_DEX, ID_ED_BASE_MIND,
                     ID_ED_BASE_CON, ID_ED_BASE_HP, ID_ED_BASE_MP};
        int col;
        for (i = 0; i < 6; i++) {
            col = i % 3;
            CreateLabel(pg, labels[i], cx + col*200, cy + (i/3)*28, 50, 20);
            g_edBase[i] = CreateEdit(pg, ids[i], cx + col*200 + 55, cy + (i/3)*28, 70, 20, 0);
        }
    }
    cy += 65;

    /* Separator */
    CreateWindowExA(0, "STATIC", "", WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
        cx, cy, pw-20, 2, pg, NULL, GetModuleHandle(NULL), NULL);
    cy += 5;

    CreateLabel(pg, "永久加成 (PermanentFightAttr) ★核心★", cx, cy, 600, 20);
    cy += 22;
    {
        const char *labels[] = {"力量","敏捷","智力","体质","HP加成","MP加成",
                                 "物攻","魔攻","防御","速度","暴击率","闪避率"};
        int ids[] = {ID_ED_PERM_STR, ID_ED_PERM_DEX, ID_ED_PERM_MIND, ID_ED_PERM_CON,
                     ID_ED_PERM_MAXHP, ID_ED_PERM_MAXMP, ID_ED_PERM_PATK, ID_ED_PERM_MATK,
                     ID_ED_PERM_DEF, ID_ED_PERM_SPD, ID_ED_PERM_CRIT, ID_ED_PERM_DODGE};
        for (i = 0; i < 12; i++) {
            int col = i % 4;
            int row = i / 4;
            CreateLabel(pg, labels[i], cx + col*180, cy + row*28, 50, 20);
            g_edPerm[i] = CreateEdit(pg, ids[i], cx + col*180 + 55, cy + row*28, 65, 20, 0);
        }
    }
    cy += 95;

    /* Separator */
    CreateWindowExA(0, "STATIC", "", WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
        cx, cy, pw-20, 2, pg, NULL, GetModuleHandle(NULL), NULL);
    cy += 5;

    CreateLabel(pg, "战斗属性 (FightAttr)  [自动计算·只读]", cx, cy, 600, 20);
    cy += 22;
    {
        const char *labels[] = {"当前HP","最大HP","当前MP","最大MP",
                                 "力量","敏捷","智力","体质",
                                 "物攻","魔攻","防御","速度",
                                 "移动力","暴击率","闪避率",
                                 "火抗","水抗","风抗","地抗","灵抗"};
        int ids[] = {ID_ED_FIGHT_HP, ID_ED_FIGHT_MAXHP, ID_ED_FIGHT_MP, ID_ED_FIGHT_MAXMP,
                     ID_ED_FIGHT_STR, ID_ED_FIGHT_DEX, ID_ED_FIGHT_MIND, ID_ED_FIGHT_CON,
                     ID_ED_FIGHT_PATK, ID_ED_FIGHT_MATK, ID_ED_FIGHT_DEF, ID_ED_FIGHT_SPD,
                     ID_ED_FIGHT_MOVE, ID_ED_FIGHT_CRIT, ID_ED_FIGHT_DODGE,
                     ID_ED_FIGHT_FRES, ID_ED_FIGHT_WRES, ID_ED_FIGHT_ARES,
                     ID_ED_FIGHT_ERES, ID_ED_FIGHT_MRES};
        for (i = 0; i < 20; i++) {
            int col = i % 4;
            int row = i / 4;
            CreateLabel(pg, labels[i], cx + col*180, cy + row*28, 50, 20);
            g_edFight[i] = CreateEdit(pg, ids[i], cx + col*180 + 55, cy + row*28, 65, 20, 1);
        }
    }

    /* ========== Tab 2: Battle Attrs ========== */
    pg = g_tabPages[2];
    cx = 10; cy = 10;
    {
        const char *labels[] = {"当前HP","最大HP","当前MP","最大MP","等级","经验",
                                 "力量","敏捷","智力","体质",
                                 "物攻","魔攻","防御","速度","移动力","暴击率","闪避率"};
        int ids[] = {ID_ED_BAT_HP, ID_ED_BAT_MAXHP, ID_ED_BAT_MP, ID_ED_BAT_MAXMP,
                     ID_ED_BAT_LV, ID_ED_BAT_EXP,
                     ID_ED_BAT_STR, ID_ED_BAT_DEX, ID_ED_BAT_MIND, ID_ED_BAT_CON,
                     ID_ED_BAT_PATK, ID_ED_BAT_MATK, ID_ED_BAT_DEF, ID_ED_BAT_SPD,
                     ID_ED_BAT_MOVE, ID_ED_BAT_CRIT, ID_ED_BAT_DODGE};
        for (i = 0; i < 17; i++) {
            int col = i % 3;
            int row = i / 3;
            CreateLabel(pg, labels[i], cx + col*240, cy + row*28, 60, 20);
            g_edBat[i] = CreateEdit(pg, ids[i], cx + col*240 + 65, cy + row*28, 70, 20, 0);
        }
    }
    cy = 10 + 6*28 + 10;
    CreateButton(pg, ID_BTN_FULL_HEAL, "满血满蓝", cx, cy, 100, 28);
    CreateButton(pg, ID_BTN_MAX_BATTLE, "战场属性MAX", cx+110, cy, 120, 28);
    CreateButton(pg, ID_BTN_MAX_LEVEL, "Lv99+满经验", cx+240, cy, 120, 28);

    /* ========== Tab 3: Equipment ========== */
    pg = g_tabPages[3];
    cx = 10; cy = 5;
    CreateLabel(pg, "下拉框按部位过滤，也可直接手输物品ID", cx, cy, 700, 20);
    cy += 25;
    for (i = 0; i < NUM_EQUIP_SLOTS; i++) {
        CreateLabel(pg, EQUIP_SLOTS[i].name_cn, cx, cy, 50, 20);
        g_cbEquip[i] = CreateWindowExA(NULL, "COMBOBOX", "",
            WS_CHILD | WS_VISIBLE | CBS_DROPDOWN | WS_VSCROLL,
            cx + 55, cy, 400, 200,
            pg, (HMENU)(INT_PTR)(ID_CB_EQUIP0 + i), GetModuleHandle(NULL), NULL);
        SendMessage(g_cbEquip[i], WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
        cy += 28;
    }
    CreateButton(pg, ID_BTN_CLR_EQUIP, "清空全部装备", cx+55, cy, 120, 28);
    cy += 35;
    CreateLabel(pg, "装备效果:", cx, cy, 70, 20);
    cy += 20;
    g_txtEquipInfo = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
        cx, cy, 720, 200,
        pg, (HMENU)(INT_PTR)ID_TXT_EQUIP_INFO, GetModuleHandle(NULL), NULL);
    SendMessage(g_txtEquipInfo, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);

    /* ========== Tab 4: Items ========== */
    pg = g_tabPages[4];
    cx = 5; cy = 5;
    CreateLabel(pg, "角色背包 (GDCharRecordInfo.ItemIDs)", cx, cy, 700, 20);
    cy += 20;

    /* Bag list */
    g_lvBag = CreateWindowExA(WS_EX_CLIENTEDGE, WC_LISTVIEWA, "",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | WS_VSCROLL,
        cx, cy, 730, 150,
        pg, (HMENU)(INT_PTR)ID_LV_BAG, GetModuleHandle(NULL), NULL);
    {
        LVCOLUMNA lvc;
        const char *cols[] = {"ID", "名称", "类型", "数量"};
        int widths[] = {50, 300, 100, 60};
        for (i = 0; i < 4; i++) {
            lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
            lvc.fmt = LVCFMT_LEFT;
            lvc.pszText = (LPSTR)cols[i];
            lvc.cx = widths[i];
            ListView_InsertColumn(g_lvBag, i, &lvc);
        }
    }
    SendMessage(g_lvBag, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    cy += 155;

    /* Bag add controls */
    CreateLabel(pg, "添加:", cx, cy+3, 35, 20);
    g_cbBagAdd = CreateWindowExA(NULL, "COMBOBOX", "",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWN | WS_VSCROLL,
        cx+38, cy, 380, 200,
        pg, (HMENU)(INT_PTR)ID_CB_BAG_ADD, GetModuleHandle(NULL), NULL);
    SendMessage(g_cbBagAdd, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    CreateLabel(pg, "数量:", cx+425, cy+3, 35, 20);
    g_edBagQty = CreateEdit(pg, ID_ED_BAG_QTY, cx+462, cy, 50, 20, 0);
    SetWindowTextA(g_edBagQty, "1");
    CreateButton(pg, ID_BTN_BAG_ADD, "添加", cx+520, cy, 60, 24);
    CreateButton(pg, ID_BTN_BAG_DEL, "删除", cx+590, cy, 60, 24);
    cy += 30;

    /* Separator */
    CreateWindowExA(0, "STATIC", "", WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
        cx, cy, 720, 2, pg, NULL, GetModuleHandle(NULL), NULL);
    cy += 5;

    CreateLabel(pg, "全队仓库 (StorageItems)", cx, cy, 700, 20);
    cy += 20;

    g_lvStorage = CreateWindowExA(WS_EX_CLIENTEDGE, WC_LISTVIEWA, "",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | WS_VSCROLL,
        cx, cy, 730, 150,
        pg, (HMENU)(INT_PTR)ID_LV_STORAGE, GetModuleHandle(NULL), NULL);
    {
        LVCOLUMNA lvc;
        const char *cols[] = {"ID", "名称", "类型", "数量"};
        int widths[] = {50, 300, 100, 60};
        for (i = 0; i < 4; i++) {
            lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
            lvc.fmt = LVCFMT_LEFT;
            lvc.pszText = (LPSTR)cols[i];
            lvc.cx = widths[i];
            ListView_InsertColumn(g_lvStorage, i, &lvc);
        }
    }
    SendMessage(g_lvStorage, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    cy += 155;

    CreateLabel(pg, "添加:", cx, cy+3, 35, 20);
    g_cbStAdd = CreateWindowExA(NULL, "COMBOBOX", "",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWN | WS_VSCROLL,
        cx+38, cy, 380, 200,
        pg, (HMENU)(INT_PTR)ID_CB_ST_ADD, GetModuleHandle(NULL), NULL);
    SendMessage(g_cbStAdd, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    CreateLabel(pg, "数量:", cx+425, cy+3, 35, 20);
    g_edStQty = CreateEdit(pg, ID_ED_ST_QTY, cx+462, cy, 50, 20, 0);
    SetWindowTextA(g_edStQty, "1");
    CreateButton(pg, ID_BTN_ST_ADD, "添加", cx+520, cy, 60, 24);
    CreateButton(pg, ID_BTN_ST_DEL, "删除", cx+590, cy, 60, 24);
    cy += 30;

    CreateLabel(pg, "物品说明:", cx, cy, 70, 20);
    cy += 20;
    g_txtItemInfo = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
        cx, cy, 730, 80,
        pg, (HMENU)(INT_PTR)ID_TXT_ITEM_INFO, GetModuleHandle(NULL), NULL);
    SendMessage(g_txtItemInfo, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);

    /* ========== Tab 5: Skills ========== */
    pg = g_tabPages[5];
    cx = 10; cy = 20;
    CreateLabel(pg, "普攻技能ID:", cx, cy, 80, 20);
    g_edBat[0] = CreateEdit(pg, ID_ED_SKILL_NRL, cx+85, cy, 80, 20, 0);
    cy += 30;
    CreateLabel(pg, "魔法技能IDs:", cx, cy, 80, 20);
    CreateEdit(pg, ID_ED_SKILL_MAGIC, cx+85, cy, 500, 20, 0);
    cy += 30;
    CreateLabel(pg, "特殊技能IDs:", cx, cy, 80, 20);
    CreateEdit(pg, ID_ED_SKILL_SP, cx+85, cy, 500, 20, 0);
    cy += 30;
    CreateLabel(pg, "格式：逗号分隔的ID，如 66,87,70", cx, cy, 700, 20);

    /* ========== Tab 6: Roster ========== */
    pg = g_tabPages[6];
    cx = 5; cy = 5;
    g_lvRoster = CreateWindowExA(WS_EX_CLIENTEDGE, WC_LISTVIEWA, "",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | WS_VSCROLL,
        cx, cy, 730, 680,
        pg, (HMENU)(INT_PTR)ID_LV_ROSTER, GetModuleHandle(NULL), NULL);
    {
        LVCOLUMNA lvc;
        const char *cols[] = {"ID","名称","等级","HP","MaxHP","物攻","魔攻","防御","阵营"};
        int widths[] = {50, 130, 60, 60, 60, 70, 70, 70, 60};
        for (i = 0; i < 9; i++) {
            lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
            lvc.fmt = LVCFMT_LEFT;
            lvc.pszText = (LPSTR)cols[i];
            lvc.cx = widths[i];
            ListView_InsertColumn(g_lvRoster, i, &lvc);
        }
    }
    SendMessage(g_lvRoster, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
}

/* ============================================================
 * Tab page visibility
 * ============================================================ */
static void ShowTabPage(int index)
{
    int i;
    RECT rc;
    int tw, th;

    g_currentTab = index;

    GetClientRect(g_hwndTab, &rc);
    TabCtrl_AdjustRect(g_hwndTab, FALSE, &rc);
    tw = rc.left;
    th = rc.top;

    for (i = 0; i < 7; i++) {
        if (i == index) {
            MoveWindow(g_tabPages[i], tw, th,
                rc.right - rc.left, rc.bottom - rc.top, TRUE);
            ShowWindow(g_tabPages[i], SW_SHOW);
        } else {
            ShowWindow(g_tabPages[i], SW_HIDE);
        }
    }
}

/* ============================================================
 * Item combo box population
 * ============================================================ */
static void PopulateItemCombo(HWND combo, int filter_slot)
{
    int i;
    /* Clear */
    SendMessage(combo, CB_RESETCONTENT, 0, 0);
    /* Add "(空)" first */
    {
        wchar_t *w;
        int wl;
        utf8_to_wide("(空)", &w, &wl);
        if (w) { SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)w); free(w); }
    }
    /* Add items sorted by price, filtered by slot */
    for (i = 0; i < g_ed.item_count; i++) {
        const ItemInfo *it = &g_ed.items[i];
        if (filter_slot >= 0 && it->slot != filter_slot) continue;
        if (it->name[0] || it->type[0]) {
            char label[256];
            wchar_t *wlabel;
            int wl;
            item_label(&g_ed, it->id, label, sizeof(label));
            utf8_to_wide(label, &wlabel, &wl);
            if (wlabel) { SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)wlabel); free(wlabel); }
        }
    }
}

static void RefreshEquipChoices(void)
{
    int i;
    for (i = 0; i < NUM_EQUIP_SLOTS; i++)
        PopulateItemCombo(g_cbEquip[i], EQUIP_SLOTS[i].id);
    PopulateItemCombo(g_cbBagAdd, -1);
    PopulateItemCombo(g_cbStAdd, -1);
}

/* ============================================================
 * Bag/Storage list refresh
 * ============================================================ */
static void RefreshBagList(void)
{
    int i, j;
    int ids[256], counts[256], n = 0;

    ListView_DeleteAllItems(g_lvBag);
    if (g_ed.current_idx < 0) return;

    /* Aggregate bag items */
    {
        CharRecord *rec = NULL;
        CharEntity *ent = NULL;

        /* Find current character */
        for (i = 0; i < g_ed.entity_count; i++) {
            if (g_ed.entities[i].pid == g_ed.current_pid) { ent = &g_ed.entities[i]; break; }
        }
        if (!ent) {
            for (i = 0; i < g_ed.record_count; i++) {
                if (g_ed.records[i].pid == g_ed.current_pid) { rec = &g_ed.records[i]; break; }
            }
        }

        if (rec) {
            for (i = 0; i < rec->bag_count; i++) {
                int found = 0;
                for (j = 0; j < n; j++) { if (ids[j] == rec->bag[i]) { counts[j]++; found=1; break; } }
                if (!found && n < 256) { ids[n] = rec->bag[i]; counts[n] = 1; n++; }
            }
        } else if (ent) {
            for (i = 0; i < ent->bag_count; i++) {
                int found = 0;
                for (j = 0; j < n; j++) { if (ids[j] == ent->bag[i]) { counts[j]++; found=1; break; } }
                if (!found && n < 256) { ids[n] = ent->bag[i]; counts[n] = 1; n++; }
            }
        }
    }

    /* Sort by ID */
    for (i = 0; i < n-1; i++) {
        for (j = i+1; j < n; j++) {
            if (ids[j] < ids[i]) {
                int t=ids[i]; ids[i]=ids[j]; ids[j]=t;
                t=counts[i]; counts[i]=counts[j]; counts[j]=t;
            }
        }
    }

    /* Insert into ListView */
    for (i = 0; i < n; i++) {
        char id_str[32], name[128], type[64], qty_str[16];
        wchar_t wid[32], wname[128], wtype[64], wqty[16];
        LVITEMA lvi;
        const ItemInfo *it = item_get(&g_ed, ids[i]);

        snprintf(id_str, sizeof(id_str), "%d", ids[i]);
        snprintf(name, sizeof(name), "%s", it ? it->name : "(未知物品)");
        snprintf(type, sizeof(type), "%s", it ? it->type : "");
        snprintf(qty_str, sizeof(qty_str), "%d", counts[i]);

        utf8_to_wide(id_str, &wid, NULL);
        utf8_to_wide(name, &wname, NULL);
        utf8_to_wide(type, &wtype, NULL);
        utf8_to_wide(qty_str, &wqty, NULL);

        ZeroMemory(&lvi, sizeof(lvi));
        lvi.mask = LVIF_TEXT;
        lvi.iItem = i;
        lvi.pszText = wid;
        lvi.iSubItem = 0;
        ListView_InsertItem(g_lvBag, &lvi);
        ListView_SetItemText(g_lvBag, i, 1, wname);
        ListView_SetItemText(g_lvBag, i, 2, wtype);
        ListView_SetItemText(g_lvBag, i, 3, wqty);

        free(wid); free(wname); free(wtype); free(wqty);
    }
}

static void RefreshStorageList(void)
{
    int i;
    ListView_DeleteAllItems(g_lvStorage);

    for (i = 0; i < g_ed.storage_count; i++) {
        char id_str[32], name[128], type[64], qty_str[16];
        wchar_t wid[32], wname[128], wtype[64], wqty[16];
        LVITEMA lvi;
        const ItemInfo *it = item_get(&g_ed, g_ed.storage_ids[i]);

        snprintf(id_str, sizeof(id_str), "%d", g_ed.storage_ids[i]);
        snprintf(name, sizeof(name), "%s", it ? it->name : "(未知物品)");
        snprintf(type, sizeof(type), "%s", it ? it->type : "");
        snprintf(qty_str, sizeof(qty_str), "%d", g_ed.storage_qty[i]);

        utf8_to_wide(id_str, &wid, NULL);
        utf8_to_wide(name, &wname, NULL);
        utf8_to_wide(type, &wtype, NULL);
        utf8_to_wide(qty_str, &wqty, NULL);

        ZeroMemory(&lvi, sizeof(lvi));
        lvi.mask = LVIF_TEXT;
        lvi.iItem = i;
        lvi.pszText = wid;
        lvi.iSubItem = 0;
        ListView_InsertItem(g_lvStorage, &lvi);
        ListView_SetItemText(g_lvStorage, i, 1, wname);
        ListView_SetItemText(g_lvStorage, i, 2, wtype);
        ListView_SetItemText(g_lvStorage, i, 3, wqty);

        free(wid); free(wname); free(wtype); free(wqty);
    }
}

/* ============================================================
 * Roster list refresh
 * ============================================================ */
static void RefreshRoster(void)
{
    int i;
    ListView_DeleteAllItems(g_lvRoster);

    /* From entities */
    for (i = 0; i < g_ed.entity_count; i++) {
        CharEntity *ent = &g_ed.entities[i];
        char pid_s[32], name_s[128], lv_s[32], hp_s[32], maxhp_s[32];
        char patk_s[32], matk_s[32], def_s[32], camp_s[32];
        wchar_t *wp, *wn, *wl, *wh, *wmh, *wpa, *wma, *wd, *wc;
        LVITEMA lvi;
        int sub = 0;

        snprintf(pid_s, sizeof(pid_s), "%d", ent->pid);
        snprintf(name_s, sizeof(name_s), "%s", ent->name[0] ? ent->name : "???");
        snprintf(lv_s, sizeof(lv_s), "%d", ent->level);
        snprintf(hp_s, sizeof(hp_s), "%d", ent->hp);
        snprintf(maxhp_s, sizeof(maxhp_s), "%d", ent->max_hp);
        snprintf(patk_s, sizeof(patk_s), "%d", ent->fight.PhysicalAttack);
        snprintf(matk_s, sizeof(matk_s), "%d", ent->fight.MagicAttack);
        snprintf(def_s, sizeof(def_s), "%d", ent->fight.Defense);
        snprintf(camp_s, sizeof(camp_s), "%s",
            ent->camp == 1 ? "敌方" : ent->camp == 2 ? "我方" : "中立");

        utf8_to_wide(pid_s, &wp, NULL);
        utf8_to_wide(name_s, &wn, NULL);
        utf8_to_wide(lv_s, &wl, NULL);
        utf8_to_wide(hp_s, &wh, NULL);
        utf8_to_wide(maxhp_s, &wmh, NULL);
        utf8_to_wide(patk_s, &wpa, NULL);
        utf8_to_wide(matk_s, &wma, NULL);
        utf8_to_wide(def_s, &wd, NULL);
        utf8_to_wide(camp_s, &wc, NULL);

        ZeroMemory(&lvi, sizeof(lvi));
        lvi.mask = LVIF_TEXT;
        lvi.iItem = i;
        lvi.iSubItem = 0;
        lvi.pszText = wp;
        ListView_InsertItem(g_lvRoster, &lvi);
        ListView_SetItemText(g_lvRoster, i, 1, wn);
        ListView_SetItemText(g_lvRoster, i, 2, wl);
        ListView_SetItemText(g_lvRoster, i, 3, wh);
        ListView_SetItemText(g_lvRoster, i, 4, wmh);
        ListView_SetItemText(g_lvRoster, i, 5, wpa);
        ListView_SetItemText(g_lvRoster, i, 6, wma);
        ListView_SetItemText(g_lvRoster, i, 7, wd);
        ListView_SetItemText(g_lvRoster, i, 8, wc);

        free(wp); free(wn); free(wl); free(wh); free(wmh);
        free(wpa); free(wma); free(wd); free(wc);
    }

    /* From records only (no entity) */
    for (i = 0; i < g_ed.record_count; i++) {
        CharRecord *rec = &g_ed.records[i];
        int j, has_entity = 0;
        char pid_s[32], name_s[128], lv_s[32], hp_s[32], maxhp_s[32];
        char patk_s[32], matk_s[32], def_s[32];
        wchar_t *wp, *wn, *wl, *wh, *wmh, *wpa, *wma, *wd;
        LVITEMA lvi;

        for (j = 0; j < g_ed.entity_count; j++) {
            if (g_ed.entities[j].pid == rec->pid) { has_entity = 1; break; }
        }
        if (has_entity) continue;

        snprintf(pid_s, sizeof(pid_s), "%d", rec->pid);
        snprintf(name_s, sizeof(name_s), "%s", rec->name[0] ? rec->name : "???");
        snprintf(lv_s, sizeof(lv_s), "%d", rec->level);
        snprintf(hp_s, sizeof(hp_s), "%d", rec->fight.Hp ? rec->fight.Hp : rec->base.Hp);
        snprintf(maxhp_s, sizeof(maxhp_s), "%d", rec->fight.MaxHp);
        snprintf(patk_s, sizeof(patk_s), "%d", rec->fight.PhysicalAttack);
        snprintf(matk_s, sizeof(matk_s), "%d", rec->fight.MagicAttack);
        snprintf(def_s, sizeof(def_s), "%d", rec->fight.Defense);

        utf8_to_wide(pid_s, &wp, NULL);
        utf8_to_wide(name_s, &wn, NULL);
        utf8_to_wide(lv_s, &wl, NULL);
        utf8_to_wide(hp_s, &wh, NULL);
        utf8_to_wide(maxhp_s, &wmh, NULL);
        utf8_to_wide(patk_s, &wpa, NULL);
        utf8_to_wide(matk_s, &wma, NULL);
        utf8_to_wide(def_s, &wd, NULL);

        ZeroMemory(&lvi, sizeof(lvi));
        lvi.mask = LVIF_TEXT;
        lvi.iItem = ListView_GetItemCount(g_lvRoster);
        lvi.iSubItem = 0;
        lvi.pszText = wp;
        ListView_InsertItem(g_lvRoster, &lvi);
        {
            int r = lvi.iItem;
            ListView_SetItemText(g_lvRoster, r, 1, wn);
            ListView_SetItemText(g_lvRoster, r, 2, wl);
            ListView_SetItemText(g_lvRoster, r, 3, wh);
            ListView_SetItemText(g_lvRoster, r, 4, wmh);
            ListView_SetItemText(g_lvRoster, r, 5, wpa);
            ListView_SetItemText(g_lvRoster, r, 6, wma);
            ListView_SetItemText(g_lvRoster, r, 7, wd);
            ListView_SetItemText(g_lvRoster, r, 8, L"我方");
        }

        free(wp); free(wn); free(wl); free(wh); free(wmh);
        free(wpa); free(wma); free(wd);
    }
}

/* ============================================================
 * Equipment info display
 * ============================================================ */
static void UpdateEquipInfo(void)
{
    int i;
    char buf[4096] = {0};
    int pos = 0;

    for (i = 0; i < NUM_EQUIP_SLOTS; i++) {
        char *text = get_ctrl_text_utf8(g_cbEquip[i]);
        int id = item_parse_id(text);
        free(text);

        if (id >= 0) {
            char title[256], desc[512], line[800];
            item_title(&g_ed, id, title, sizeof(title));
            pos += snprintf(buf + pos, sizeof(buf) - pos, "%s\n", title);
            {
                const ItemInfo *it = item_get(&g_ed, id);
                if (it && it->desc[0]) {
                    pos += snprintf(buf + pos, sizeof(buf) - pos, "    %s\n", it->desc);
                }
            }
            pos += snprintf(buf + pos, sizeof(buf) - pos, "\n");
        }
    }
    if (pos == 0)
        snprintf(buf, sizeof(buf), "(未装备任何物品)");

    set_ctrl_text_utf8(g_txtEquipInfo, buf);
}

static void UpdateItemInfo(int item_id)
{
    char buf[2048] = {0};

    if (item_id < 0) {
        snprintf(buf, sizeof(buf),
            "(在下拉框里选择物品或在列表里点选一行，这里会显示物品介绍)");
    } else {
        const ItemInfo *it = item_get(&g_ed, item_id);
        if (!it) {
            snprintf(buf, sizeof(buf), "%d  (数据表中没有此ID)", item_id);
        } else {
            char title[256];
            item_title(&g_ed, item_id, title, sizeof(title));
            snprintf(buf, sizeof(buf), "%s\n买价：%d\n\n    %s",
                title, it->price, it->desc[0] ? it->desc : "");
        }
    }
    set_ctrl_text_utf8(g_txtItemInfo, buf);
}

/* ============================================================
 * Load data into UI for current character
 * ============================================================ */
static void LoadEquipFromData(void)
{
    int i;
    CharRecord *rec = NULL;
    CharEntity *ent = NULL;

    for (i = 0; i < g_ed.entity_count; i++) {
        if (g_ed.entities[i].pid == g_ed.current_pid) { ent = &g_ed.entities[i]; break; }
    }
    if (!ent) {
        for (i = 0; i < g_ed.record_count; i++) {
            if (g_ed.records[i].pid == g_ed.current_pid) { rec = &g_ed.records[i]; break; }
        }
    }

    for (i = 0; i < NUM_EQUIP_SLOTS; i++) {
        int id = -1;
        if (rec) id = rec->equip[i];
        else if (ent) id = ent->equip[i];

        if (id >= 0) {
            char label[256];
            wchar_t *wlabel;
            item_label(&g_ed, id, label, sizeof(label));
            utf8_to_wide(label, &wlabel, NULL);
            SendMessageW(g_cbEquip[i], CB_SETTEXT, 0, (LPARAM)wlabel);
            free(wlabel);
        } else {
            SendMessageA(g_cbEquip[i], CB_SETTEXT, 0, (LPARAM)"");
        }
    }
    UpdateEquipInfo();
}

static void LoadBagFromData(void)
{
    CharRecord *rec = NULL;
    CharEntity *ent = NULL;
    int i;

    for (i = 0; i < g_ed.entity_count; i++) {
        if (g_ed.entities[i].pid == g_ed.current_pid) { ent = &g_ed.entities[i]; break; }
    }
    if (!ent) {
        for (i = 0; i < g_ed.record_count; i++) {
            if (g_ed.records[i].pid == g_ed.current_pid) { rec = &g_ed.records[i]; break; }
        }
    }

    /* Store bag IDs temporarily for refresh */
    /* We'll aggregate and display in RefreshBagList */
    RefreshBagList();
}

static void LoadSkillsFromData(void)
{
    CharRecord *rec = NULL;
    CharEntity *ent = NULL;
    int i;
    char buf[1024];
    int pos;

    for (i = 0; i < g_ed.entity_count; i++) {
        if (g_ed.entities[i].pid == g_ed.current_pid) { ent = &g_ed.entities[i]; break; }
    }
    if (!ent) {
        for (i = 0; i < g_ed.record_count; i++) {
            if (g_ed.records[i].pid == g_ed.current_pid) { rec = &g_ed.records[i]; break; }
        }
    }

    /* Normal attack skill */
    {
        int nrl = 0;
        if (rec) nrl = rec->nrl_skill;
        else if (ent) nrl = ent->nrl_skill;
        snprintf(buf, sizeof(buf), "%d", nrl);
        SetWindowTextA(GetDlgItem(g_tabPages[5], ID_ED_SKILL_NRL), nrl ? buf : "");
    }

    /* Magic skills */
    pos = 0;
    if (rec) {
        for (i = 0; i < rec->magic_skill_count; i++)
            pos += snprintf(buf + pos, sizeof(buf) - pos, "%s%d", i ? "," : "", rec->magic_skills[i]);
    } else if (ent) {
        for (i = 0; i < ent->magic_skill_count; i++)
            pos += snprintf(buf + pos, sizeof(buf) - pos, "%s%d", i ? "," : "", ent->magic_skills[i]);
    }
    SetWindowTextA(GetDlgItem(g_tabPages[5], ID_ED_SKILL_MAGIC), buf);

    /* Special skills */
    pos = 0;
    if (rec) {
        for (i = 0; i < rec->sp_skill_count; i++)
            pos += snprintf(buf + pos, sizeof(buf) - pos, "%s%d", i ? "," : "", rec->sp_skills[i]);
    } else if (ent) {
        for (i = 0; i < ent->sp_skill_count; i++)
            pos += snprintf(buf + pos, sizeof(buf) - pos, "%s%d", i ? "," : "", ent->sp_skills[i]);
    }
    SetWindowTextA(GetDlgItem(g_tabPages[5], ID_ED_SKILL_SP), buf);
}

static void LoadStorageFromData(void)
{
    RefreshStorageList();
}

/* ============================================================
 * Refresh entire UI for current character
 * ============================================================ */
static void RefreshCurrentChar(void)
{
    int i;
    CharRecord *rec = NULL;
    CharEntity *ent = NULL;
    cJSON *rec_json = NULL;

    if (g_ed.current_pid < 0) return;

    /* Find source data */
    for (i = 0; i < g_ed.entity_count; i++) {
        if (g_ed.entities[i].pid == g_ed.current_pid) { ent = &g_ed.entities[i]; break; }
    }
    for (i = 0; i < g_ed.record_count; i++) {
        if (g_ed.records[i].pid == g_ed.current_pid) { rec = &g_ed.records[i]; break; }
    }

    /* Get JSON source for direct field access */
    if (g_ed.gplay) {
        cJSON *chars = cJSON_GetObjectItem(g_ed.gplay, "GDCharRecordInfo");
        if (chars) {
            char pid_str[32];
            snprintf(pid_str, sizeof(pid_str), "%d", g_ed.current_pid);
            rec_json = cJSON_GetObjectItem(chars, pid_str);
        }
    }

    /* Basic info (Tab 0) */
    if (rec_json) {
        cJSON *lv_item = cJSON_GetObjectItem(rec_json, "Level");
        cJSON *exp_item = cJSON_GetObjectItem(rec_json, "Exp");
        char v[32];
        snprintf(v, sizeof(v), "%d", lv_item ? cJSON_IsNumber(lv_item) ? lv_item->valueint : atoi(lv_item->valuestring) : 0);
        SetWindowTextA(g_edBase[0], v);
        snprintf(v, sizeof(v), "%d", exp_item ? cJSON_IsNumber(exp_item) ? exp_item->valueint : atoi(exp_item->valuestring) : 0);
        SetWindowTextA(g_edBase[1], v);
    } else {
        SetWindowTextA(g_edBase[0], "0");
        SetWindowTextA(g_edBase[1], "0");
    }

    /* Record attrs (Tab 1) - BaseAttr */
    if (rec_json) {
        cJSON *ba = cJSON_GetObjectItem(rec_json, "BaseAttr");
        cJSON *pfa = cJSON_GetObjectItem(rec_json, "PermanentFightAttr");
        cJSON *fa = cJSON_GetObjectItem(rec_json, "FightAttr");

        if (ba) {
            char v[32];
            snprintf(v, sizeof(v), "%d", cJSON_GetInt(ba, "Str", 0)); SetWindowTextA(g_edBase[0], v);
            snprintf(v, sizeof(v), "%d", cJSON_GetInt(ba, "Dex", 0)); SetWindowTextA(g_edBase[1], v);
            snprintf(v, sizeof(v), "%d", cJSON_GetInt(ba, "Mind", 0)); SetWindowTextA(g_edBase[2], v);
            snprintf(v, sizeof(v), "%d", cJSON_GetInt(ba, "Con", 0)); SetWindowTextA(g_edBase[3], v);
            snprintf(v, sizeof(v), "%d", cJSON_GetInt(ba, "Hp", 0)); SetWindowTextA(g_edBase[4], v);
            snprintf(v, sizeof(v), "%d", cJSON_GetInt(ba, "Mp", 0)); SetWindowTextA(g_edBase[5], v);
        }

        if (pfa) {
            char v[32];
            int ids[] = {ID_ED_PERM_STR, ID_ED_PERM_DEX, ID_ED_PERM_MIND, ID_ED_PERM_CON,
                         ID_ED_PERM_MAXHP, ID_ED_PERM_MAXMP, ID_ED_PERM_PATK, ID_ED_PERM_MATK,
                         ID_ED_PERM_DEF, ID_ED_PERM_SPD, ID_ED_PERM_CRIT, ID_ED_PERM_DODGE};
            const char *keys[] = {"Str","Dex","Mind","Con","MaxHp","MaxMp",
                                   "PhysicalAttack","MagicAttack","Defense","Speed","CriticalRatio","DodgeRatio"};
            int k;
            for (k = 0; k < 12; k++) {
                snprintf(v, sizeof(v), "%d", cJSON_GetInt(pfa, keys[k], 0));
                SetWindowTextA(GetDlgItem(g_tabPages[1], ids[k]), v);
            }
        }

        if (fa) {
            char v[32];
            int ids[] = {ID_ED_FIGHT_HP, ID_ED_FIGHT_MAXHP, ID_ED_FIGHT_MP, ID_ED_FIGHT_MAXMP,
                         ID_ED_FIGHT_STR, ID_ED_FIGHT_DEX, ID_ED_FIGHT_MIND, ID_ED_FIGHT_CON,
                         ID_ED_FIGHT_PATK, ID_ED_FIGHT_MATK, ID_ED_FIGHT_DEF, ID_ED_FIGHT_SPD,
                         ID_ED_FIGHT_MOVE, ID_ED_FIGHT_CRIT, ID_ED_FIGHT_DODGE,
                         ID_ED_FIGHT_FRES, ID_ED_FIGHT_WRES, ID_ED_FIGHT_ARES,
                         ID_ED_FIGHT_ERES, ID_ED_FIGHT_MRES};
            const char *keys[] = {"Hp","MaxHp","Mp","MaxMp","Str","Dex","Mind","Con",
                                   "PhysicalAttack","MagicAttack","Defense","Speed","Move",
                                   "CriticalRatio","DodgeRatio","FireRes","WaterRes","AirRes","EarthRes","MindRes"};
            int k;
            for (k = 0; k < 20; k++) {
                snprintf(v, sizeof(v), "%d", cJSON_GetInt(fa, keys[k], 0));
                SetWindowTextA(GetDlgItem(g_tabPages[1], ids[k]), v);
            }
        }
    }

    /* Battle attrs (Tab 2) */
    if (ent) {
        char v[32];
        snprintf(v, sizeof(v), "%d", ent->hp); SetWindowTextA(g_edBat[0], v);
        snprintf(v, sizeof(v), "%d", ent->max_hp); SetWindowTextA(g_edBat[1], v);
        snprintf(v, sizeof(v), "%d", ent->mp); SetWindowTextA(g_edBat[2], v);
        snprintf(v, sizeof(v), "%d", ent->max_mp); SetWindowTextA(g_edBat[3], v);
        snprintf(v, sizeof(v), "%d", ent->level); SetWindowTextA(g_edBat[4], v);
        snprintf(v, sizeof(v), "%d", ent->exp); SetWindowTextA(g_edBat[5], v);
        snprintf(v, sizeof(v), "%d", ent->fight.Str); SetWindowTextA(g_edBat[6], v);
        snprintf(v, sizeof(v), "%d", ent->fight.Dex); SetWindowTextA(g_edBat[7], v);
        snprintf(v, sizeof(v), "%d", ent->fight.Mind); SetWindowTextA(g_edBat[8], v);
        snprintf(v, sizeof(v), "%d", ent->fight.Con); SetWindowTextA(g_edBat[9], v);
        snprintf(v, sizeof(v), "%d", ent->fight.PhysicalAttack); SetWindowTextA(g_edBat[10], v);
        snprintf(v, sizeof(v), "%d", ent->fight.MagicAttack); SetWindowTextA(g_edBat[11], v);
        snprintf(v, sizeof(v), "%d", ent->fight.Defense); SetWindowTextA(g_edBat[12], v);
        snprintf(v, sizeof(v), "%d", ent->fight.Speed); SetWindowTextA(g_edBat[13], v);
        snprintf(v, sizeof(v), "%d", ent->fight.Move); SetWindowTextA(g_edBat[14], v);
        snprintf(v, sizeof(v), "%d", ent->fight.CriticalRatio); SetWindowTextA(g_edBat[15], v);
        snprintf(v, sizeof(v), "%d", ent->fight.DodgeRatio); SetWindowTextA(g_edBat[16], v);
    }

    /* Equipment / Items / Skills / Storage */
    LoadEquipFromData();
    LoadBagFromData();
    LoadSkillsFromData();
    LoadStorageFromData();

    /* Roster */
    RefreshRoster();
}

/* ============================================================
 * Apply UI values back to current character data
 * ============================================================ */
static void ApplyUIToCurrent(void)
{
    int i;
    cJSON *rec_json = NULL;

    if (g_ed.current_pid < 0 || !g_ed.gplay) return;

    /* Find JSON record */
    {
        cJSON *chars = cJSON_GetObjectItem(g_ed.gplay, "GDCharRecordInfo");
        if (chars) {
            char pid_str[32];
            snprintf(pid_str, sizeof(pid_str), "%d", g_ed.current_pid);
            rec_json = cJSON_GetObjectItem(chars, pid_str);
        }
    }

    /* Basic info → record */
    if (rec_json) {
        char buf[64];
        GetWindowTextA(g_edBase[0], buf, sizeof(buf)); cJSON_SetNumber(cJSON_GetObjectItem(rec_json, "Level"), atoi(buf));
        GetWindowTextA(g_edBase[1], buf, sizeof(buf)); cJSON_SetNumber(cJSON_GetObjectItem(rec_json, "Exp"), atoi(buf));
    }

    /* Record attrs → record */
    if (rec_json) {
        cJSON *ba = cJSON_GetObjectItem(rec_json, "BaseAttr");
        cJSON *pfa = cJSON_GetObjectItem(rec_json, "PermanentFightAttr");
        cJSON *fa = cJSON_GetObjectItem(rec_json, "FightAttr");
        char buf[64];

        if (ba) {
            GetWindowTextA(g_edBase[0], buf, sizeof(buf)); cJSON_SetNumber(cJSON_GetObjectItem(ba, "Str"), atoi(buf));
            GetWindowTextA(g_edBase[1], buf, sizeof(buf)); cJSON_SetNumber(cJSON_GetObjectItem(ba, "Dex"), atoi(buf));
            GetWindowTextA(g_edBase[2], buf, sizeof(buf)); cJSON_SetNumber(cJSON_GetObjectItem(ba, "Mind"), atoi(buf));
            GetWindowTextA(g_edBase[3], buf, sizeof(buf)); cJSON_SetNumber(cJSON_GetObjectItem(ba, "Con"), atoi(buf));
            GetWindowTextA(g_edBase[4], buf, sizeof(buf)); cJSON_SetNumber(cJSON_GetObjectItem(ba, "Hp"), atoi(buf));
            GetWindowTextA(g_edBase[5], buf, sizeof(buf)); cJSON_SetNumber(cJSON_GetObjectItem(ba, "Mp"), atoi(buf));
        }

        if (pfa) {
            int ids[] = {ID_ED_PERM_STR, ID_ED_PERM_DEX, ID_ED_PERM_MIND, ID_ED_PERM_CON,
                         ID_ED_PERM_MAXHP, ID_ED_PERM_MAXMP, ID_ED_PERM_PATK, ID_ED_PERM_MATK,
                         ID_ED_PERM_DEF, ID_ED_PERM_SPD, ID_ED_PERM_CRIT, ID_ED_PERM_DODGE};
            const char *keys[] = {"Str","Dex","Mind","Con","MaxHp","MaxMp",
                                   "PhysicalAttack","MagicAttack","Defense","Speed","CriticalRatio","DodgeRatio"};
            int k;
            for (k = 0; k < 12; k++) {
                GetWindowTextA(GetDlgItem(g_tabPages[1], ids[k]), buf, sizeof(buf));
                cJSON_SetNumber(cJSON_GetObjectItem(pfa, keys[k]), atoi(buf));
            }
        }

        /* Note: FightAttr is read-only in Tab 1, no need to apply back */
    }

    /* Equipment from UI */
    {
        cJSON *equip_obj = cJSON_GetObjectItem(rec_json, "EquipIDs");
        if (!equip_obj) { equip_obj = cJSON_CreateObject(); cJSON_AddItemToObject(rec_json, "EquipIDs", equip_obj); }
        for (i = 0; i < NUM_EQUIP_SLOTS; i++) {
            char key[8], *text;
            int id;
            snprintf(key, sizeof(key), "%d", i);
            text = get_ctrl_text_utf8(g_cbEquip[i]);
            id = item_parse_id(text);
            free(text);
            if (id >= 0)
                cJSON_SetNumber(cJSON_GetObjectItem(equip_obj, key), id);
            else
                cJSON_DeleteItemFromObject(equip_obj, key);
        }
    }

    /* Skills from UI */
    {
        char buf[1024];
        cJSON *magic_arr, *sp_arr;

        GetWindowTextA(GetDlgItem(g_tabPages[5], ID_ED_SKILL_NRL), buf, sizeof(buf));
        cJSON_SetNumber(cJSON_GetObjectItem(rec_json, "NrlSkillId"), atoi(buf));

        GetWindowTextA(GetDlgItem(g_tabPages[5], ID_ED_SKILL_MAGIC), buf, sizeof(buf));
        magic_arr = cJSON_GetObjectItem(rec_json, "MagicSkillIDs");
        if (magic_arr) cJSON_DeleteItemFromObject(rec_json, "MagicSkillIDs");
        magic_arr = cJSON_CreateArray();
        {
            char *tok = strtok(buf, ",");
            while (tok) {
                while (*tok == ' ') tok++;
                if (*tok) cJSON_AddItemToArray(magic_arr, cJSON_CreateNumber(atoi(tok)));
                tok = strtok(NULL, ",");
            }
        }
        cJSON_AddItemToObject(rec_json, "MagicSkillIDs", magic_arr);

        GetWindowTextA(GetDlgItem(g_tabPages[5], ID_ED_SKILL_SP), buf, sizeof(buf));
        sp_arr = cJSON_GetObjectItem(rec_json, "SpSkillIDs");
        if (sp_arr) cJSON_DeleteItemFromObject(rec_json, "SpSkillIDs");
        sp_arr = cJSON_CreateArray();
        {
            char *tok = strtok(buf, ",");
            while (tok) {
                while (*tok == ' ') tok++;
                if (*tok) cJSON_AddItemToArray(sp_arr, cJSON_CreateNumber(atoi(tok)));
                tok = strtok(NULL, ",");
            }
        }
        cJSON_AddItemToObject(rec_json, "SpSkillIDs", sp_arr);
    }

    /* Also sync to entity if exists */
    for (i = 0; i < g_ed.entity_count; i++) {
        if (g_ed.entities[i].pid == g_ed.current_pid) {
            CharEntity *ent = &g_ed.entities[i];
            if (g_ed.stage) {
                cJSON *cem = cJSON_GetObjectItem(g_ed.stage, "charEntitiesMap");
                cJSON *ent_json = cem ? cJSON_GetObjectItem(cem, ent->key) : NULL;
                if (ent_json && cJSON_IsObject(ent_json)) {
                    cJSON *ent_equip = cJSON_GetObjectItem(ent_json, "EquipIDs");
                    if (!ent_equip) { ent_equip = cJSON_CreateObject(); cJSON_AddItemToObject(ent_json, "EquipIDs", ent_equip); }
                    { int k;
                      for (k = 0; k < NUM_EQUIP_SLOTS; k++) {
                          char key[8], *text;
                          int eid;
                          snprintf(key, sizeof(key), "%d", k);
                          text = get_ctrl_text_utf8(g_cbEquip[k]);
                          eid = item_parse_id(text);
                          free(text);
                          if (eid >= 0) cJSON_SetNumber(cJSON_GetObjectItem(ent_equip, key), eid);
                          else cJSON_DeleteItemFromObject(ent_equip, key);
                      }
                    }
                }
            }
            break;
        }
    }
}

/* ============================================================
 * Build character combo box
 * ============================================================ */
static void BuildCharCombo(void)
{
    int i;
    SendMessage(g_hwndCharCombo, CB_RESETCONTENT, 0, 0);

    for (i = 0; i < g_ed.entity_count; i++) {
        CharEntity *ent = &g_ed.entities[i];
        char label[256];
        wchar_t *wlabel;
        snprintf(label, sizeof(label), "%s Lv.%d (PID:%d)",
            ent->name[0] ? ent->name : "???", ent->level, ent->pid);
        utf8_to_wide(label, &wlabel, NULL);
        if (wlabel) { SendMessageW(g_hwndCharCombo, CB_ADDSTRING, 0, (LPARAM)wlabel); free(wlabel); }
    }
    for (i = 0; i < g_ed.record_count; i++) {
        CharRecord *rec = &g_ed.records[i];
        int j, has_entity = 0;
        char label[256];
        wchar_t *wlabel;
        for (j = 0; j < g_ed.entity_count; j++) {
            if (g_ed.entities[j].pid == rec->pid) { has_entity = 1; break; }
        }
        if (has_entity) continue;
        snprintf(label, sizeof(label), "%s Lv.%d (PID:%d) [仅存档]",
            rec->name[0] ? rec->name : "???", rec->level, rec->pid);
        utf8_to_wide(label, &wlabel, NULL);
        if (wlabel) { SendMessageW(g_hwndCharCombo, CB_ADDSTRING, 0, (LPARAM)wlabel); free(wlabel); }
    }

    /* Select current */
    if (g_ed.current_pid >= 0) {
        for (i = 0; i < (int)SendMessage(g_hwndCharCombo, CB_GETCOUNT, 0, 0); i++) {
            wchar_t wbuf[256];
            char buf[256];
            SendMessageW(g_hwndCharCombo, CB_GETLBTEXT, i, (LPARAM)wbuf);
            WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, buf, sizeof(buf), NULL, NULL);
            if (strstr(buf, "(PID:")) {
                char *p = strstr(buf, "(PID:") + 5;
                int pid = atoi(p);
                if (pid == g_ed.current_pid) {
                    SendMessage(g_hwndCharCombo, CB_SETCURSEL, i, 0);
                    break;
                }
            }
        }
    }
}

/* ============================================================
 * Status bar
 * ============================================================ */
static void SetStatus(const char *msg, int is_error)
{
    wchar_t *wmsg;
    int wlen;
    set_ctrl_text_utf8(g_hwndStatus, msg);
    /* Note: colored status would require custom draw; keep it simple for now */
}

/* ============================================================
 * Open file dialog + load
 * ============================================================ */
static void DoOpenFile(void)
{
    char filename[MAX_PATH] = {0};
    OPENFILENAMEA ofn;
    char default_dir[MAX_PATH];

    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_hwndMain;
    ofn.lpstrFilter = "SAV files\0*.sav\0All files\0*.*\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (data_get_default_save_dir(default_dir, MAX_PATH) == 0)
        ofn.lpstrInitialDir = default_dir;

    if (!GetOpenFileNameA(&ofn)) return;

    if (save_load(&g_ed, filename) == 0) {
        char msg[512];
        snprintf(msg, sizeof(msg), "已加载: %s (角色:%d, 实体:%d, 物品表:%d)",
            strrchr(filename, '\\') ? strrchr(filename, '\\') + 1 : filename,
            g_ed.record_count, g_ed.entity_count, g_ed.item_count);
        SetStatus(msg, 0);
        BuildCharCombo();
        RefreshEquipChoices();
        RefreshCurrentChar();
    } else {
        SetStatus(g_ed.status_msg, 1);
    }
}

/* ============================================================
 * Save file
 * ============================================================ */
static void DoSaveFile(void)
{
    char filename[MAX_PATH] = {0};
    OPENFILENAMEA ofn;

    /* Apply UI changes first */
    ApplyUIToCurrent();

    /* If we have a loaded file, save to it directly */
    if (g_ed.save_path[0]) {
        /* Backup existing file */
        if (GetFileAttributesA(g_ed.save_path) != INVALID_FILE_ATTRIBUTES) {
            char bak[MAX_PATH];
            snprintf(bak, MAX_PATH, "%s.bak", g_ed.save_path);
            CopyFileA(g_ed.save_path, bak, FALSE);
        }
        if (save_write(&g_ed, g_ed.save_path) == 0) {
            SetStatus("已保存存档", 0);
        } else {
            SetStatus(g_ed.status_msg, 1);
        }
        return;
    }

    /* Otherwise, ask for save path */
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_hwndMain;
    ofn.lpstrFilter = "SAV files\0*.sav\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = "sav";
    ofn.Flags = OFN_OVERWRITEPROMPT;

    if (!GetSaveFileNameA(&ofn)) return;

    if (save_write(&g_ed, filename) == 0) {
        SetStatus("已保存存档", 0);
    } else {
        SetStatus(g_ed.status_msg, 1);
    }
}

/* ============================================================
 * Batch max all characters
 * ============================================================ */
static void DoBatchMax(void)
{
    int i, count = 0;

    ApplyUIToCurrent();

    /* Max all records */
    for (i = 0; i < g_ed.record_count; i++) {
        cJSON *rec_json = NULL;
        cJSON *chars = cJSON_GetObjectItem(g_ed.gplay, "GDCharRecordInfo");
        char pid_str[32];
        cJSON *ba, *pfa, *fa;

        snprintf(pid_str, sizeof(pid_str), "%d", g_ed.records[i].pid);
        if (chars) rec_json = cJSON_GetObjectItem(chars, pid_str);
        if (!rec_json) continue;

        ba = cJSON_GetObjectItem(rec_json, "BaseAttr");
        pfa = cJSON_GetObjectItem(rec_json, "PermanentFightAttr");
        fa = cJSON_GetObjectItem(rec_json, "FightAttr");
        if (!ba) { ba = cJSON_CreateObject(); cJSON_AddItemToObject(rec_json, "BaseAttr", ba); }
        if (!pfa) { pfa = cJSON_CreateObject(); cJSON_AddItemToObject(rec_json, "PermanentFightAttr", pfa); }
        if (!fa) { fa = cJSON_CreateObject(); cJSON_AddItemToObject(rec_json, "FightAttr", fa); }

        cJSON_SetNumber(cJSON_GetObjectItem(ba, "Str"), 99);
        cJSON_SetNumber(cJSON_GetObjectItem(ba, "Dex"), 99);
        cJSON_SetNumber(cJSON_GetObjectItem(ba, "Mind"), 99);
        cJSON_SetNumber(cJSON_GetObjectItem(ba, "Con"), 99);
        cJSON_SetNumber(cJSON_GetObjectItem(ba, "Hp"), 999);
        cJSON_SetNumber(cJSON_GetObjectItem(ba, "Mp"), 99);

        cJSON_SetNumber(cJSON_GetObjectItem(pfa, "Str"), 999);
        cJSON_SetNumber(cJSON_GetObjectItem(pfa, "Dex"), 999);
        cJSON_SetNumber(cJSON_GetObjectItem(pfa, "Mind"), 999);
        cJSON_SetNumber(cJSON_GetObjectItem(pfa, "Con"), 999);
        cJSON_SetNumber(cJSON_GetObjectItem(pfa, "MaxHp"), 9000);
        cJSON_SetNumber(cJSON_GetObjectItem(pfa, "MaxMp"), 900);
        cJSON_SetNumber(cJSON_GetObjectItem(pfa, "PhysicalAttack"), 999);
        cJSON_SetNumber(cJSON_GetObjectItem(pfa, "MagicAttack"), 999);
        cJSON_SetNumber(cJSON_GetObjectItem(pfa, "Defense"), 999);
        cJSON_SetNumber(cJSON_GetObjectItem(pfa, "Speed"), 999);
        cJSON_SetNumber(cJSON_GetObjectItem(pfa, "CriticalRatio"), 100);
        cJSON_SetNumber(cJSON_GetObjectItem(pfa, "DodgeRatio"), 100);

        cJSON_SetNumber(cJSON_GetObjectItem(fa, "Hp"), 9999);
        cJSON_SetNumber(cJSON_GetObjectItem(fa, "MaxHp"), 9999);
        cJSON_SetNumber(cJSON_GetObjectItem(fa, "Mp"), 999);
        cJSON_SetNumber(cJSON_GetObjectItem(fa, "MaxMp"), 999);
        {
            int k;
            const char *attr_keys[] = {"Str","Dex","Mind","Con","PhysicalAttack","MagicAttack","Defense","Speed"};
            for (k = 0; k < 8; k++)
                cJSON_SetNumber(cJSON_GetObjectItem(fa, attr_keys[k]), 999);
            cJSON_SetNumber(cJSON_GetObjectItem(fa, "CriticalRatio"), 100);
            cJSON_SetNumber(cJSON_GetObjectItem(fa, "DodgeRatio"), 100);
        }
        count++;
    }

    /* Re-extract and refresh */
    data_extract_characters(&g_ed);
    BuildCharCombo();
    RefreshCurrentChar();

    {
        char msg[128];
        snprintf(msg, sizeof(msg), "已全队满属性: %d 个角色", count);
        SetStatus(msg, 0);
    }
}

/* ============================================================
 * Window Procedure
 * ============================================================ */
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_CREATE: {
        /* Top bar: Open button + path + char selector */
        HWND hTopFrame = CreateWindowExA(0, "STATIC", "",
            WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
            hwnd, NULL, GetModuleHandle(NULL), NULL);

        CreateButton(hTopFrame, ID_BTN_OPEN, "打开存档", 8, 8, 90, 26);
        CreateWindowExA(0, "STATIC", "请先打开存档文件",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            105, 12, 200, 20,
            hTopFrame, NULL, GetModuleHandle(NULL), NULL);

        CreateLabel(hTopFrame, "当前角色:", 320, 12, 60, 20);
        g_hwndCharCombo = CreateWindowExA(NULL, "COMBOBOX", "",
            WS_CHILD | WS_VISIBLE | CBS_DROPDOWN | CBS_AUTOHSCROLL | WS_VSCROLL,
            385, 8, 350, 200,
            hTopFrame, (HMENU)(INT_PTR)ID_CHAR_COMBO, GetModuleHandle(NULL), NULL);
        SendMessage(g_hwndCharCombo, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);

        /* Tab control */
        g_hwndTab = CreateWindowExA(0, WC_TABCTRLA, "",
            WS_CHILD | WS_VISIBLE | TCS_FIXEDWIDTH,
            0, 0, 0, 0,
            hwnd, (HMENU)(INT_PTR)ID_TAB, GetModuleHandle(NULL), NULL);
        SendMessage(g_hwndTab, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);

        /* Create tab pages */
        CreateTabPages(hwnd);

        /* Bottom buttons */
        CreateButton(hwnd, ID_BTN_SAVE, "保存存档", 0, 0, 90, 28);
        CreateButton(hwnd, ID_BTN_REFRESH, "刷新显示", 0, 0, 90, 28);
        CreateButton(hwnd, ID_BTN_BATCH, "全队满属性(持久)", 0, 0, 140, 28);

        /* Status bar */
        g_hwndStatus = CreateWindowExA(0, STATUSCLASSNAMEA, "",
            WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
            0, 0, 0, 0,
            hwnd, (HMENU)(INT_PTR)ID_STATUS, GetModuleHandle(NULL), NULL);
        SendMessage(g_hwndStatus, SB_SETTEXTA, 0, (LPARAM)"就绪");

        return 0;
    }

    case WM_SIZE: {
        int w = LOWORD(lp), h = HIWORD(lp);
        int top_h = 42;
        int btn_h = 34;
        int status_h = 22;
        int tab_h = h - top_h - btn_h - status_h;

        MoveWindow(g_hwndTab, 0, top_h, w, tab_h, TRUE);
        ShowTabPage(g_currentTab);

        /* Bottom buttons */
        MoveWindow(GetDlgItem(hwnd, ID_BTN_SAVE), w-200, h-status_h-btn_h+2, 90, btn_h-4, TRUE);
        MoveWindow(GetDlgItem(hwnd, ID_BTN_REFRESH), w-300, h-status_h-btn_h+2, 90, btn_h-4, TRUE);
        MoveWindow(GetDlgItem(hwnd, ID_BTN_BATCH), 8, h-status_h-btn_h+2, 140, btn_h-4, TRUE);

        /* Status bar auto-sizes */
        SendMessage(g_hwndStatus, WM_SIZE, 0, 0);
        return 0;
    }

    case WM_NOTIFY: {
        NMHDR *nm = (NMHDR *)lp;
        if (nm->idFrom == ID_TAB && nm->code == TCN_SELCHANGE) {
            int idx = TabCtrl_GetCurSel(g_hwndTab);
            ShowTabPage(idx);
            return 0;
        }
        /* ListView item click for item info */
        if (nm->code == LVN_ITEMCHANGED) {
            NMLISTVIEW *nmlv = (NMLISTVIEW *)lp;
            if (nmlv->hdr.hwndFrom == g_lvBag && (nmlv->uNewState & LVIS_SELECTED)) {
                wchar_t wbuf[32];
                char buf[32];
                ListView_GetItemText(g_lvBag, nmlv->iItem, 0, wbuf, 32);
                WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, buf, sizeof(buf), NULL, NULL);
                UpdateItemInfo(atoi(buf));
            }
            if (nmlv->hdr.hwndFrom == g_lvStorage && (nmlv->uNewState & LVIS_SELECTED)) {
                wchar_t wbuf[32];
                char buf[32];
                ListView_GetItemText(g_lvStorage, nmlv->iItem, 0, wbuf, 32);
                WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, buf, sizeof(buf), NULL, NULL);
                UpdateItemInfo(atoi(buf));
            }
        }
        return 0;
    }

    case WM_COMMAND: {
        int id = LOWORD(wp);
        int code = HIWORD(wp);

        switch (id) {
        case ID_BTN_OPEN:
            DoOpenFile();
            break;

        case ID_BTN_SAVE:
            DoSaveFile();
            break;

        case ID_BTN_REFRESH:
            ApplyUIToCurrent();
            RefreshCurrentChar();
            SetStatus("已刷新显示", 0);
            break;

        case ID_BTN_BATCH:
            DoBatchMax();
            break;

        case ID_CHAR_COMBO:
            if (code == CBN_SELCHANGE) {
                int sel = (int)SendMessage(g_hwndCharCombo, CB_GETCURSEL, 0, 0);
                if (sel >= 0) {
                    wchar_t wbuf[256];
                    char buf[256];
                    char *p;
                    int pid;
                    ApplyUIToCurrent();
                    SendMessageW(g_hwndCharCombo, CB_GETLBTEXT, sel, (LPARAM)wbuf);
                    WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, buf, sizeof(buf), NULL, NULL);
                    p = strstr(buf, "(PID:");
                    if (p) {
                        pid = atoi(p + 5);
                        g_ed.current_pid = pid;
                        RefreshCurrentChar();
                    }
                }
            }
            break;

        case ID_BTN_CLR_EQUIP:
            { int i;
              for (i = 0; i < NUM_EQUIP_SLOTS; i++)
                  SendMessageA(g_cbEquip[i], CB_SETTEXT, 0, (LPARAM)"");
              UpdateEquipInfo();
              SetStatus("已清空装备槽", 0);
            }
            break;

        case ID_BTN_FULL_HEAL:
            SetWindowTextA(g_edBat[0], GetWindowTextA(g_edBat[1], NULL, 0) ? "" : "9999");
            /* Actually: set Hp = MaxHp */
            { char buf[32]; GetWindowTextA(g_edBat[1], buf, sizeof(buf)); SetWindowTextA(g_edBat[0], buf); }
            { char buf[32]; GetWindowTextA(g_edBat[3], buf, sizeof(buf)); SetWindowTextA(g_edBat[2], buf); }
            SetStatus("已满血满蓝", 0);
            break;

        case ID_BTN_MAX_BATTLE:
            { char buf[32];
              int i;
              SetWindowTextA(g_edBat[0], "9999"); SetWindowTextA(g_edBat[1], "9999");
              SetWindowTextA(g_edBat[2], "999"); SetWindowTextA(g_edBat[3], "999");
              for (i = 6; i <= 13; i++) SetWindowTextA(g_edBat[i], "999");
              SetWindowTextA(g_edBat[14], "6");
              SetWindowTextA(g_edBat[15], "100"); SetWindowTextA(g_edBat[16], "100");
              SetStatus("战场属性已MAX", 0);
            }
            break;

        case ID_BTN_MAX_LEVEL:
            SetWindowTextA(g_edBat[4], "99");
            SetWindowTextA(g_edBat[5], "99999");
            SetStatus("等级已设为99", 0);
            break;

        case ID_BTN_BAG_ADD: {
            char *text = get_ctrl_text_utf8(g_cbBagAdd);
            int id = item_parse_id(text);
            int qty = GetWindowTextA(g_edBagQty, NULL, 0);
            char qty_buf[16];
            free(text);
            GetWindowTextA(g_edBagQty, qty_buf, sizeof(qty_buf));
            qty = atoi(qty_buf);
            if (id >= 0 && qty > 0) {
                /* Add to current character's bag */
                /* Find current record/entity and add items */
                /* For simplicity, we work through cJSON directly */
                cJSON *rec_json = NULL;
                cJSON *chars = cJSON_GetObjectItem(g_ed.gplay, "GDCharRecordInfo");
                if (chars) {
                    char pid_str[32];
                    snprintf(pid_str, sizeof(pid_str), "%d", g_ed.current_pid);
                    rec_json = cJSON_GetObjectItem(chars, pid_str);
                }
                if (rec_json) {
                    cJSON *bag = cJSON_GetObjectItem(rec_json, "ItemIDs");
                    int j;
                    if (!bag) { bag = cJSON_CreateArray(); cJSON_AddItemToObject(rec_json, "ItemIDs", bag); }
                    for (j = 0; j < qty; j++)
                        cJSON_AddItemToArray(bag, cJSON_CreateNumber(id));
                    RefreshBagList();
                    { char msg[128]; char title[128];
                      item_title(&g_ed, id, title, sizeof(title));
                      snprintf(msg, sizeof(msg), "已添加 %d 个 %s", qty, title);
                      SetStatus(msg, 0);
                    }
                }
            } else {
                SetStatus("请选择物品并输入数量", 1);
            }
            break;
        }

        case ID_BTN_BAG_DEL: {
            int sel = (int)ListView_GetNextItem(g_lvBag, -1, LVNI_SELECTED);
            if (sel >= 0) {
                wchar_t wbuf[32];
                char buf[32];
                int id;
                ListView_GetItemText(g_lvBag, sel, 0, wbuf, 32);
                WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, buf, sizeof(buf), NULL, NULL);
                id = atoi(buf);
                /* Remove all instances of this ID from bag */
                {
                    cJSON *rec_json = NULL;
                    cJSON *chars = cJSON_GetObjectItem(g_ed.gplay, "GDCharRecordInfo");
                    if (chars) {
                        char pid_str[32];
                        snprintf(pid_str, sizeof(pid_str), "%d", g_ed.current_pid);
                        rec_json = cJSON_GetObjectItem(chars, pid_str);
                    }
                    if (rec_json) {
                        cJSON *bag = cJSON_GetObjectItem(rec_json, "ItemIDs");
                        if (bag && cJSON_IsArray(bag)) {
                            int removed = 0;
                            int k = cJSON_GetArraySize(bag);
                            while (k--) {
                                cJSON *item = cJSON_GetArrayItem(bag, k);
                                if (cJSON_IsNumber(item) && item->valueint == id) {
                                    cJSON_DeleteItemFromArray(bag, k);
                                    removed++;
                                }
                            }
                            RefreshBagList();
                            { char msg[128]; char title[128];
                              item_title(&g_ed, id, title, sizeof(title));
                              snprintf(msg, sizeof(msg), "已移除 %d 个 %s", removed, title);
                              SetStatus(msg, 0);
                            }
                        }
                    }
                }
            } else {
                SetStatus("请先在背包列表中选中一行", 1);
            }
            break;
        }

        case ID_BTN_ST_ADD: {
            char *text = get_ctrl_text_utf8(g_cbStAdd);
            int id = item_parse_id(text);
            char qty_buf[16];
            int qty;
            free(text);
            GetWindowTextA(g_edStQty, qty_buf, sizeof(qty_buf));
            qty = atoi(qty_buf);
            if (id >= 0 && qty > 0) {
                cJSON *st = cJSON_GetObjectItem(g_ed.gplay, "StorageItems");
                char key[32];
                snprintf(key, sizeof(key), "%d", id);
                if (!st) { st = cJSON_CreateObject(); cJSON_AddItemToObject(g_ed.gplay, "StorageItems", st); }
                {
                    cJSON *item = cJSON_GetObjectItem(st, key);
                    int old_qty = item && cJSON_IsNumber(item) ? item->valueint : 0;
                    cJSON_SetNumber(item, old_qty + qty);
                    if (!item) {
                        cJSON_AddNumberToObject(st, key, qty);
                    }
                }
                RefreshStorageList();
                { char msg[128]; char title[128];
                  item_title(&g_ed, id, title, sizeof(title));
                  snprintf(msg, sizeof(msg), "已添加 %d 个 %s 到仓库", qty, title);
                  SetStatus(msg, 0);
                }
            } else {
                SetStatus("请选择物品并输入数量", 1);
            }
            break;
        }

        case ID_BTN_ST_DEL: {
            int sel = (int)ListView_GetNextItem(g_lvStorage, -1, LVNI_SELECTED);
            if (sel >= 0) {
                wchar_t wbuf[32];
                char buf[32];
                int id;
                ListView_GetItemText(g_lvStorage, sel, 0, wbuf, 32);
                WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, buf, sizeof(buf), NULL, NULL);
                id = atoi(buf);
                {
                    cJSON *st = cJSON_GetObjectItem(g_ed.gplay, "StorageItems");
                    if (st) {
                        char key[32];
                        snprintf(key, sizeof(key), "%d", id);
                        cJSON_DeleteItemFromObject(st, key);
                    }
                }
                RefreshStorageList();
                SetStatus("已从仓库移除", 0);
            } else {
                SetStatus("请先在仓库列表中选中一行", 1);
            }
            break;
        }
        }
        return 0;
    }

    case WM_DESTROY:
        if (g_ed.save_root) cJSON_Delete(g_ed.save_root);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcA(hwnd, msg, wp, lp);
}
