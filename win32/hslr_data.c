/*
 * hslr_data.c — Game data management: CSV loading, character extraction,
 *                JSON sync, UTF-8 helpers.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "hslr_data.h"

/* ============================================================
 * UTF-8 / Wide string helpers
 * ============================================================ */
int utf8_to_wide(const char *utf8, wchar_t **out_w, int *out_w_len)
{
    int wlen;
    wchar_t *wbuf;

    if (!utf8 || !out_w || !out_w_len) return -1;

    wlen = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
    if (wlen <= 0) return -1;

    wbuf = (wchar_t *)malloc((size_t)wlen * sizeof(wchar_t));
    if (!wbuf) return -1;

    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wbuf, wlen);

    *out_w = wbuf;
    *out_w_len = wlen;
    return 0;
}

int wide_to_utf8(const wchar_t *wide, char **out_u, int *out_u_len)
{
    int ulen;
    char *ubuf;

    if (!wide || !out_u || !out_u_len) return -1;

    ulen = WideCharToMultiByte(CP_UTF8, 0, wide, -1, NULL, 0, NULL, NULL);
    if (ulen <= 0) return -1;

    ubuf = (char *)malloc((size_t)ulen);
    if (!ubuf) return -1;

    WideCharToMultiByte(CP_UTF8, 0, wide, -1, ubuf, ulen, NULL, NULL);

    *out_u = ubuf;
    *out_u_len = ulen;
    return 0;
}

void set_ctrl_text_utf8(HWND hwnd, const char *utf8)
{
    wchar_t *w = NULL;
    int wlen = 0;
    if (!utf8 || !hwnd) return;
    if (utf8_to_wide(utf8, &w, &wlen) == 0) {
        SetWindowTextW(hwnd, w);
        free(w);
    }
}

char *get_ctrl_text_utf8(HWND hwnd)
{
    int wlen, ulen;
    wchar_t *wbuf;
    char *ubuf;

    if (!hwnd) return NULL;

    wlen = GetWindowTextLengthW(hwnd) + 1;
    wbuf = (wchar_t *)malloc((size_t)wlen * sizeof(wchar_t));
    if (!wbuf) return NULL;

    GetWindowTextW(hwnd, wbuf, wlen);

    if (wide_to_utf8(wbuf, &ubuf, &ulen) != 0) {
        free(wbuf);
        return NULL;
    }
    free(wbuf);
    return ubuf;
}

/* ============================================================
 * Item table: load from data/items.csv
 * ============================================================ */

/* Simple CSV field parser (handles quoted fields with commas/newlines) */
static int csv_next_field(const char **pos, char *field, int field_size)
{
    const char *p = *pos;
    int fi = 0;

    if (*p == '\0') return 0;  /* end of string */

    if (*p == '"') {
        /* Quoted field */
        p++;  /* skip opening quote */
        while (*p && fi < field_size - 1) {
            if (*p == '"') {
                if (p[1] == '"') {
                    field[fi++] = '"';
                    p += 2;
                } else {
                    p++;  /* skip closing quote */
                    break;
                }
            } else {
                field[fi++] = *p++;
            }
        }
        /* Skip to next comma or newline */
        while (*p && *p != ',' && *p != '\n' && *p != '\r') p++;
    } else {
        /* Unquoted field */
        while (*p && *p != ',' && *p != '\n' && *p != '\r' && fi < field_size - 1)
            field[fi++] = *p++;
    }
    field[fi] = '\0';

    /* Skip delimiter */
    if (*p == ',') p++;
    else if (*p == '\r') p++;
    if (*p == '\n') p++;

    *pos = p;
    return 1;
}

static void str_trim(char *s)
{
    char *end;
    while (*s && isspace((unsigned char)*s)) memmove(s, s+1, strlen(s));
    end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1])) end--;
    *end = '\0';
}

static void str_replace_char(char *s, char from, char to)
{
    while (*s) { if (*s == from) *s = to; s++; }
}

static const char *SLOT_ID_NAMES[] = {
    "Head", "Body", "Feet", "Weapon", "Amulet1", "Amulet2"
};

