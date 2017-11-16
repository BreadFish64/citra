
#pragma once

#if defined(_MSC_VER)
/* Microsoft C/C++-compatible compiler */
#include <intrin.h>
#elif defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
/* GCC-compatible compiler, targeting x86/x86-64 */
#include <x86intrin.h>
#endif

// It seems a lot of folks out there compile citra with avx. This lil function
// fixes the penalty by mixing SSE instructions with AVX. For further info:
// https://software.intel.com/en-us/articles/avoiding-avx-sse-transition-penalties
// call this function before running dynamicly generated code.
inline void ZeroUpperAVX() {
#ifdef __AVX__
    _mm256_zeroupper();
#else
// Do Nothing
#endif
}
