#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "BLAKE3/c/blake3_impl.h"

#if defined(_M_IX86) ||  defined(_M_X64)
#include <immintrin.h>
#endif

void blake3_compress_in_place(uint32_t cv[8],
                              const uint8_t block[BLAKE3_BLOCK_LEN],
                              uint8_t block_len, uint64_t counter,
                              uint8_t flags) {

#if defined(__AVX512VL__) && defined(__AVX512F__)
  blake3_compress_in_place_avx512(cv, block, block_len, counter, flags);
#elif defined(__SSE4_1__)
  blake3_compress_in_place_sse41(cv, block, block_len, counter, flags);
#elif defined(__SSE2__)
  blake3_compress_in_place_sse2(cv, block, block_len, counter, flags);
#else
  blake3_compress_in_place_portable(cv, block, block_len, counter, flags);
#endif
}

void blake3_compress_xof(const uint32_t cv[8],
                         const uint8_t block[BLAKE3_BLOCK_LEN],
                         uint8_t block_len, uint64_t counter, uint8_t flags,
                         uint8_t out[64]) {

#if defined(__AVX512VL__) && defined(__AVX512F__)
  blake3_compress_xof_avx512(cv, block, block_len, counter, flags, out);
#elif defined(__SSE4_1__)
  blake3_compress_xof_sse41(cv, block, block_len, counter, flags, out);
#elif defined(__SSE2__)
  blake3_compress_xof_sse2(cv, block, block_len, counter, flags, out);
#else
  blake3_compress_xof_portable(cv, block, block_len, counter, flags, out);
#endif
}

void blake3_hash_many(const uint8_t *const *inputs, size_t num_inputs,
                      size_t blocks, const uint32_t key[8], uint64_t counter,
                      bool increment_counter, uint8_t flags,
                      uint8_t flags_start, uint8_t flags_end, uint8_t *out) {

#if defined(__AVX512VL__) && defined(__AVX512F__)
  blake3_hash_many_avx512(inputs, num_inputs, blocks, key, counter,
    increment_counter, flags, flags_start, flags_end,
    out);
#elif defined(__AVX2__)
  blake3_hash_many_avx2(inputs, num_inputs, blocks, key, counter,
    increment_counter, flags, flags_start, flags_end,
    out);
#elif defined(__SSE4_1__)
  blake3_hash_many_sse41(inputs, num_inputs, blocks, key, counter,
    increment_counter, flags, flags_start, flags_end,
    out);
#elif defined(__SSE2__)
  blake3_hash_many_sse2(inputs, num_inputs, blocks, key, counter,
    increment_counter, flags, flags_start, flags_end,
    out);
#elif defined(__ARM_NEON)
  blake3_hash_many_neon(inputs, num_inputs, blocks, key, counter,
    increment_counter, flags, flags_start, flags_end,
    out);
#else
  blake3_hash_many_portable(inputs, num_inputs, blocks, key, counter,
    increment_counter, flags, flags_start, flags_end,
    out);
#endif
}

// The dynamically detected SIMD degree of the current platform.
size_t blake3_simd_degree(void) {

#if defined(__AVX512VL__) && defined(__AVX512F__)
  return 16;
#elif defined(__AVX2__)
  return 8;
#elif defined(__SSE4_1__)
  return 4;
#elif defined(__SSE2__)
  return 4;
#elif defined(__ARM_NEON)
  return 4;
#else
  return 1;
#endif
}
