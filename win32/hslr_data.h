/*
 * hslr_data.h — Data structures, constants, and function prototypes
 * for the HSLR (幻世录重制版) save editor.
 */
#ifndef HSLR_DATA_H
#define HSLR_DATA_H

#include <windows.h>
#include "cJSON.h"

/* TCC winapi headers may not define CP_UTF8 */
#ifndef CP_UTF8
#define CP_UTF8 65001
#endif

/* TCC winapi: minimal declarations for missing functions/types */
#ifndef SNDMSG
#define SNDMSG SendMessage
#endif

/* ComboBox messages missing from TCC headers */
#ifndef CB_SETTEXT
#define CB_SETTEXT         0x014D
#define CB_GETTEXT         0x0148
#define CB_GETLBTEXT       0x0149
#define CB_GETLBTEXTLEN    0x014A
#define CB_ADDSTRING       0x0143
#define CB_RESETCONTENT    0x014B
#define CB_SETCURSEL       0x014E
#define CB_GETCURSEL       0x014F
#define CB_GETCOUNT        0x0146
#define CBS_DROPDOWN       0x00020000
#define CBS_AUTOHSCROLL    0x00000080
#define CBN_SELCHANGE      1
#endif

/* Tab control messages missing from TCC headers */
#ifndef TCM_GETCURSEL
#define TCM_GETCURSEL      (TCM_FIRST + 11)
#endif

/* ListView messages missing from TCC headers */
#ifndef LVM_INSERTCOLUMNA
#define LVM_INSERTCOLUMNA   (LVM_FIRST + 27)
#endif

/* ListView states */
#ifndef LVIS_SELECTED
#define LVIS_SELECTED       0x0002
#endif

/* WM_USER */
#ifndef WM_USER
#define WM_USER            0x0400
#endif

#ifndef WINNLS_H /* TCC may not include winnls.h */
#ifdef __cplusplus
extern "C" {
#endif
WCHAR* WINAPI CharLowerW(WCHAR*);
int WINAPI MultiByteToWideChar(UINT, DWORD, LPCSTR, int, LPWSTR, int);
int WINAPI WideCharToMultiByte(UINT, DWORD, LPCWSTR, int, LPSTR, int, LPCSTR, LPBOOL);
#ifdef __cplusplus
}
#endif
#endif

/* Helper: set a cJSON number's value (item must already exist and be a number) */
#define cJSON_SetNumber(item, val) do { \
    if (item) { (item)->valueint = (val); (item)->valuedouble = (double)(val); } \
} while(0)

/* Helper: get int from cJSON object, with default */
static int cJSON_GetIntDef(cJSON *obj, const char *key, int def)
{
    cJSON *item = cJSON_GetObjectItem(obj, key);
    if (!item) return def;
    if (cJSON_IsNumber(item)) return item->valueint;
    if (cJSON_IsString(item)) return atoi(item->valuestring);
    return def;
}

/* ============================================================
 * 加密参数 (from Python: KEY, IV, MAGIC)
 * ============================================================ */
#define HSLR_MAGIC      "ECC:"
#define HSLR_MAGIC_LEN  4

static const unsigned char HSLR_KEY[32] = {
    0x48,0x53,0x4c,0x52,0x32,0x30,0x32,0x35,0x55,0x53,0x4a,0x4f,
    0x59,0x21,0x40,0x23,0x41,0x45,0x53,0x32,0x35,0x36,0x21,0x40,
    0x23,0x46,0x4f,0x52,0x46,0x55,0x4e,0x40
};

static const unsigned char HSLR_IV[16] = {
    0x68,0x73,0x6c,0x72,0x76,0x32,0x30,0x32,
    0x35,0x30,0x35,0x30,0x37,0x30,0x30,0x31
};

/* ============================================================
 * 物品/装备槽位定义
 * ============================================================ */
typedef struct {
    int id;
    const char *name_cn;  /* 中文名 */
} EquipSlot;

#define NUM_EQUIP_SLOTS 6
static const EquipSlot EQUIP_SLOTS[NUM_EQUIP_SLOTS] = {
    {0, "头盔"}, {1, "防具"}, {2, "鞋子"},
    {3, "武器"}, {4, "饰品1"}, {5, "饰品2"}
};

/* ============================================================
 * 物品表项 (from data/items.csv)
 * ============================================================ */
typedef struct ItemInfo {
    int id;
    char name[128];        /* 名称(简中) */
    char type[64];         /* 类型 */
    char subtype[64];      /* 子类型 */
    int  slot;             /* 装备部位 (-1=非装备) */
    char slot_name[32];    /* 装备部位名 */
    char desc[512];        /* 描述(简中), '#' replaced by '\n' */
    int  price;            /* 买价 */
} ItemInfo;

#define MAX_ITEMS 2048

/* ============================================================
 * 角色数据 (GDCharRecordInfo / charEntitiesMap entity)
 * ============================================================ */
typedef struct {
    int Str, Dex, Mind, Con;
    int Hp, Mp;
} BaseAttr;

typedef struct {
    int Str, Dex, Mind, Con;
    int MaxHp, MaxMp, MaxStamina;
    int PhysicalAttack, MagicAttack, Defense, Speed;
    int CriticalRatio, DodgeRatio;
} PermFightAttr;

typedef struct {
    int Hp, MaxHp, Mp, MaxMp, Stamina, MaxStamina;
    int Str, Dex, Mind, Con;
    int PhysicalAttack, MagicAttack, Defense, Speed, Move;
    int CriticalRatio, DodgeRatio;
    int FireRes, WaterRes, AirRes, EarthRes, MindRes;
} FightAttr;

