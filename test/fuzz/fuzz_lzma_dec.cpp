#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "LzmaLib.h"

FILE * outfile = NULL;

extern "C" void fuzz_openFile(const char * name) {
    if (outfile != NULL) {
        fclose(outfile);
    }
    outfile = fopen(name, "w");
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    size_t srcLen;
    size_t dstLen;
    uint8_t *outBuf;
    int r;

    //initialization
    if (outfile == NULL) {
        fuzz_openFile("/dev/null");
    }

    if (Size < LZMA_PROPS_SIZE + 8) {
        return 0;
    }
    srcLen = Size - (LZMA_PROPS_SIZE + 8);
    //Limits expectation up to 16 Mo of uncompressed data
    dstLen = (Data[LZMA_PROPS_SIZE+5] << 16) | (Data[LZMA_PROPS_SIZE+6] << 8) | (Data[LZMA_PROPS_SIZE+7]);
    outBuf = (uint8_t *) malloc(dstLen);
    if (outBuf == NULL) {
        return 0;
    }

    r = LzmaUncompress(outBuf, &dstLen, Data + LZMA_PROPS_SIZE + 8, &srcLen, Data, LZMA_PROPS_SIZE);
    fprintf(outfile, "(status: %d) decompressed %zu bytes \n", r, dstLen);

    free(outBuf);

    return 0;
}
