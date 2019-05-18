#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "Common/Common.h"
#include "basetyps.h"
#include "7zip/Common/StreamObjects.h"
#include "7zip/Archive/IArchive.h"

FILE * outfile = NULL;
static int initialized = 0;
bool g_CaseSensitive = true;

extern "C" void fuzz_openFile(const char * name) {
    if (outfile != NULL) {
        fclose(outfile);
    }
    outfile = fopen(name, "w");
}

STDAPI CreateInArchiver(int formatIndex, IInArchive **outObject);

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    CBufferInStream *bufStream;
    IInArchive *archive;
    HRESULT result;

    //initialization
    if (initialized == 0) {
        if (outfile == NULL) {
            fuzz_openFile("/dev/null");
        }

        //CREATE_CODECS_OBJECT
        //ThrowException_if_Error(codecs->Load());
        initialized = 1;
    }

    bufStream = new CBufferInStream;
    bufStream->Buf.CopyFrom(Data, Size);
    bufStream->Init();

    //TODO loop
    CreateInArchiver(0, &archive);
    result = archive->Open(bufStream, NULL, NULL);

    printf("lol %x\n", result);
    //TODO all IInArchive possibilities

    archive->Close();
    delete archive;
    delete bufStream;

    return 0;
}

//run
//test against real 7z file
//TODOs
//clean
//rebase
//merge