int item_table_load(EditorState *ed, const char *csv_path)
{
    FILE *f;
    char line[4096];
    char field[1024];
    const char *p;
    int header_done = 0;
    int col_id = -1, col_name_cn = -1, col_name_tw = -1;
    int col_type = -1, col_subtype = -1, col_slot = -1, col_price = -1;
    int col_desc_cn = -1, col_desc_tw = -1;
    int col_count = 0;

    ed->item_count = 0;

    f = fopen(csv_path, "r, ccs=UTF-8");
    if (!f) {
        /* Try without BOM encoding */
        f = fopen(csv_path, "r");
    }
    if (!f) return -1;

    while (fgets(line, sizeof(line), f)) {
        /* Remove BOM if present at start of file */
        if (!header_done && (unsigned char)line[0] == 0xEF &&
            (unsigned char)line[1] == 0xBB && (unsigned char)line[2] == 0xBF) {
            memmove(line, line+3, strlen(line+3)+1);
        }

        p = line;
        col_count = 0;

        if (!header_done) {
            /* Parse header to find column indices */
            while (csv_next_field(&p, field, sizeof(field))) {
                str_trim(field);
                if (strcmp(field, "ID") == 0) col_id = col_count;
                else if (strcmp(field, "名称(简中)") == 0) col_name_cn = col_count;
                else if (strcmp(field, "名称(繁中)") == 0) col_name_tw = col_count;
                else if (strcmp(field, "类型") == 0) col_type = col_count;
                else if (strcmp(field, "子类型") == 0) col_subtype = col_count;
                else if (strcmp(field, "装备部位") == 0) col_slot = col_count;
                else if (strcmp(field, "买价") == 0) col_price = col_count;
                else if (strcmp(field, "描述(简中)") == 0) col_desc_cn = col_count;
                else if (strcmp(field, "描述(繁中)") == 0) col_desc_tw = col_count;
                col_count++;
            }
            header_done = 1;
            continue;
        }

        /* Parse data row */
        if (col_id < 0 || ed->item_count >= MAX_ITEMS) continue;

        {
            ItemInfo *it = &ed->items[ed->item_count];
            int c = 0;
            char slot_str[64] = {0};

            memset(it, 0, sizeof(ItemInfo));
            it->slot = -1;

            p = line;
            while (csv_next_field(&p, field, sizeof(field))) {
                str_trim(field);

                if (c == col_id) {
                    it->id = atoi(field);
                } else if (c == col_name_cn && field[0]) {
                    strncpy(it->name, field, sizeof(it->name)-1);
                } else if (c == col_name_tw && field[0] && it->name[0] == '\0') {
                    strncpy(it->name, field, sizeof(it->name)-1);
                } else if (c == col_type && field[0]) {
                    strncpy(it->type, field, sizeof(it->type)-1);
                } else if (c == col_subtype && field[0]) {
                    strncpy(it->subtype, field, sizeof(it->subtype)-1);
                } else if (c == col_slot && field[0]) {
                    strncpy(slot_str, field, sizeof(slot_str)-1);
                    strncpy(it->slot_name, field, sizeof(it->slot_name)-1);
                } else if (c == col_price && field[0]) {
                    it->price = atoi(field);
                } else if (c == col_desc_cn && field[0]) {
                    strncpy(it->desc, field, sizeof(it->desc)-1);
                } else if (c == col_desc_tw && field[0] && it->desc[0] == '\0') {
                    strncpy(it->desc, field, sizeof(it->desc)-1);
                }
                c++;
            }

            if (it->id <= 0) continue;

            /* Map slot name to ID */
            if (slot_str[0]) {
                int s;
                for (s = 0; s < NUM_EQUIP_SLOTS; s++) {
                    if (_stricmp(slot_str, SLOT_ID_NAMES[s]) == 0) {
                        it->slot = EQUIP_SLOTS[s].id;
                        break;
                    }
                }
            }

            /* Replace '#' with '\n' in description */
            str_replace_char(it->desc, '#', '\n');

            ed->item_count++;
        }
    }

    fclose(f);
    return 0;
}

const ItemInfo *item_get(EditorState *ed, int id)
{
    int i;
    for (i = 0; i < ed->item_count; i++) {
        if (ed->items[i].id == id)
            return &ed->items[i];
    }
    return NULL;
}

int item_parse_id(const char *text)
{
    const char *p;
    if (!text || !*text) return -1;
    /* Skip leading whitespace */
    p = text;
    while (*p && isspace((unsigned char)*p)) p++;
    if (!*p || !isdigit((unsigned char)*p)) return -1;
    return atoi(p);
}

void item_label(EditorState *ed, int id, char *buf, int buf_size)
{
    const ItemInfo *it;
    if (id < 0 || !buf || buf_size <= 0) { if (buf) buf[0] = '\0'; return; }

    it = item_get(ed, id);
    if (!it) {
        snprintf(buf, buf_size, "%d", id);
    } else if (it->name[0] && (it->type[0] || it->subtype[0])) {
        char extra[128] = {0};
        if (it->type[0] && it->subtype[0])
            snprintf(extra, sizeof(extra), "%s/%s", it->type, it->subtype);
        else if (it->type[0])
            strncpy(extra, it->type, sizeof(extra)-1);
        else
            strncpy(extra, it->subtype, sizeof(extra)-1);
        snprintf(buf, buf_size, "%d %s [%s]", id, it->name, extra);
    } else if (it->name[0]) {
        snprintf(buf, buf_size, "%d %s", id, it->name);
    } else {
        snprintf(buf, buf_size, "%d", id);
    }
}

