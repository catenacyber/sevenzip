cmake_minimum_required(VERSION 2.8.6)


project(7zip LANGUAGES C CXX)


add_library (lzma C/Alloc.c C/Threads.c C/LzFind.c C/LzFindMt.c C/LzmaDec.c C/LzmaEnc.c C/LzmaLib.c)

set (INCLUDES ${INCLUDES} ${PROJECT_SOURCE_DIR}/C/)
include_directories(${INCLUDES})

add_executable(fuzz_lzma_dec test/fuzz/onefile.c test/fuzz/fuzz_lzma_dec.c)
target_link_libraries(fuzz_lzma_dec lzma)

add_executable(fuzz_lzma_round test/fuzz/onefile.c test/fuzz/fuzz_lzma_round.c)
target_link_libraries(fuzz_lzma_round lzma)
