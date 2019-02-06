#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "LzmaLib.h"

FILE * outfile = NULL;

void fuzz_openFile(const char * name) {
    if (outfile != NULL) {
        fclose(outfile);
    }
    outfile = fopen(name, "w");
}

int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    size_t compLen;
    size_t uncompLen;
    uint8_t *compBuf;
    uint8_t *uncompBuf;
    size_t inSize;
    size_t propsSize = LZMA_PROPS_SIZE;
    int level;
    int lc;
    int lp;
    int pb;
    int fb;
    int nbThreads;
    int r;

    //initialization
    if (outfile == NULL) {
        fuzz_openFile("/dev/null");
    }

    if (Size < 3) {
        return 0;
    }
    inSize = Size - 3;
    compLen = inSize + inSize / 3 + 128;
    compBuf = malloc(compLen);
    if (compBuf == NULL) {
        return 0;
    }

    level = Data[0] & 0xF;
    lc = Data[0] >> 4;
    lp = Data[1] & 7;
    pb = (Data[1] >> 3) & 7;
    nbThreads = 1 + ((Data[1] >> 6) & 1);
    fb = Data[2] + ((Data[1] & 0x80) << 1);

    r = LzmaCompress(compBuf+LZMA_PROPS_SIZE, &compLen,
                           Data+3, inSize,
                           compBuf, &propsSize,
                           level, 0, lc, lp, pb, fb, nbThreads);
    if (r != SZ_OK) {
        free(compBuf);
        if (r == SZ_ERROR_PARAM) {
            return 0;
        }
        printf("LzmaCompress failed 0x%x\n", r);
        abort();
    }
    uncompLen = inSize;
    uncompBuf = malloc(uncompLen);
    if (uncompBuf == NULL) {
        return 0;
    }

    r = LzmaUncompress(uncompBuf, &uncompLen, compBuf + LZMA_PROPS_SIZE, &compLen, compBuf, LZMA_PROPS_SIZE);
    if (r != SZ_OK) {
        free(uncompBuf);
        free(compBuf);
        printf("LzmaUncompress failed 0x%x\n", r);
        abort();
    }

    if (uncompLen != inSize) {
        abort();
    }
    if (memcmp(Data+3, uncompBuf, inSize) != 0) {
        abort();
    }

    free(uncompBuf);
    free(compBuf);

    return 0;
}
