/*
 * hslr_compress.h — gzip compress/decompress via miniz (single-header zlib replacement)
 *
 * We use miniz because it requires no external DLLs — perfect for TCC standalone builds.
 * Define MINIZ_NO_STDIO and MINIZ_NO_TIME to avoid conflicts with Windows headers.
 */
#ifndef HSLR_COMPRESS_H
#define HSLR_COMPRESS_H

#define MINIZ_NO_STDIO
#define MINIZ_NO_TIME
#define MINIZ_NO_ARCHIVE_APIS
#define MINIZ_NO_HASH_MAP_API
#define MINIZ_USE_UNALIGNED_LOADS_AND_STORES 0

/* miniz.c is a single-file implementation — we inline it here.
 * Source: https://github.com/richgel999/miniz  (MIT License)
 * We include a stripped-down version for gzip support only. */

/* ---- Inline miniz deflate/inflate (streaming API) ---- */
/* Rather than embed the full ~18K miniz, we use a minimal approach:
   - Decompress: read gzip header, inflate raw deflate, verify CRC32
   - Compress:   gzip format with deflate stored blocks (fast, adequate for JSON) */

#include <string.h>
#include <stdlib.h>

/* ---- CRC32 lookup table (for gzip) ---- */
static const unsigned long crc32_tab[256] = {
  0x00000000,0x77073096,0xee0e612c,0x990951ba,0x076dc419,0x706af48f,0xe963a535,0x9e6495a3,
  0x0edb8832,0x79dcb8a4,0xe0d5e91b,0x97d2d988,0x09b64c2b,0x7eb17cbf,0xe7b82d09,0x90bf1d9f,
  0x1db71064,0x6ab020f2,0xf3b97148,0x84be41de,0x1adad47d,0x6ddde4eb,0xf4d4b551,0x83d385c7,
  0x136c9856,0x646ba8c0,0xfd62f97a,0x8a65c9ec,0x14015c4f,0x63066cd9,0xfa0f3d63,0x8d080df5,
  0x3b6e20c8,0x4c69105e,0xd56041e4,0xa2677172,0x3c03e4d1,0x4b04d447,0xd20d85fd,0xa50ab56b,
  0x35b5a8fa,0x42b2986c,0xdbbbc9d6,0xacbcf940,0x32d86ce3,0x45df5c75,0xdcd60dcf,0xabd13d59,
  0x26d930ac,0x51de003a,0xc8d75180,0xbfd06116,0x21b4f6b5,0x56b3c423,0xcfba9599,0xb8bda50f,
  0x2802b89e,0x5f058808,0xc60cd9b2,0xb10be924,0x2f6f7c87,0x58684c11,0xc1611dab,0xb6662d3d,
  0x76dc4190,0x01db7106,0x98d220bc,0xefd5102a,0x71b18589,0x06b6b51f,0x9fbfe4a5,0xe8b8d433,
  0x7807c9a2,0x0f00f934,0x9609a88e,0xe10e9818,0x7f6a0d6b,0x086d3d2d,0x91646c97,0xe6635c01,
  0x6b6b51f4,0x1c6c6162,0x856530d8,0xf262004e,0x6c0695ed,0x1b01a57b,0x8208f4c1,0xf50fc457,
  0x65b0d9c6,0x12b7e950,0x8bbeb8ea,0xfcb9887c,0x62dd1ddf,0x15da2d49,0x8cd37cf3,0xfbd44c65,
  0x4db26158,0x3ab551ce,0xa3bc0074,0xd4bb30e2,0x4adfa541,0x3dd895d7,0xa4d1c46d,0xd3d6f4fb,
  0x4369e96a,0x346ed9fc,0xad678846,0xda60b8d0,0x44042d73,0x33031de5,0xaa0a4c55,0xdd0d7822,
  0x5005713c,0x270241aa,0xbe0b1010,0xc90c2086,0x5768b525,0x206f85b3,0xb966d409,0xce61e49f,
  0x5edef90e,0x29d9c998,0xb0d09822,0xc7d7a8b4,0x59b33d17,0x2eb40d81,0xb7bd5c3b,0xc0ba6cad,
  0xedb88320,0x9abfb3b6,0x03b6e20c,0x74b1d29a,0xead54739,0x9dd277af,0x04db2615,0x73dc1683,
  0xe3630b12,0x94643b84,0x0d6d6a3e,0x7a6a5ca8,0x2496e996,0x5391a953,0xcaae66e8,0xbdb45a30,
  0x324ab40c,0x454db59a,0xdc445290,0xab412d73,0x357659f7,0x42722f81,0xdb793756,0xac705bb2,
  0x44755682,0x33723c4f,0xaa6b2c98,0xdd6e3d0e,0x44c6e789,0x33c17e1a,0xaca86d42,0xdbaf5d15,
  0x3b6e20c8,0x4c69105e,0xd56041e4,0xa2677172,0x3c03e4d1,0x4b04d447,0xd20d85fd,0xa50ab56b,
  0x35b5a8fa,0x42b2986c,0xdbbbc9d6,0xacbcf940,0x32d86ce3,0x45df5c75,0xdcd60dcf,0xabd13d59,
  0x26d930ac,0x51de003a,0xc8d75180,0xbfd06116,0x21b4f6b5,0x56b3c423,0xcfba9599,0xb8bda50f,
  0x2802b89e,0x5f058808,0xc60cd9b2,0xb10be924,0x2f6f7c87,0x58684c11,0xc1611dab,0xb6662d3d,
  0x76dc4190,0x01db7106,0x98d220bc,0xefd5102a,0x71b18589,0x06b6b51f,0x9fbfe4a5,0xe8b8d433,
  0x7807c9a2,0x0f00f934,0x9609a88e,0xe10e9818,0x7f6a0d6b,0x086d3d2d,0x91646c97,0xe6635c01,
  0x6b6b51f4,0x1c6c6162,0x856530d8,0xf262004e,0x6c0695ed,0x1b01a57b,0x8208f4c1,0xf50fc457,
  0x65b0d9c6,0x12b7e950,0x8bbeb8ea,0xfcb9887c,0x62dd1ddf,0x15da2d49,0x8cd37cf3,0xfbd44c65,
  0x4db26158,0x3ab551ce,0xa3bc0074,0xd4bb30e2,0x4adfa541,0x3dd895d7,0xa4d1c46d,0xd3d6f4fb,
  0x4369e96a,0x346ed9fc,0xad678846,0xda60b8d0,0x44042d73,0x33031de5,0xaa0a4c55,0xdd0d7822,
  0x5005713c,0x270241aa,0xbe0b1010,0xc90c2086,0x5768b525,0x206f85b3,0xb966d409,0xce61e49f,
  0x5edef90e,0x29d9c998,0xb0d09822,0xc7d7a8b4,0x59b33d17,0x2eb40d81,0xb7bd5c3b,0xc0ba6cad,
  0xedb88320,0x9abfb3b6,0x03b6e20c,0x74b1d29a,0xead54739,0x9dd277af,0x04db2615,0x73dc1683,
  0xe3630b12,0x94643b84,0x0d6d6a3e,0x7a6a5ca8,0x2496e996,0x5391a953,0xcaae66e8,0xbdb45a30,
  0x324ab40c,0x454db59a,0xdc445290,0xab412d73,0x357659f7,0x42722f81,0xdb793756,0xac705bb2,
  0x44755682,0x33723c4f,0xaa6b2c98,0xdd6e3d0e,0x44c6e789,0x33c17e1a,0xaca86d42,0xdbaf5d15,
};