void item_title(EditorState *ed, int id, char *buf, int buf_size)
{
    const ItemInfo *it;
    if (id < 0 || !buf || buf_size <= 0) { if (buf) buf[0] = '\0'; return; }

    it = item_get(ed, id);
    if (!it) {
        snprintf(buf, buf_size, "%d (数据表中无此ID)", id);
    } else {
        char extra[128] = {0};
        if (it->type[0] && it->subtype[0])
            snprintf(extra, sizeof(extra), "%s/%s", it->type, it->subtype);
        else if (it->type[0])
            strncpy(extra, it->type, sizeof(extra)-1);
        else if (it->subtype[0])
            strncpy(extra, it->subtype, sizeof(extra)-1);
        if (extra[0])
            snprintf(buf, buf_size, "%d %s（%s）", id, it->name, extra);
        else
            snprintf(buf, buf_size, "%d %s", id, it->name);
    }
}

/* ============================================================
 * Character data extraction from parsed JSON
 * ============================================================ */

/* Helper: get cJSON item and return its int value, or default */
static int cJSON_GetInt(cJSON *obj, const char *key, int def)
{
    cJSON *item = cJSON_GetObjectItem(obj, key);
    if (!item) return def;
    if (cJSON_IsNumber(item)) return item->valueint;
    if (cJSON_IsString(item)) return atoi(item->valuestring);
    return def;
}

static const char *cJSON_GetStr(cJSON *obj, const char *key, const char *def)
{
    cJSON *item = cJSON_GetObjectItem(obj, key);
    if (!item) return def;
    if (cJSON_IsString(item)) return item->valuestring;
    return def;
}

/* Parse BaseAttr from cJSON */
static void parse_base_attr(cJSON *obj, BaseAttr *ba)
{
    if (!obj) { memset(ba, 0, sizeof(*ba)); return; }
    ba->Str  = cJSON_GetInt(obj, "Str", 0);
    ba->Dex  = cJSON_GetInt(obj, "Dex", 0);
    ba->Mind = cJSON_GetInt(obj, "Mind", 0);
    ba->Con  = cJSON_GetInt(obj, "Con", 0);
    ba->Hp   = cJSON_GetInt(obj, "Hp", 0);
    ba->Mp   = cJSON_GetInt(obj, "Mp", 0);
}

/* Parse PermanentFightAttr */
static void parse_perm_attr(cJSON *obj, PermFightAttr *pfa)
{
    if (!obj) { memset(pfa, 0, sizeof(*pfa)); return; }
    pfa->Str  = cJSON_GetInt(obj, "Str", 0);
    pfa->Dex  = cJSON_GetInt(obj, "Dex", 0);
    pfa->Mind = cJSON_GetInt(obj, "Mind", 0);
    pfa->Con  = cJSON_GetInt(obj, "Con", 0);
    pfa->MaxHp = cJSON_GetInt(obj, "MaxHp", 0);
    pfa->MaxMp = cJSON_GetInt(obj, "MaxMp", 0);
    pfa->PhysicalAttack = cJSON_GetInt(obj, "PhysicalAttack", 0);
    pfa->MagicAttack   = cJSON_GetInt(obj, "MagicAttack", 0);
    pfa->Defense       = cJSON_GetInt(obj, "Defense", 0);
    pfa->Speed         = cJSON_GetInt(obj, "Speed", 0);
    pfa->CriticalRatio = cJSON_GetInt(obj, "CriticalRatio", 0);
    pfa->DodgeRatio    = cJSON_GetInt(obj, "DodgeRatio", 0);
}

/* Parse FightAttr */
static void parse_fight_attr(cJSON *obj, FightAttr *fa)
{
    if (!obj) { memset(fa, 0, sizeof(*fa)); return; }
    fa->Hp   = cJSON_GetInt(obj, "Hp", 0);
    fa->MaxHp = cJSON_GetInt(obj, "MaxHp", 0);
    fa->Mp   = cJSON_GetInt(obj, "Mp", 0);
    fa->MaxMp = cJSON_GetInt(obj, "MaxMp", 0);
    fa->Str  = cJSON_GetInt(obj, "Str", 0);
    fa->Dex  = cJSON_GetInt(obj, "Dex", 0);
    fa->Mind = cJSON_GetInt(obj, "Mind", 0);
    fa->Con  = cJSON_GetInt(obj, "Con", 0);
    fa->PhysicalAttack = cJSON_GetInt(obj, "PhysicalAttack", 0);
    fa->MagicAttack    = cJSON_GetInt(obj, "MagicAttack", 0);
    fa->Defense        = cJSON_GetInt(obj, "Defense", 0);
    fa->Speed          = cJSON_GetInt(obj, "Speed", 0);
    fa->Move           = cJSON_GetInt(obj, "Move", 0);
    fa->CriticalRatio  = cJSON_GetInt(obj, "CriticalRatio", 0);
    fa->DodgeRatio     = cJSON_GetInt(obj, "DodgeRatio", 0);
    fa->FireRes   = cJSON_GetInt(obj, "FireRes", 0);
    fa->WaterRes  = cJSON_GetInt(obj, "WaterRes", 0);
    fa->AirRes    = cJSON_GetInt(obj, "AirRes", 0);
    fa->EarthRes  = cJSON_GetInt(obj, "EarthRes", 0);
    fa->MindRes   = cJSON_GetInt(obj, "MindRes", 0);
}

