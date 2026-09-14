/*
 * hslr_save.c — Save file load/save (decrypt + gunzip + parse JSON)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hslr_data.h"
#include "hslr_aes.h"
#include "hslr_compress.h"

/* Read entire file into buffer. Caller must free(*out). Returns 0 on success. */
static int read_file_all(const char *path, unsigned char **out, int *out_len)
{
    FILE *f;
    long len;
    unsigned char *buf;

    f = fopen(path, "rb");
    if (!f) return -1;

    fseek(f, 0, SEEK_END);
    len = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (len <= 0) { fclose(f); return -1; }

    buf = (unsigned char *)malloc((size_t)len);
    if (!buf) { fclose(f); return -1; }

    if ((long)fread(buf, 1, (size_t)len, f) != len) {
        free(buf); fclose(f); return -1;
    }
    fclose(f);

    *out = buf;
    *out_len = (int)len;
    return 0;
}

/* Write buffer to file. Returns 0 on success. */
static int write_file_all(const char *path, const unsigned char *data, int len)
{
    FILE *f;
    f = fopen(path, "wb");
    if (!f) return -1;
    if ((int)fwrite(data, 1, (size_t)len, f) != len) {
        fclose(f); return -1;
    }
    fclose(f);
    return 0;
}

/* ============================================================
 * save_load: .sav → decrypt → gunzip → parse JSON
 * ============================================================ */
int save_load(EditorState *ed, const char *path)
{
    unsigned char *raw = NULL;
    int raw_len = 0;
    unsigned char *compressed = NULL;
    int compressed_len = 0;
    unsigned char *json_str = NULL;
    int json_len = 0;
    char *json_cstr = NULL;
    int ret = -1;

    /* 1) Read file */
    if (read_file_all(path, &raw, &raw_len) != 0) {
        strcpy(ed->status_msg, "无法读取文件");
        ed->status_is_error = 1;
        return -1;
    }

    /* 2) Verify magic */
    if (raw_len < HSLR_MAGIC_LEN || memcmp(raw, HSLR_MAGIC, HSLR_MAGIC_LEN) != 0) {
        strcpy(ed->status_msg, "不是HSLR存档文件 (缺少ECC:标记)");
        ed->status_is_error = 1;
        free(raw);
        return -1;
    }

    /* 3) AES-CBC decrypt */
    if (aes_cbc_decrypt(HSLR_KEY, 32, HSLR_IV,
                        raw + HSLR_MAGIC_LEN, raw_len - HSLR_MAGIC_LEN,
                        &compressed, &compressed_len) != 0) {
        strcpy(ed->status_msg, "AES解密失败");
        ed->status_is_error = 1;
        free(raw);
        return -1;
    }
    free(raw);

    /* 4) Gzip decompress */
    if (gzip_decompress(compressed, compressed_len, &json_str, &json_len) != 0) {
        strcpy(ed->status_msg, "Gzip解压失败");
        ed->status_is_error = 1;
        free(compressed);
        return -1;
    }
    free(compressed);

    /* 5) Parse JSON (ensure null-terminated) */
    json_cstr = (char *)malloc((size_t)json_len + 1);
    if (!json_cstr) { free(json_str); return -1; }
    memcpy(json_cstr, json_str, (size_t)json_len);
    json_cstr[json_len] = '\0';
    free(json_str);

    /* Free previous save data */
    if (ed->save_root) cJSON_Delete(ed->save_root);
    ed->save_root = NULL;
    ed->gplay = NULL;
    ed->stage = NULL;

    ed->save_root = cJSON_Parse(json_cstr);
    free(json_cstr);

    if (!ed->save_root) {
        strcpy(ed->status_msg, "JSON解析失败");
        ed->status_is_error = 1;
        return -1;
    }

    /* 6) Extract gplay and stage sub-objects */
    {
        cJSON *gp_raw = cJSON_GetObjectItem(ed->save_root, "gplay");
        cJSON *st_raw = cJSON_GetObjectItem(ed->save_root, "stage");

        /* gplay is stored as a JSON string (double-encoded) or as an object */
        if (gp_raw && cJSON_IsString(gp_raw)) {
            ed->gplay = cJSON_Parse(gp_raw->valuestring);
            /* Replace the string with the parsed object for easier manipulation */
            if (ed->gplay) {
                cJSON_ReplaceItemInObject(ed->save_root, "gplay",
                    cJSON_DetachItemFromObject(ed->save_root, "gplay"));
                cJSON_ReplaceItemInObject(ed->save_root, "gplay", ed->gplay);
            }
        } else if (gp_raw && cJSON_IsObject(gp_raw)) {
            ed->gplay = gp_raw;
        }

        /* stage can be null (non-battle save), a string, or an object */
        if (st_raw && cJSON_IsString(st_raw)) {
            if (strcmp(st_raw->valuestring, "null") == 0 ||
                strcmp(st_raw->valuestring, "") == 0) {
                ed->stage = NULL;
            } else {
                ed->stage = cJSON_Parse(st_raw->valuestring);
                if (ed->stage) {
                    cJSON_ReplaceItemInObject(ed->save_root, "stage",
                        cJSON_DetachItemFromObject(ed->save_root, "stage"));
                    cJSON_ReplaceItemInObject(ed->save_root, "stage", ed->stage);
                }
            }
        } else if (st_raw && cJSON_IsObject(st_raw)) {
            ed->stage = st_raw;
        } else {
            ed->stage = NULL;  /* null or missing */
        }
    }

    /* 7) Extract character data */
    data_extract_characters(ed);

    /* 8) Store file path */
    strncpy(ed->save_path, path, MAX_PATH - 1);
    ed->save_path[MAX_PATH - 1] = '\0';

    strcpy(ed->status_msg, "已加载存档");
    ed->status_is_error = 0;
    return 0;
}