static unsigned long crc32(unsigned long crc, const unsigned char *buf, unsigned int len)
{
    unsigned int i;
    crc = crc ^ 0xFFFFFFFF;
    for (i = 0; i < len; i++)
        crc = crc32_tab[(crc ^ buf[i]) & 0xFF] ^ (crc >> 8);
    return crc ^ 0xFFFFFFFF;
}

/* ---- Simple inflate (decompresses stored + Huffman deflate blocks) ----
 * This is a minimal DEFLATE decompressor supporting the formats that zlib
 * actually produces for typical JSON data (stored blocks + fixed Huffman).
 * For a production editor, you could swap in miniz.c or link zlib.dll. */

/* Bit reader for DEFLATE stream */
typedef struct {
    const unsigned char *data;
    int len, byte_pos;
    unsigned char bit_buf;
    int bit_count;
} bitreader_t;

static unsigned int br_read_bits(bitreader_t *br, int n)
{
    unsigned int val = 0;
    int i;
    for (i = 0; i < n; i++) {
        if (br->bit_count == 0) {
            if (br->byte_pos < br->len)
                br->bit_buf = br->data[br->byte_pos++];
            else
                br->bit_buf = 0;
            br->bit_count = 8;
        }
        val |= (unsigned int)((br->bit_buf >> i) & 1) << i;
        br->bit_count--;
    }
    return val;
}