/* Parse equipment IDs: {"0": 5, "1": 12, ...} or string values */
static void parse_equips(cJSON *obj, int equips[], int count)
{
    int i;
    for (i = 0; i < count; i++) equips[i] = -1;
    if (!obj || !cJSON_IsObject(obj)) return;

    for (i = 0; i < count; i++) {
        char key[8];
        cJSON *item;
        snprintf(key, sizeof(key), "%d", i);
        item = cJSON_GetObjectItem(obj, key);
        if (item) {
            if (cJSON_IsNumber(item)) equips[i] = item->valueint;
            else if (cJSON_IsString(item)) equips[i] = atoi(item->valuestring);
        }
    }
}

/* Parse bag items: [id, id, id, ...] */
static void parse_bag(cJSON *arr, int bag[], int *count)
{
    int i;
    *count = 0;
    if (!arr || !cJSON_IsArray(arr)) return;

    for (i = 0; i < cJSON_GetArraySize(arr) && *count < MAX_BAG_ITEMS; i++) {
        cJSON *item = cJSON_GetArrayItem(arr, i);
        if (cJSON_IsNumber(item))
            bag[(*count)++] = item->valueint;
        else if (cJSON_IsString(item))
            bag[(*count)++] = atoi(item->valuestring);
    }
}

/* Parse skill ID list */
static void parse_skill_ids(cJSON *arr, int ids[], int *count)
{
    int i;
    *count = 0;
    if (!arr || !cJSON_IsArray(arr)) return;
    for (i = 0; i < cJSON_GetArraySize(arr) && *count < 64; i++) {
        cJSON *item = cJSON_GetArrayItem(arr, i);
        if (cJSON_IsNumber(item))
            ids[(*count)++] = item->valueint;
        else if (cJSON_IsString(item))
            ids[(*count)++] = atoi(item->valuestring);
    }
}

/* Extract one character record from GDCharRecordInfo entry */
static void extract_record(EditorState *ed, cJSON *rec_json, int pid)
{
    CharRecord *rec;
    cJSON *ba_obj, *pfa_obj, *fa_obj;
    const char *name;
    cJSON *equip_obj, *bag_arr;
    cJSON *nrl_id, *magic_arr, *sp_arr;

    if (ed->record_count >= 64) return;
    rec = &ed->records[ed->record_count];

    memset(rec, 0, sizeof(CharRecord));
    rec->pid = pid;

    name = cJSON_GetStr(rec_json, "Name", "");
    strncpy(rec->name, name, sizeof(rec->name)-1);

    rec->level = cJSON_GetInt(rec_json, "Level", 0);
    rec->exp   = cJSON_GetInt(rec_json, "Exp", 0);

    ba_obj  = cJSON_GetObjectItem(rec_json, "BaseAttr");
    pfa_obj = cJSON_GetObjectItem(rec_json, "PermanentFightAttr");
    fa_obj  = cJSON_GetObjectItem(rec_json, "FightAttr");

    parse_base_attr(ba_obj, &rec->base);
    parse_perm_attr(pfa_obj, &rec->perm);
    parse_fight_attr(fa_obj, &rec->fight);

    equip_obj = cJSON_GetObjectItem(rec_json, "EquipIDs");
    parse_equips(equip_obj, rec->equip, NUM_EQUIP_SLOTS);

    bag_arr = cJSON_GetObjectItem(rec_json, "ItemIDs");
    parse_bag(bag_arr, rec->bag, &rec->bag_count);

    nrl_id = cJSON_GetObjectItem(rec_json, "NrlSkillId");
    if (nrl_id && cJSON_IsNumber(nrl_id)) rec->nrl_skill = nrl_id->valueint;

    magic_arr = cJSON_GetObjectItem(rec_json, "MagicSkillIDs");
    parse_skill_ids(magic_arr, rec->magic_skills, &rec->magic_skill_count);

    sp_arr = cJSON_GetObjectItem(rec_json, "SpSkillIDs");
    parse_skill_ids(sp_arr, rec->sp_skills, &rec->sp_skill_count);

    ed->record_count++;
}

