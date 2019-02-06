#include <stdio.h>
#include <stdlib.h>

#include "LzmaLib.h"

FILE * outfile = NULL;

void fuzz_openFile(const char * name) {
    if (outfile != NULL) {
        fclose(outfile);
    }
    outfile = fopen(name, "w");
}

int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    size_t srcLen;
    size_t dstLen;
    uint8_t *outBuf;
    int r;

    //initialization
    if (outfile == NULL) {
        fuzz_openFile("/dev/null");
    }

    if (Size < LZMA_PROPS_SIZE + 3) {
        return 0;
    }
    srcLen = Size;
    //Limits expectation up to 16 Mo of uncompressed data
    dstLen = (Data[LZMA_PROPS_SIZE] << 16) | (Data[LZMA_PROPS_SIZE+1] << 8) | (Data[LZMA_PROPS_SIZE+2]);
    outBuf = malloc(dstLen);
    if (outBuf == NULL) {
        return 0;
    }

    //TODO LzmaCompress round trip
    r = LzmaUncompress(&outBuf[0], &dstLen, Data + LZMA_PROPS_SIZE + 3, &srcLen, Data, LZMA_PROPS_SIZE);
    fprintf(outfile, "(status: %d) decompressed %zu bytes \n", r, dstLen);

    free(outBuf);

    return 0;
}