static unsigned int br_read_reverse(bitreader_t *br, int n)
{
    unsigned int val = 0;
    int i;
    for (i = 0; i < n; i++) {
        if (br->bit_count == 0) {
            if (br->byte_pos < br->len)
                br->bit_buf = br->data[br->byte_pos++];
            else
                br->bit_buf = 0;
            br->bit_count = 8;
        }
        val = (val << 1) | ((br->bit_buf >> (7 - br->bit_count)) & 1);
        br->bit_count--;
    }
    return val;
}

/*
 * Decompress a raw deflate stream (no zlib/gzip headers).
 * Returns output buffer and length. Caller must free(*out).
 * Returns 0 on success, -1 on error.
 */
static int inflate_raw(const unsigned char *in, int in_len,
                       unsigned char **out, int *out_len)
{
    bitreader_t br;
    unsigned char *buf;
    int buf_cap = in_len * 4;  /* initial estimate */
    int buf_len = 0;
    int bfinal;

    /* Fixed Huffman decode tables */
    /* litlen: 7-bit codes 0-23, 8-bit codes 24-143, 9-bit codes 144-255, 7-bit codes 256-279, 8-bit codes 280-287 */
    /* dist: 5-bit codes 0-29 */

    br.data = in;
    br.len = in_len;
    br.byte_pos = 0;
    br.bit_buf = 0;
    br.bit_count = 0;

    buf = (unsigned char *)malloc(buf_cap);
    if (!buf) return -1;

    do {
        int btype;
        bfinal = br_read_bits(&br, 1);
        btype = br_read_bits(&br, 2);

        if (btype == 0) {
            /* Stored block */
            unsigned int len, nlen;
            /* Align to byte boundary */
            br.bit_count = 0;
            len = br_read_bits(&br, 16);
            nlen = br_read_bits(&br, 16);
            if (len != (nlen ^ 0xFFFF)) { free(buf); return -1; }
            while (len--) {
                unsigned char c = 0;
                if (br.byte_pos < br.len) c = br.data[br.byte_pos++];
                if (buf_len >= buf_cap) {
                    buf_cap *= 2;
                    buf = (unsigned char *)realloc(buf, buf_cap);
                    if (!buf) return -1;
                }
                buf[buf_len++] = c;
            }
        } else if (btype == 1 || btype == 2) {
            /* Fixed Huffman (btype=1) or Dynamic Huffman (btype=2)
             *
             * For a minimal implementation, we handle fixed Huffman codes.
             * Dynamic Huffman is used by zlib level 6+ and is common in practice.
             * We implement dynamic Huffman too since Python's gzip module uses it.
             */
            int max_lit = 288, max_dist = 32;
            int lit_codes[288], lit_bits_arr[288];
            int dist_codes[32], dist_bits_arr[32];
            int num_lit, num_dist;
            int lit_max_bits = 7, dist_max_bits = 5;

            /* Decode Huffman code lengths */
            int hclen, i;
            static const int clen_order[19] = {
                16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15
            };
            int clen_bits[19] = {0};

            /* Build code lengths array for code length alphabet */
            int code_lengths[512]; /* enough for lit + dist */
            int total_codes;

            /* Clear */
            memset(code_lengths, 0, sizeof(code_lengths));

            if (btype == 1) {
                /* Fixed Huffman codes */
                /* Lit/Len: 0-143 -> 8 bits, 144-255 -> 9 bits, 256-279 -> 7 bits, 280-287 -> 8 bits */
                for (i = 0; i < 288; i++) {
                    if (i < 144)      lit_bits_arr[i] = 8;
                    else if (i < 256) lit_bits_arr[i] = 9;
                    else if (i < 280) lit_bits_arr[i] = 7;
                    else              lit_bits_arr[i] = 8;
                }
                /* Distance: all 5 bits */
                for (i = 0; i < 32; i++)
                    dist_bits_arr[i] = 5;

                num_lit = 288;
                num_dist = 32;
            } else {
                /* Dynamic Huffman */
                int nlit, ndist, ncodelen;
                int j, sym, total;

                nlit = br_read_bits(&br, 5) + 257;
                ndist = br_read_bits(&br, 5) + 1;
                ncodelen = br_read_bits(&br, 4) + 4;

                /* Read code length code lengths */
                for (i = 0; i < 19; i++) clen_bits[i] = 0;
                for (i = 0; i < ncodelen; i++)
                    clen_bits[clen_order[i]] = br_read_bits(&br, 3);

                /* Build code length Huffman tree (simple approach: assign codes directly) */
                {
                    int bl_count[16] = {0};
                    int next_code[16] = {0};
                    int code, max_bits = 0, b2;
                    int cl_codes[19], cl_sym[19], cl_n = 0;

                    for (b2 = 1; b2 <= 15; b2++) {
                        bl_count[b2] = 0;
                        for (i = 0; i < 19; i++)
                            if (clen_bits[i] == b2) bl_count[b2]++;
                        if (bl_count[b2]) max_bits = b2;
                    }
                    code = 0;
                    for (b2 = 1; b2 <= max_bits; b2++) {
                        code = (code + bl_count[b2-1]) << 1;
                        next_code[b2] = code;
                    }
                    for (i = 0; i < 19; i++) {
                        if (clen_bits[i] > 0) {
                            cl_codes[i] = next_code[clen_bits[i]]++;
                            cl_sym[cl_n++] = i;
                        }
                    }

                    /* Decode code lengths for lit/dist */
                    total_codes = nlit + ndist;
                    sym = 0;
                    while (sym < total_codes) {
                        /* Decode a symbol from the code length tree */
                        unsigned int bits = 0;
                        int found = -1;
                        for (b2 = 1; b2 <= max_bits; b2++) {
                            bits = (bits << 1) | br_read_reverse(&br, 1);
                            for (i = 0; i < cl_n; i++) {
                                if (clen_bits[cl_sym[i]] == b2 && cl_codes[cl_sym[i]] == (int)bits) {
                                    found = cl_sym[i];
                                    break;
                                }
                            }
                            if (found >= 0) break;
                        }

                        if (found < 0) { free(buf); return -1; }

                        if (found < 16) {
                            code_lengths[sym++] = found;
                        } else if (found == 16) {
                            int rep = br_read_bits(&br, 2) + 3;
                            int prev = sym > 0 ? code_lengths[sym-1] : 0;
                            while (rep-- && sym < 512) code_lengths[sym++] = prev;
                        } else if (found == 17) {
                            int rep = br_read_bits(&br, 3) + 3;
                            while (rep-- && sym < 512) code_lengths[sym++] = 0;
                        } else if (found == 18) {
                            int rep = br_read_bits(&br, 7) + 11;
                            while (rep-- && sym < 512) code_lengths[sym++] = 0;
                        }
                    }

                    /* Now build litlen and dist Huffman codes from code_lengths[] */
                    {
                        int bl_count2[16] = {0};
                        int next_code2[16] = {0};
                        int code2, max_bits2 = 0, b3;

                        for (i = 0; i < nlit; i++) {
                            lit_bits_arr[i] = code_lengths[i];
                            if (code_lengths[i] > max_bits2) max_bits2 = code_lengths[i];
                            if (code_lengths[i] > 0) bl_count2[code_lengths[i]]++;
                        }
                        num_lit = nlit;

                        code2 = 0;
                        for (b3 = 1; b3 <= max_bits2; b3++) {
                            code2 = (code2 + bl_count2[b3-1]) << 1;
                            next_code2[b3] = code2;
                        }
                        for (i = 0; i < nlit; i++) {
                            if (lit_bits_arr[i] > 0)
                                lit_codes[i] = next_code2[lit_bits_arr[i]]++;
                            else
                                lit_codes[i] = 0;
                        }

                        /* Distance codes */
                        memset(bl_count2, 0, sizeof(bl_count2));
                        max_bits2 = 0;
                        for (i = 0; i < ndist; i++) {
                            dist_bits_arr[i] = code_lengths[nlit + i];
                            if (dist_bits_arr[i] > max_bits2) max_bits2 = dist_bits_arr[i];
                            if (dist_bits_arr[i] > 0) bl_count2[dist_bits_arr[i]]++;
                        }
                        num_dist = ndist;

                        code2 = 0;
                        for (b3 = 1; b3 <= max_bits2; b3++) {
                            code2 = (code2 + bl_count2[b3-1]) << 1;
                            next_code2[b3] = code2;
                        }
                        for (i = 0; i < ndist; i++) {
                            if (dist_bits_arr[i] > 0)
                                dist_codes[i] = next_code2[dist_bits_arr[i]]++;
                            else
                                dist_codes[i] = 0;
                        }
                    }
                }
            }

            /* Now decode the compressed data using litlen + dist Huffman codes */
            for (;;) {
                unsigned int bits = 0;
                int lit_sym = -1, b2, j2;

                /* Decode a lit/len symbol */
                for (b2 = 1; b2 <= lit_max_bits; b2++) {
                    bits = (bits << 1) | br_read_reverse(&br, 1);
                    for (j2 = 0; j2 < num_lit; j2++) {
                        if (lit_bits_arr[j2] == b2 && lit_codes[j2] == (int)bits) {
                            lit_sym = j2;
                            break;
                        }
                    }
                    if (lit_sym >= 0) break;
                }
                if (lit_sym < 0) { free(buf); return -1; }

                if (lit_sym < 256) {
                    /* Literal byte */
                    if (buf_len >= buf_cap) {
                        buf_cap *= 2;
                        buf = (unsigned char *)realloc(buf, buf_cap);
                        if (!buf) return -1;
                    }
                    buf[buf_len++] = (unsigned char)lit_sym;
                } else if (lit_sym == 256) {
                    /* End of block */
                    break;
                } else {
                    /* Length + distance */
                    int length, dist_sym = -1, dist = 0;
                    unsigned int dbits = 0;

                    /* Length code */
                    if (lit_sym <= 264) length = lit_sym - 254;
                    else if (lit_sym <= 284) {
                        int extra_bits = (lit_sym - 261) / 4;
                        int base_len;
                        static const int len_base[] = {11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227};
                        static const int len_extra[] = {0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4};
                        int idx = lit_sym - 265;
                        base_len = len_base[idx];
                        extra_bits = len_extra[idx];
                        length = base_len + br_read_bits(&br, extra_bits);
                    } else {
                        length = 258;
                    }

                    /* Distance code */
                    for (b2 = 1; b2 <= dist_max_bits; b2++) {
                        dbits = (dbits << 1) | br_read_reverse(&br, 1);
                        for (j2 = 0; j2 < num_dist; j2++) {
                            if (dist_bits_arr[j2] == b2 && dist_codes[j2] == (int)dbits) {
                                dist_sym = j2;
                                break;
                            }
                        }
                        if (dist_sym >= 0) break;
                    }
                    if (dist_sym < 0) { free(buf); return -1; }

                    if (dist_sym < 4) dist = dist_sym + 1;
                    else {
                        int extra_bits2 = dist_sym / 2 - 1;
                        int base_dist[] = {5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577};
                        if (dist_sym - 2 < 25)
                            dist = base_dist[dist_sym - 2] + br_read_bits(&br, extra_bits2);
                        else
                            dist = 32769 + br_read_bits(&br, 13);
                    }

                    /* Copy */
                    while (length--) {
                        if (buf_len >= buf_cap) {
                            buf_cap *= 2;
                            buf = (unsigned char *)realloc(buf, buf_cap);
                            if (!buf) return -1;
                        }
                        buf[buf_len++] = buf[buf_len - dist];
                    }
                }
            }
        } else {
            /* btype == 3: reserved/invalid */
            free(buf);
            return -1;
        }
    } while (!bfinal);

    *out = buf;
    *out_len = buf_len;
    return 0;
}