/* Extract one character entity from charEntitiesMap entry */
static void extract_entity(EditorState *ed, cJSON *ent_json, const char *key)
{
    CharEntity *ent;
    cJSON *ba_obj, *fa_obj;
    const char *name;
    cJSON *equip_obj, *bag_arr;
    cJSON *nrl_id, *magic_arr, *sp_arr;
    int pid, camp;

    if (ed->entity_count >= 64) return;

    pid  = cJSON_GetInt(ent_json, "PlayerId", -1);
    camp = cJSON_GetInt(ent_json, "Camp", 0);

    if (camp != 2) return;  /* Only extract ally characters (Camp=2) */

    ent = &ed->entities[ed->entity_count];
    memset(ent, 0, sizeof(CharEntity));

    ent->pid = pid;
    strncpy(ent->key, key, sizeof(ent->key)-1);

    name = cJSON_GetStr(ent_json, "Name", "");
    strncpy(ent->name, name, sizeof(ent->name)-1);

    ent->camp  = camp;
    ent->level = cJSON_GetInt(ent_json, "Level", 0);
    ent->exp   = cJSON_GetInt(ent_json, "Exp", 0);

    ent->hp     = cJSON_GetInt(ent_json, "Hp", 0);
    ent->max_hp = cJSON_GetInt(ent_json, "MaxHp", 0);
    ent->mp     = cJSON_GetInt(ent_json, "Mp", 0);
    ent->max_mp = cJSON_GetInt(ent_json, "MaxMp", 0);

    ba_obj = cJSON_GetObjectItem(ent_json, "BaseAttr");
    fa_obj = cJSON_GetObjectItem(ent_json, "FightAttr");
    parse_base_attr(ba_obj, &ent->base);
    parse_fight_attr(fa_obj, &ent->fight);

    equip_obj = cJSON_GetObjectItem(ent_json, "EquipIDs");
    parse_equips(equip_obj, ent->equip, NUM_EQUIP_SLOTS);

    bag_arr = cJSON_GetObjectItem(ent_json, "ItemIDs");
    parse_bag(bag_arr, ent->bag, &ent->bag_count);

    nrl_id = cJSON_GetObjectItem(ent_json, "NrlSkillID");
    if (nrl_id && cJSON_IsNumber(nrl_id)) ent->nrl_skill = nrl_id->valueint;

    magic_arr = cJSON_GetObjectItem(ent_json, "MagicSkillIDs");
    parse_skill_ids(magic_arr, ent->magic_skills, &ent->magic_skill_count);

    sp_arr = cJSON_GetObjectItem(ent_json, "SpSkillIDs");
    parse_skill_ids(sp_arr, ent->sp_skills, &ent->sp_skill_count);

    ed->entity_count++;
}

void data_extract_characters(EditorState *ed)
{
    cJSON *gp_chars, *cem;
    int i;

    ed->record_count = 0;
    ed->entity_count = 0;

    /* Extract from GDCharRecordInfo */
    gp_chars = cJSON_GetObjectItem(ed->gplay, "GDCharRecordInfo");
    if (gp_chars && cJSON_IsObject(gp_chars)) {
        cJSON *child = gp_chars->child;
        while (child) {
            if (cJSON_IsString(child)) {
                int pid = atoi(child->string);
                cJSON *rec = cJSON_Parse(child->valuestring);
                if (rec) {
                    extract_record(ed, rec, pid);
                    cJSON_Delete(rec);
                }
            } else if (cJSON_IsObject(child)) {
                int pid = atoi(child->string);
                extract_record(ed, child, pid);
            }
            child = child->next;
        }
    }

    /* Extract from charEntitiesMap (stage) */
    cem = NULL;
    if (ed->stage) cem = cJSON_GetObjectItem(ed->stage, "charEntitiesMap");
    if (cem && cJSON_IsObject(cem)) {
        cJSON *child = cem->child;
        while (child) {
            if (cJSON_IsString(child)) {
                cJSON *ent = cJSON_Parse(child->valuestring);
                if (ent) {
                    extract_entity(ed, ent, child->string);
                    cJSON_Delete(ent);
                }
            } else if (cJSON_IsObject(child)) {
                extract_entity(ed, child, child->string);
            }
            child = child->next;
        }
    }

    /* Extract storage items */
    ed->storage_count = 0;
    if (ed->gplay) {
        cJSON *st = cJSON_GetObjectItem(ed->gplay, "StorageItems");
        if (st && cJSON_IsObject(st)) {
            cJSON *child = st->child;
            while (child) {
                if (ed->storage_count < MAX_ITEMS) {
                    ed->storage_ids[ed->storage_count] = atoi(child->string);
                    if (cJSON_IsNumber(child))
                        ed->storage_qty[ed->storage_count] = child->valueint;
                    else if (cJSON_IsString(child))
                        ed->storage_qty[ed->storage_count] = atoi(child->valuestring);
                    ed->storage_count++;
                }
                child = child->next;
            }
        }
    }

    /* Select first character */
    ed->current_idx = 0;
    if (ed->entity_count > 0)
        ed->current_pid = ed->entities[0].pid;
    else if (ed->record_count > 0)
        ed->current_pid = ed->records[0].pid;
    else
        ed->current_pid = -1;
}

