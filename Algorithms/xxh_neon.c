#include <arm_neon.h>
#define XXH_STATIC_LINKING_ONLY
#define XXH_IMPLEMENTATION
#define XXH_NAMESPACE neon_
#define XXH_VECTOR XXH_NEON
#include "../xxHash/xxhash.h"
