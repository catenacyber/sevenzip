#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "7z.h"
#include "7zCrc.h"
#include "7zAlloc.h"
#include "Alloc.h"

FILE * outfile = NULL;
static int initialized = 0;

extern "C" void fuzz_openFile(const char * name) {
    if (outfile != NULL) {
        fclose(outfile);
    }
    outfile = fopen(name, "w");
}

typedef struct
{
    ILookInStream vt;
    size_t pos;
    size_t size;
    const Byte *buf;
} CbufStream;

static SRes bufStream_Look(const ILookInStream *pp, const void **buf, size_t *size)
{
    CbufStream *p = (CbufStream *) pp;
    SRes res = SZ_OK;

    size_t rem = p->size - p->pos;
    if (rem == 0 && *size != 0)
        return SZ_ERROR_INPUT_EOF;

    if (*size > rem)
        *size = rem;
    *buf = p->buf + p->pos;
    return res;
}

static SRes bufStream_Skip(const ILookInStream *pp, size_t offset)
{
    CbufStream *p = (CbufStream *) pp;
    p->pos += offset;
    return SZ_OK;
}

static SRes bufStream_Read(const ILookInStream *pp, void *buf, size_t *size)
{
    CbufStream *p = (CbufStream *) pp;
    size_t rem = p->size - p->pos;
    if (rem == 0)
        return SZ_ERROR_INPUT_EOF;
    if (rem > *size)
        rem = *size;
    memcpy(buf, p->buf + p->pos, rem);
    p->pos += rem;
    *size = rem;
    return SZ_OK;
}

static SRes bufStream_Seek(const ILookInStream *pp, Int64 *pos, ESzSeek origin)
{
    CbufStream *p = (CbufStream *) pp;
    switch (origin) {
        case SZ_SEEK_SET:
        if (*pos < 0 || *pos > p->size) {
            return SZ_ERROR_READ;
        }
        p->pos = *pos;
        break;

        case SZ_SEEK_CUR:
        if (p->pos + *pos < 0 || p->pos + *pos > p->size) {
            return SZ_ERROR_READ;
        }
        p->pos += *pos;
        *pos = p->pos;
        break;

        case SZ_SEEK_END:
        if (p->size + *pos < 0 || p->size + *pos > p->size) {
            return SZ_ERROR_READ;
        }
        p->pos = p->size + *pos;
        *pos = p->pos;
        break;

        default:
            return SZ_ERROR_PARAM;
    }
    return SZ_OK;
}

static char *UIntToStr(char *s, unsigned value, int numDigits)
{
    char temp[16];
    int pos = 0;
    do
    temp[pos++] = (char)('0' + (value % 10));
    while (value /= 10);
    
    for (numDigits -= pos; numDigits > 0; numDigits--)
    *s++ = '0';
    
    do
    *s++ = temp[--pos];
    while (pos);
    *s = '\0';
    return s;
}

static void UIntToStr_2(char *s, unsigned value)
{
    s[0] = (char)('0' + (value / 10));
    s[1] = (char)('0' + (value % 10));
}

#define PERIOD_4 (4 * 365 + 1)
#define PERIOD_100 (PERIOD_4 * 25 - 1)
#define PERIOD_400 (PERIOD_100 * 4 + 1)
static void ConvertFileTimeToString(const CNtfsFileTime *nt, char *s)
{
    unsigned year, mon, hour, min, sec;
    Byte ms[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    unsigned t;
    UInt32 v;
    UInt64 v64 = nt->Low | ((UInt64)nt->High << 32);
    v64 /= 10000000;
    sec = (unsigned)(v64 % 60); v64 /= 60;
    min = (unsigned)(v64 % 60); v64 /= 60;
    hour = (unsigned)(v64 % 24); v64 /= 24;
    
    v = (UInt32)v64;
    
    year = (unsigned)(1601 + v / PERIOD_400 * 400);
    v %= PERIOD_400;
    
    t = v / PERIOD_100; if (t ==  4) t =  3; year += t * 100; v -= t * PERIOD_100;
    t = v / PERIOD_4;   if (t == 25) t = 24; year += t * 4;   v -= t * PERIOD_4;
    t = v / 365;        if (t ==  4) t =  3; year += t;       v -= t * 365;
    
    if (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0))
    ms[1] = 29;
    for (mon = 0;; mon++)
    {
        unsigned d = ms[mon];
        if (v < d)
        break;
        v -= d;
    }
    s = UIntToStr(s, year, 4); *s++ = '-';
    UIntToStr_2(s, mon + 1); s[2] = '-'; s += 3;
    UIntToStr_2(s, (unsigned)v + 1); s[2] = ' '; s += 3;
    UIntToStr_2(s, hour); s[2] = ':'; s += 3;
    UIntToStr_2(s, min); s[2] = ':'; s += 3;
    UIntToStr_2(s, sec); s[2] = 0;
}

static void PrintUTFString(const UInt16 *s) {
    const uint16_t * utfChar = s;
    while (*utfChar) {
        fprintf(outfile, "%c", (uint8_t) *utfChar);
        utfChar++;
    }
}

CbufStream bufStream;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    CSzArEx db;
    SRes res;
    uint32_t i;
    uint16_t *temp = NULL;
    size_t tempSize = 0;

    //initialization
    if (initialized == 0) {
        if (outfile == NULL) {
            fuzz_openFile("/dev/null");
        }
        CrcGenerateTable();
        bufStream.vt.Look = bufStream_Look;
        bufStream.vt.Skip = bufStream_Skip;
        bufStream.vt.Read = bufStream_Read;
        bufStream.vt.Seek = bufStream_Seek;
        initialized = 1;
    }

    SzArEx_Init(&db);

    bufStream.buf = Data;
    bufStream.size = Size;
    bufStream.pos = 0;
    res = SzArEx_Open(&db, &bufStream.vt, &g_Alloc, &g_Alloc);

    if (res != SZ_OK) {
        return 0;
    }

    for (i = 0; i < db.NumFiles; i++)
    {
        // const CSzFileItem *f = db.Files + i;
        size_t len;
        unsigned isDir = SzArEx_IsDir(&db, i);
        len = SzArEx_GetFileNameUtf16(&db, i, NULL);
        if (len > tempSize)
        {
            SzFree(NULL, temp);
            tempSize = len;
            temp = (uint16_t *)SzAlloc(NULL, tempSize * sizeof(temp[0]));
            if (!temp) {
                break;
            }
        }
        SzArEx_GetFileNameUtf16(&db, i, temp);

        char t[32];
        UInt64 fileSize;
        UInt32 attrib;

        attrib = SzBitWithVals_Check(&db.Attribs, i) ? db.Attribs.Vals[i] : 0;
        fileSize = SzArEx_GetFileSize(&db, i);
        if (SzBitWithVals_Check(&db.MTime, i)) {
            ConvertFileTimeToString(&db.MTime.Vals[i], t);
            fprintf(outfile, "%s ", t);
        }

        fprintf(outfile, "0x%x ", attrib);
        fprintf(outfile, "0x%llx ", fileSize);
        PrintUTFString(temp);
        fprintf(outfile, "\n");
    }
    SzFree(NULL, temp);
    SzArEx_Free(&db, &g_Alloc);

    return 0;
}