/* ============================================================
 * Sync character data back to cJSON (for saving)
 * ============================================================ */

/* Helper: set cJSON int value */
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

/* Helper: set cJSON string value */
static void cJSON_SetStr(cJSON *obj, const char *key, const char *val)
{
    cJSON *item = cJSON_GetObjectItem(obj, key);
    if (item && cJSON_IsString(item)) {
        free(item->valuestring);
        item->valuestring = strdup(val);
    } else {
        cJSON_AddStringToObject(obj, key, val);
    }
}

/* Sync a CharRecord back to its cJSON object */
static void sync_record_to_json(EditorState *ed, CharRecord *rec, cJSON *rec_json)
{
    cJSON *ba_obj, *pfa_obj, *fa_obj;
    cJSON *equip_obj, *bag_arr, *magic_arr, *sp_arr;
    int i;

    if (!rec_json) return;

    cJSON_SetStr(rec_json, "Name", rec->name);
    cJSON_SetInt(rec_json, "Level", rec->level);
    cJSON_SetInt(rec_json, "Exp", rec->exp);

    /* BaseAttr */
    ba_obj = cJSON_GetObjectItem(rec_json, "BaseAttr");
    if (!ba_obj) { ba_obj = cJSON_CreateObject(); cJSON_AddItemToObject(rec_json, "BaseAttr", ba_obj); }
    cJSON_SetInt(ba_obj, "Str", rec->base.Str);
    cJSON_SetInt(ba_obj, "Dex", rec->base.Dex);
    cJSON_SetInt(ba_obj, "Mind", rec->base.Mind);
    cJSON_SetInt(ba_obj, "Con", rec->base.Con);
    cJSON_SetInt(ba_obj, "Hp", rec->base.Hp);
    cJSON_SetInt(ba_obj, "Mp", rec->base.Mp);

    /* PermanentFightAttr */
    pfa_obj = cJSON_GetObjectItem(rec_json, "PermanentFightAttr");
    if (!pfa_obj) { pfa_obj = cJSON_CreateObject(); cJSON_AddItemToObject(rec_json, "PermanentFightAttr", pfa_obj); }
    cJSON_SetInt(pfa_obj, "Str", rec->perm.Str);
    cJSON_SetInt(pfa_obj, "Dex", rec->perm.Dex);
    cJSON_SetInt(pfa_obj, "Mind", rec->perm.Mind);
    cJSON_SetInt(pfa_obj, "Con", rec->perm.Con);
    cJSON_SetInt(pfa_obj, "MaxHp", rec->perm.MaxHp);
    cJSON_SetInt(pfa_obj, "MaxMp", rec->perm.MaxMp);
    cJSON_SetInt(pfa_obj, "PhysicalAttack", rec->perm.PhysicalAttack);
    cJSON_SetInt(pfa_obj, "MagicAttack", rec->perm.MagicAttack);
    cJSON_SetInt(pfa_obj, "Defense", rec->perm.Defense);
    cJSON_SetInt(pfa_obj, "Speed", rec->perm.Speed);
    cJSON_SetInt(pfa_obj, "CriticalRatio", rec->perm.CriticalRatio);
    cJSON_SetInt(pfa_obj, "DodgeRatio", rec->perm.DodgeRatio);

    /* FightAttr */
    fa_obj = cJSON_GetObjectItem(rec_json, "FightAttr");
    if (!fa_obj) { fa_obj = cJSON_CreateObject(); cJSON_AddItemToObject(rec_json, "FightAttr", fa_obj); }
    cJSON_SetInt(fa_obj, "Hp", rec->fight.Hp);
    cJSON_SetInt(fa_obj, "MaxHp", rec->fight.MaxHp);
    cJSON_SetInt(fa_obj, "Mp", rec->fight.Mp);
    cJSON_SetInt(fa_obj, "MaxMp", rec->fight.MaxMp);
    cJSON_SetInt(fa_obj, "Str", rec->fight.Str);
    cJSON_SetInt(fa_obj, "Dex", rec->fight.Dex);
    cJSON_SetInt(fa_obj, "Mind", rec->fight.Mind);
    cJSON_SetInt(fa_obj, "Con", rec->fight.Con);
    cJSON_SetInt(fa_obj, "PhysicalAttack", rec->fight.PhysicalAttack);
    cJSON_SetInt(fa_obj, "MagicAttack", rec->fight.MagicAttack);
    cJSON_SetInt(fa_obj, "Defense", rec->fight.Defense);
    cJSON_SetInt(fa_obj, "Speed", rec->fight.Speed);
    cJSON_SetInt(fa_obj, "Move", rec->fight.Move);
    cJSON_SetInt(fa_obj, "CriticalRatio", rec->fight.CriticalRatio);
    cJSON_SetInt(fa_obj, "DodgeRatio", rec->fight.DodgeRatio);
    cJSON_SetInt(fa_obj, "FireRes", rec->fight.FireRes);
    cJSON_SetInt(fa_obj, "WaterRes", rec->fight.WaterRes);
    cJSON_SetInt(fa_obj, "AirRes", rec->fight.AirRes);
    cJSON_SetInt(fa_obj, "EarthRes", rec->fight.EarthRes);
    cJSON_SetInt(fa_obj, "MindRes", rec->fight.MindRes);

    /* Equipment */
    equip_obj = cJSON_GetObjectItem(rec_json, "EquipIDs");
    if (!equip_obj) { equip_obj = cJSON_CreateObject(); cJSON_AddItemToObject(rec_json, "EquipIDs", equip_obj); }
    for (i = 0; i < NUM_EQUIP_SLOTS; i++) {
        char key[8];
        snprintf(key, sizeof(key), "%d", i);
        if (rec->equip[i] >= 0)
            cJSON_SetInt(equip_obj, key, rec->equip[i]);
        else
            cJSON_DeleteItemFromObject(equip_obj, key);
    }

    /* Bag items */
    bag_arr = cJSON_GetObjectItem(rec_json, "ItemIDs");
    if (bag_arr) cJSON_DeleteItemFromObject(rec_json, "ItemIDs");
    bag_arr = cJSON_CreateArray();
    for (i = 0; i < rec->bag_count; i++)
        cJSON_AddItemToArray(bag_arr, cJSON_CreateNumber(rec->bag[i]));
    cJSON_AddItemToObject(rec_json, "ItemIDs", bag_arr);

    /* Skills */
    cJSON_SetInt(rec_json, "NrlSkillId", rec->nrl_skill);
    magic_arr = cJSON_GetObjectItem(rec_json, "MagicSkillIDs");
    if (magic_arr) cJSON_DeleteItemFromObject(rec_json, "MagicSkillIDs");
    magic_arr = cJSON_CreateArray();
    for (i = 0; i < rec->magic_skill_count; i++)
        cJSON_AddItemToArray(magic_arr, cJSON_CreateNumber(rec->magic_skills[i]));
    cJSON_AddItemToObject(rec_json, "MagicSkillIDs", magic_arr);

    sp_arr = cJSON_GetObjectItem(rec_json, "SpSkillIDs");
    if (sp_arr) cJSON_DeleteItemFromObject(rec_json, "SpSkillIDs");
    sp_arr = cJSON_CreateArray();
    for (i = 0; i < rec->sp_skill_count; i++)
        cJSON_AddItemToArray(sp_arr, cJSON_CreateNumber(rec->sp_skills[i]));
    cJSON_AddItemToObject(rec_json, "SpSkillIDs", sp_arr);
}

