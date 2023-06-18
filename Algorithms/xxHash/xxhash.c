#define XXH_STATIC_LINKING_ONLY
#define XXH_IMPLEMENTATION

#if defined(__AVX512F__)
#  define XXH_VECTOR XXH_AVX512
#elif defined(__AVX2__)
#  define XXH_VECTOR XXH_AVX2
#elif defined(__SSE2__) || defined(_M_AMD64) || defined(_M_X64) || (defined(_M_IX86_FP) && (_M_IX86_FP == 2))
#  define XXH_VECTOR XXH_SSE2
#elif defined(_M_ARM64)
#  define XXH_VECTOR XXH_NEON
#  include <arm_neon.h>
#endif

#include <xxhash.h>