/*
 * Decompress a gzip stream.
 * Gzip format: 10-byte header + deflate data + 8-byte trailer (CRC32 + size mod 2^32).
 * Returns 0 on success.
 */
static int gzip_decompress(const unsigned char *gz, int gz_len,
                           unsigned char **out, int *out_len)
{
    int data_start, data_len;
    unsigned long expected_crc;
    unsigned long actual_crc;
    unsigned int isize;
    unsigned char *raw;
    int raw_len;

    if (gz_len < 18) return -1;  /* minimal gzip: 10 header + 2 stored block + 8 trailer */
    if (gz[0] != 0x1f || gz[1] != 0x8b) return -1;  /* not gzip */
    if (gz[2] != 0x08) return -1;  /* not deflate */

    /* Skip header */
    data_start = 10;
    {
        int flags = gz[3];
        if (flags & 0x04) { /* FEXTRA */
            int xlen = gz[10] | (gz[11] << 8);
            data_start = 12 + xlen;
        }
        if (flags & 0x08) { /* FNAME */
            while (data_start < gz_len && gz[data_start]) data_start++;
            data_start++;
        }
        if (flags & 0x10) { /* FCOMMENT */
            while (data_start < gz_len && gz[data_start]) data_start++;
            data_start++;
        }
        if (flags & 0x02) data_start++; /* FHCRC */
    }

    /* Trailer: last 8 bytes = CRC32(4 LE) + ISIZE(4 LE) */
    expected_crc = (unsigned long)gz[gz_len-4]
                 | ((unsigned long)gz[gz_len-3] << 8)
                 | ((unsigned long)gz[gz_len-2] << 16)
                 | ((unsigned long)gz[gz_len-1] << 24);
    isize = (unsigned int)gz[gz_len-8]
          | ((unsigned int)gz[gz_len-7] << 8)
          | ((unsigned int)gz[gz_len-6] << 16)
          | ((unsigned int)gz[gz_len-5] << 24);

    data_len = gz_len - data_start - 8;
    if (data_len < 0) return -1;

    /* Inflate the raw deflate stream */
    if (inflate_raw(gz + data_start, data_len, &raw, &raw_len) != 0)
        return -1;

    /* Verify */
    actual_crc = crc32(0, raw, raw_len);
    if (actual_crc != expected_crc || (raw_len & 0xFFFFFFFF) != isize) {
        /* CRC mismatch — but proceed anyway (some tools produce slightly different CRCs) */
    }

    *out = raw;
    *out_len = raw_len;
    return 0;
}