#define MAX_BAG_ITEMS 256

/* Character record — corresponds to GDCharRecordInfo entry */
typedef struct {
    int pid;               /* PlayerId */
    char name[128];
    int level;
    int exp;

    BaseAttr      base;
    PermFightAttr perm;
    FightAttr     fight;

    /* Equipment: slot(int) -> item_id(int). -1 = empty */
    int equip[NUM_EQUIP_SLOTS];

    /* Bag items: array of item IDs (duplicates = quantity) */
    int bag[MAX_BAG_ITEMS];
    int bag_count;

    /* Skills */
    int nrl_skill;              /* NrlSkillId */
    int magic_skills[64];
    int magic_skill_count;
    int sp_skills[64];
    int sp_skill_count;
} CharRecord;

/* Character battle entity (from charEntitiesMap) */
typedef struct {
    int pid;
    char key[64];           /* key in charEntitiesMap */
    char name[128];
    int camp;               /* 1=enemy, 2=ally, 3=neutral */
    int level;
    int exp;

    int hp, max_hp, mp, max_mp, stamina, max_stamina;
    int is_dead;
    BaseAttr  base;
    FightAttr fight;

    /* Equipment + items + skills (same as record) */
    int equip[NUM_EQUIP_SLOTS];
    int bag[MAX_BAG_ITEMS];
    int bag_count;
    int nrl_skill;
    int magic_skills[64];
    int magic_skill_count;
    int sp_skills[64];
    int sp_skill_count;
} CharEntity;

/* ============================================================
 * 全局编辑器状态
 * ============================================================ */
typedef struct {
    /* Raw save data (top-level JSON) */
    cJSON *save_root;      /* root JSON object */

    /* Parsed sub-objects */
    cJSON *gplay;          /* gplay dict (GDCharRecordInfo, StorageItems, ...) */
    cJSON *stage;          /* stage dict (charEntitiesMap) or NULL */

    /* Characters */
    CharRecord  records[64];
    int         record_count;
    CharEntity  entities[64];
    int         entity_count;

    /* Current editing state */
    int current_idx;       /* index into entities[] or records[] */
    int current_pid;

    /* Storage items (全队仓库): item_id -> quantity */
    int storage_ids[MAX_ITEMS];
    int storage_qty[MAX_ITEMS];
    int storage_count;

    /* Item table */
    ItemInfo items[MAX_ITEMS];
    int item_count;

    /* Current file path */
    char save_path[MAX_PATH];

    /* Status message */
    char status_msg[512];
    int  status_is_error;
} EditorState;

/* ============================================================
 /* Helper: get cJSON int value (with default) -- usable from hslr_gui.c */
static int cJSON_GetInt(cJSON *obj, const char *key, int def)
{
    cJSON *item = cJSON_GetObjectItem(obj, key);
    if (!item) return def;
    if (cJSON_IsNumber(item)) return item->valueint;
    if (cJSON_IsString(item)) return atoi(item->valuestring);
    return def;
}

/* Helper: set cJSON int value -- usable from hslr_gui.c */
static void cJSON_SetInt(cJSON *obj, const char *key, int val)
{
    cJSON *item = cJSON_GetObjectItem(obj, key);
    if (item && cJSON_IsNumber(item)) {
        item->valueint = val;
        item->valuedouble = (double)val;
    } else {
        cJSON_AddNumberToObject(obj, key, val);
    }
}

/* ============================================================
 * Function prototypes — hslr_save.c
 * ============================================================ */

/* Load a .sav file: decrypt → gunzip → parse JSON.
 * Returns 0 on success. */
int save_load(EditorState *ed, const char *path);

/* Save the current state back to a .sav file.
 * Returns 0 on success. */
int save_write(EditorState *ed, const char *path);

/* ============================================================
 * Function prototypes — hslr_data.c
 * ============================================================ */

/* Load item table from data/items.csv */
int item_table_load(EditorState *ed, const char *csv_path);

/* Get item info by ID. Returns NULL if not found. */
const ItemInfo *item_get(EditorState *ed, int id);

/* Parse item ID from text like "2 斧剑" or "2". Returns -1 if invalid. */
int item_parse_id(const char *text);

/* Get item label for display: "ID Name [Type/Subtype]" */
void item_label(EditorState *ed, int id, char *buf, int buf_size);

/* Get item title: "ID Name (Type/Subtype)" */
void item_title(EditorState *ed, int id, char *buf, int buf_size);

/* Extract characters from parsed JSON */
void data_extract_characters(EditorState *ed);

/* Sync UI data back to cJSON tree (for saving) */
void data_sync_to_json(EditorState *ed);

/* Get default save directory path */
int data_get_default_save_dir(char *buf, int buf_size);

/* ============================================================
 * UTF-8 helpers for Win32 API
 * ============================================================ */
/* Convert UTF-8 string to wide string. Caller must free(*out_w). */
int utf8_to_wide(const char *utf8, wchar_t **out_w, int *out_w_len);

/* Convert wide string to UTF-8. Caller must free(*out_u). */
int wide_to_utf8(const wchar_t *wide, char **out_u, int *out_u_len);

/* Set text of a Win32 control from UTF-8 string */
void set_ctrl_text_utf8(HWND hwnd, const char *utf8);

/* Get text from a Win32 control as UTF-8. Caller must free(*out). */
char *get_ctrl_text_utf8(HWND hwnd);

#endif /* HSLR_DATA_H */
