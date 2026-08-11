/* Vix OS build config for the vendored miniz. We use only the low-level tinfl
 * streaming inflate (raw DEFLATE from ZIP entries) and CRC — no stdio, no
 * archive/deflate/zlib layers.
 *
 * CRITICAL (from CrossPoint's notes): the ESP32-C3 mask ROM exports tinfl_* /
 * mz_crc32 / mz_adler32 / mz_free at fixed addresses via linker-script
 * assignments, which can override our object-file definitions and bind inflate
 * to the ROM's differently-laid-out 2021 build, silently corrupting state on
 * real data. Rename the symbols so the linker can never capture the ROM copies.
 *
 * Include this header (not <miniz.h>) so the ROM's identically-guarded
 * esp_rom/include/miniz.h is never used by mistake — we reach the vendored
 * header by explicit relative path below. */
#pragma once

#define MINIZ_NO_STDIO
#define MINIZ_NO_TIME
#define MINIZ_NO_ARCHIVE_APIS
#define MINIZ_NO_ARCHIVE_WRITING_APIS
#define MINIZ_NO_DEFLATE_APIS
#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES

#define tinfl_decompress                 vix_tinfl_decompress
#define tinfl_decompress_mem_to_heap     vix_tinfl_decompress_mem_to_heap
#define tinfl_decompress_mem_to_mem      vix_tinfl_decompress_mem_to_mem
#define tinfl_decompress_mem_to_callback vix_tinfl_decompress_mem_to_callback
#define mz_crc32                         vix_mz_crc32
#define mz_adler32                       vix_mz_adler32
#define mz_free                          vix_mz_free

#include "../third_party/miniz.h"