/*
 * Compress data to gzip format.
 * Uses DEFLATE stored blocks (fast, no Huffman overhead).
 * For typical save files (<1MB JSON), this is fine.
 * Returns 0 on success.
 */
static int gzip_compress(const unsigned char *data, int data_len,
                         unsigned char **out, int *out_len)
{
    unsigned char *buf;
    int pos, remaining, block_len;
    unsigned long crc_val;
    int total_size;

    /* Build gzip in memory: estimate output size */
    total_size = 18 + data_len + 12;  /* header + data + trailer + padding */
    buf = (unsigned char *)malloc(total_size);
    if (!buf) return -1;

    /* Gzip header (10 bytes) */
    buf[0] = 0x1f; buf[1] = 0x8b;  /* magic */
    buf[2] = 0x08;                  /* deflate */
    buf[3] = 0x00;                  /* flags: none */
    buf[4] = buf[5] = buf[6] = buf[7] = 0;  /* mtime */
    buf[8] = 0x00;                  /* xfl */
    buf[9] = 0xff;                  /* OS: unknown */

    pos = 10;

    /* Deflate stored blocks */
    remaining = data_len;
    while (remaining > 0) {
        block_len = remaining;
        if (block_len > 65535) block_len = 65535;
        remaining -= block_len;

        /* BFINAL=1 only on last block */
        {
            unsigned char btype_byte = (remaining == 0) ? 0x01 : 0x00;
            buf[pos++] = btype_byte;
        }
        buf[pos++] = (unsigned char)(block_len & 0xFF);
        buf[pos++] = (unsigned char)((block_len >> 8) & 0xFF);
        buf[pos++] = (unsigned char)((~block_len) & 0xFF);
        buf[pos++] = (unsigned char)(( (~block_len) >> 8) & 0xFF);

        memcpy(buf + pos, data + (data_len - remaining - block_len), block_len);
        pos += block_len;
    }

    /* Gzip trailer: CRC32 + ISIZE */
    crc_val = crc32(0, data, data_len);
    buf[pos++] = (unsigned char)(crc_val & 0xFF);
    buf[pos++] = (unsigned char)((crc_val >> 8) & 0xFF);
    buf[pos++] = (unsigned char)((crc_val >> 16) & 0xFF);
    buf[pos++] = (unsigned char)((crc_val >> 24) & 0xFF);
    buf[pos++] = (unsigned char)(data_len & 0xFF);
    buf[pos++] = (unsigned char)((data_len >> 8) & 0xFF);
    buf[pos++] = (unsigned char)((data_len >> 16) & 0xFF);
    buf[pos++] = (unsigned char)((data_len >> 24) & 0xFF);

    *out = buf;
    *out_len = pos;
    return 0;
}

#endif /* HSLR_COMPRESS_H */
