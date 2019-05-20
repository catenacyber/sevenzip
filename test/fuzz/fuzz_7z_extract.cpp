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

    UInt32 blockIndex = 0xFFFFFFFF; /* it can have any value before first call (if outBuffer = 0) */
    Byte *outBuffer = 0; /* it must be 0 before first call for each new archive. */
    size_t outBufferSize = 0;  /* it can have any value before first call (if outBuffer = 0) */

    for (i = 0; i < db.NumFiles; i++)
    {
        size_t offset = 0;
        size_t outSizeProcessed = 0;
        // const CSzFileItem *f = db.Files + i;
        unsigned isDir = SzArEx_IsDir(&db, i);

        if (!isDir) {
            res = SzArEx_Extract(&db, &bufStream.vt, i,
                                 &blockIndex, &outBuffer, &outBufferSize,
                                 &offset, &outSizeProcessed,
                                 &g_Alloc, &g_Alloc);
            if (res != SZ_OK)
                break;
        }
    }
    ISzAlloc_Free(&g_Alloc, outBuffer);
    SzFree(NULL, temp);
    SzArEx_Free(&db, &g_Alloc);

    return 0;
}
