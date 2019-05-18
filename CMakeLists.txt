cmake_minimum_required(VERSION 2.8.6)


project(7zip LANGUAGES C CXX)



add_library (lzmac C/Alloc.c C/Threads.c C/LzFind.c C/LzFindMt.c C/LzmaDec.c C/LzmaEnc.c C/LzmaLib.c)

add_library (7zc C/7zBuf.c C/7zCrc.c C/7zCrcOpt.c C/7zFile.c C/7zStream.c C/7zArcIn.c C/7zDec.c C/7zAlloc.c C/Bcj2.c C/Bra.c C/Bra86.c C/BraIA64.c C/Delta.c C/CpuArch.c C/Lzma2Dec.c)

add_executable(7z C/Util/7z/7zMain.c)
target_link_libraries(7z 7zc lzmac)

set (INCLUDES ${INCLUDES} ${PROJECT_SOURCE_DIR}/C/ ${PROJECT_SOURCE_DIR}/CPP/include_windows ${PROJECT_SOURCE_DIR}/CPP)
include_directories(${INCLUDES})

add_executable(fuzz_lzma_dec test/fuzz/onefile.c test/fuzz/fuzz_lzma_dec.c)
target_link_libraries(fuzz_lzma_dec lzmac)

add_executable(fuzz_lzma_round test/fuzz/onefile.c test/fuzz/fuzz_lzma_round.c)
target_link_libraries(fuzz_lzma_round lzmac)

add_executable(fuzz_7z_list test/fuzz/onefile.c test/fuzz/fuzz_7z_list.c)
target_link_libraries(fuzz_7z_list 7zc lzmac)

add_executable(fuzz_7z_extract test/fuzz/onefile.c test/fuzz/fuzz_7z_extract.c)
target_link_libraries(fuzz_7z_extract 7zc lzmac)

file(GLOB ar7zip_SRC "CPP/7zip/Archive/*.cpp")
add_library (ar7zip ${ar7zip_SRC})

add_executable(fuzz_7z_archive test/fuzz/onefile.c test/fuzz/fuzz_7z_archive.cpp CPP/7zip/Common/StreamObjects.cpp)
target_link_libraries(fuzz_7z_archive ar7zip lzmac)