/* ============================================================
 * save_write: sync JSON tree → gzip → AES encrypt → write .sav
 * ============================================================ */
int save_write(EditorState *ed, const char *path)
{
    unsigned char *json_bytes = NULL;
    int json_len = 0;
    unsigned char *compressed = NULL;
    int compressed_len = 0;
    unsigned char *encrypted = NULL;
    int encrypted_len = 0;
    unsigned char *final_buf = NULL;
    int final_len = 0;
    char *json_str = NULL;
    int ret = -1;

    if (!ed->save_root) {
        strcpy(ed->status_msg, "没有可保存的数据");
        ed->status_is_error = 1;
        return -1;
    }

    /* 1) Sync character data back to cJSON tree */
    data_sync_to_json(ed);

    /* 2) Serialize JSON (ensure_ascii=False for Chinese characters) */
    json_str = cJSON_PrintUnformatted(ed->save_root);
    if (!json_str) {
        strcpy(ed->status_msg, "JSON序列化失败");
        ed->status_is_error = 1;
        return -1;
    }

    json_len = (int)strlen(json_str);
    json_bytes = (unsigned char *)json_str;  /* take ownership */

    /* 3) Gzip compress */
    if (gzip_compress(json_bytes, json_len, &compressed, &compressed_len) != 0) {
        strcpy(ed->status_msg, "Gzip压缩失败");
        ed->status_is_error = 1;
        free(json_bytes);
        return -1;
    }
    free(json_bytes);

    /* 4) AES-CBC encrypt */
    if (aes_cbc_encrypt(HSLR_KEY, 32, HSLR_IV,
                        compressed, compressed_len,
                        &encrypted, &encrypted_len) != 0) {
        strcpy(ed->status_msg, "AES加密失败");
        ed->status_is_error = 1;
        free(compressed);
        return -1;
    }
    free(compressed);

    /* 5) Prepend MAGIC header */
    final_len = HSLR_MAGIC_LEN + encrypted_len;
    final_buf = (unsigned char *)malloc((size_t)final_len);
    if (!final_buf) { free(encrypted); return -1; }

    memcpy(final_buf, HSLR_MAGIC, HSLR_MAGIC_LEN);
    memcpy(final_buf + HSLR_MAGIC_LEN, encrypted, (size_t)encrypted_len);
    free(encrypted);

    /* 6) Write to file */
    if (write_file_all(path, final_buf, final_len) != 0) {
        strcpy(ed->status_msg, "写入文件失败");
        ed->status_is_error = 1;
        free(final_buf);
        return -1;
    }
    free(final_buf);

    /* Update path */
    strncpy(ed->save_path, path, MAX_PATH - 1);
    ed->save_path[MAX_PATH - 1] = '\0';

    strcpy(ed->status_msg, "已保存存档");
    ed->status_is_error = 0;
    return 0;
}