void data_sync_to_json(EditorState *ed)
{
    cJSON *gp_chars, *cem;
    int i;

    if (!ed->save_root) return;

    /* Sync records back to gplay */
    gp_chars = cJSON_GetObjectItem(ed->gplay, "GDCharRecordInfo");
    if (gp_chars) {
        for (i = 0; i < ed->record_count; i++) {
            CharRecord *rec = &ed->records[i];
            char pid_str[32];
            cJSON *rec_json;

            snprintf(pid_str, sizeof(pid_str), "%d", rec->pid);
            rec_json = cJSON_GetObjectItem(gp_chars, pid_str);
            if (!rec_json) {
                rec_json = cJSON_CreateObject();
                cJSON_AddItemToObject(gp_chars, pid_str, rec_json);
            }
            sync_record_to_json(ed, rec, rec_json);
        }
    }

    /* Sync entities back to stage charEntitiesMap */
    cem = NULL;
    if (ed->stage) cem = cJSON_GetObjectItem(ed->stage, "charEntitiesMap");
    if (cem) {
        for (i = 0; i < ed->entity_count; i++) {
            CharEntity *ent = &ed->entities[i];
            cJSON *ent_json;

            ent_json = cJSON_GetObjectItem(cem, ent->key);
            if (ent_json && cJSON_IsString(ent_json)) {
                /* Replace string with object for easier editing */
                cJSON *new_obj = cJSON_Parse(ent_json->valuestring);
                if (new_obj) {
                    cJSON_ReplaceItemInObject(cem, ent->key, new_obj);
                    ent_json = new_obj;
                }
            }
            if (!ent_json || !cJSON_IsObject(ent_json)) continue;

            cJSON_SetInt(ent_json, "PlayerId", ent->pid);
            cJSON_SetStr(ent_json, "Name", ent->name);
            cJSON_SetInt(ent_json, "Level", ent->level);
            cJSON_SetInt(ent_json, "Exp", ent->exp);
            cJSON_SetInt(ent_json, "Hp", ent->hp);
            cJSON_SetInt(ent_json, "MaxHp", ent->max_hp);
            cJSON_SetInt(ent_json, "Mp", ent->mp);
            cJSON_SetInt(ent_json, "MaxMp", ent->max_mp);

            /* Equipment, bag, skills — sync to entity JSON too */
            {
                cJSON *equip_obj = cJSON_GetObjectItem(ent_json, "EquipIDs");
                int j;
                if (!equip_obj) { equip_obj = cJSON_CreateObject(); cJSON_AddItemToObject(ent_json, "EquipIDs", equip_obj); }
                for (j = 0; j < NUM_EQUIP_SLOTS; j++) {
                    char key[8];
                    snprintf(key, sizeof(key), "%d", j);
                    if (ent->equip[j] >= 0) cJSON_SetInt(equip_obj, key, ent->equip[j]);
                    else cJSON_DeleteItemFromObject(equip_obj, key);
                }
            }
            {
                cJSON *bag_arr = cJSON_GetObjectItem(ent_json, "ItemIDs");
                int j;
                if (bag_arr) cJSON_DeleteItemFromObject(ent_json, "ItemIDs");
                bag_arr = cJSON_CreateArray();
                for (j = 0; j < ent->bag_count; j++)
                    cJSON_AddItemToArray(bag_arr, cJSON_CreateNumber(ent->bag[j]));
                cJSON_AddItemToObject(ent_json, "ItemIDs", bag_arr);
            }
            cJSON_SetInt(ent_json, "NrlSkillID", ent->nrl_skill);
            {
                cJSON *ma = cJSON_GetObjectItem(ent_json, "MagicSkillIDs");
                int j;
                if (ma) cJSON_DeleteItemFromObject(ent_json, "MagicSkillIDs");
                ma = cJSON_CreateArray();
                for (j = 0; j < ent->magic_skill_count; j++)
                    cJSON_AddItemToArray(ma, cJSON_CreateNumber(ent->magic_skills[j]));
                cJSON_AddItemToObject(ent_json, "MagicSkillIDs", ma);
            }
            {
                cJSON *sa = cJSON_GetObjectItem(ent_json, "SpSkillIDs");
                int j;
                if (sa) cJSON_DeleteItemFromObject(ent_json, "SpSkillIDs");
                sa = cJSON_CreateArray();
                for (j = 0; j < ent->sp_skill_count; j++)
                    cJSON_AddItemToArray(sa, cJSON_CreateNumber(ent->sp_skills[j]));
                cJSON_AddItemToObject(ent_json, "SpSkillIDs", sa);
            }
        }
    }

    /* Sync storage items back to gplay */
    if (ed->gplay) {
        cJSON *st = cJSON_GetObjectItem(ed->gplay, "StorageItems");
        if (st) cJSON_DeleteItemFromObject(ed->gplay, "StorageItems");
        st = cJSON_CreateObject();
        for (i = 0; i < ed->storage_count; i++) {
            char key[32];
            snprintf(key, sizeof(key), "%d", ed->storage_ids[i]);
            cJSON_AddNumberToObject(st, key, ed->storage_qty[i]);
        }
        cJSON_AddItemToObject(ed->gplay, "StorageItems", st);
    }
}

/* ============================================================
 * Default save directory
 * ============================================================ */
int data_get_default_save_dir(char *buf, int buf_size)
{
    const char *profile = getenv("USERPROFILE");
    if (!profile) return -1;

    /* Try formal version first */
    snprintf(buf, buf_size, "%s\\AppData\\LocalLow\\UserJoy\\HSLR\\Save\\sav", profile);
    if (GetFileAttributesA(buf) != INVALID_FILE_ATTRIBUTES) return 0;

    /* Try demo version */
    snprintf(buf, buf_size, "%s\\AppData\\LocalLow\\UserJoy\\HSLR\\Save\\Save_Demo\\sav", profile);
    if (GetFileAttributesA(buf) != INVALID_FILE_ATTRIBUTES) return 0;

    return -1;
}
