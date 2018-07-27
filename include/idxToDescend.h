#ifndef CSBPLUSTREE_IDXTODESCEND_H
#define CSBPLUSTREE_IDXTODESCEND_H

#include <cstdint>

#if __x86_64 && __AVX2__
#include <immintrin.h>

#define __load__mm256i _mm256_stream_load_si256((__m256i*) &aKeys[i])
//#define __load__mm256i _mm256_load_si256((__m256i*) &aKeys[i])
//#define __load__mm256i *(__m256i*)(&aKeys[i])

template <class Key_t>
uint16_t idxToDescend(Key_t const aKey, Key_t const * aKeys, uint16_t const aNumKeys);

template <>
uint16_t
idxToDescend<uint64_t >(uint64_t const aKey, uint64_t const * aKeys, uint16_t const aNumKeys) {
    if (aNumKeys == 0) return 0;
    __m256i lVecKeys;
    __m256i lVecRes;
    __m256i const lVecComp= _mm256_set1_epi64x(aKey);
    uint16_t lIdxItemGt;
    uint16_t const lNumItemsIter = 4;
    uint32_t lMaskRes;

    for (uint16_t i = 0; i < aNumKeys; i+= lNumItemsIter){
        lVecKeys = __load__mm256i;
        lVecRes =  _mm256_or_si256(
                _mm256_cmpeq_epi64 (lVecKeys, lVecComp),
                _mm256_cmpgt_epi64 (lVecKeys, lVecComp)
        );
        lMaskRes = _mm256_movemask_epi8 (lVecRes);
        lIdxItemGt = (__builtin_ctzll(lMaskRes) / 8) + i;
        if (lMaskRes != 0 && aNumKeys > lIdxItemGt){
            return lIdxItemGt;
        }
    }
    return -1;
}


template <>
uint16_t
idxToDescend<int64_t >(int64_t const aKey, int64_t const * aKeys, uint16_t const aNumKeys) {
    if (aNumKeys == 0) return 0;
    __m256i lVecKeys;
    __m256i lVecRes;
    __m256i const lVecComp= _mm256_set1_epi64x(aKey);
    uint16_t lIdxItemGt;
    uint16_t const lNumItemsIter = 4;
    uint32_t lMaskRes;

    for (uint16_t i = 0; i < aNumKeys; i+= lNumItemsIter){
        lVecKeys = __load__mm256i;

        lVecRes =  _mm256_or_si256(
                _mm256_cmpeq_epi64 (lVecKeys, lVecComp),
                _mm256_cmpgt_epi64 (lVecKeys, lVecComp)
        );
        lMaskRes = _mm256_movemask_epi8 (lVecRes);
        lIdxItemGt = (__builtin_ctzll(lMaskRes) / 8) + i;
        if (lMaskRes != 0 && aNumKeys > lIdxItemGt){
            return lIdxItemGt;
        }
    }
    return -1;
}

template <>
uint16_t
idxToDescend<uint32_t >(uint32_t const aKey, uint32_t const * aKeys, uint16_t const aNumKeys) {
    if (aNumKeys == 0) return 0;
    uint16_t const lNumItemsIter = 8;
    uint16_t lIdxItemGt;
    uint32_t lMaskRes;

    __m256i lVecKeys;
    __m256i lVecRes;
    __m256i const lVecComp= _mm256_set1_epi32(aKey);


    for (uint16_t i = 0; i < aNumKeys; i+= lNumItemsIter){
        lVecKeys = __load__mm256i;

        lVecRes =  _mm256_or_si256(
                _mm256_cmpeq_epi32 (lVecKeys, lVecComp),
                _mm256_cmpgt_epi32 (lVecKeys, lVecComp)
        );
        lMaskRes = _mm256_movemask_epi8(lVecRes);

        lIdxItemGt = (__builtin_ctzll(lMaskRes) / 4) + i;
        if (lMaskRes != 0 && aNumKeys > lIdxItemGt){
            return lIdxItemGt;
        }
    }
    return -1;
}

template <>
uint16_t
idxToDescend<int32_t >(int32_t const aKey, int32_t const * aKeys, uint16_t const aNumKeys) {
    if (aNumKeys == 0) return 0;
    uint16_t const lNumItemsIter = 8;
    uint16_t lIdxItemGt;
    uint32_t lMaskRes;

    __m256i lVecKeys;
    __m256i lVecRes;
    __m256i const lVecComp= _mm256_set1_epi32(aKey);


    for (uint16_t i = 0; i < aNumKeys; i+= lNumItemsIter){
        lVecKeys = __load__mm256i;

        lVecRes =  _mm256_or_si256(
                _mm256_cmpeq_epi32 (lVecKeys, lVecComp),
                _mm256_cmpgt_epi32 (lVecKeys, lVecComp)
        );
        lMaskRes = _mm256_movemask_epi8(lVecRes);

        lIdxItemGt = (__builtin_ctzll(lMaskRes) / 4) + i;
        if (lMaskRes != 0 && aNumKeys > lIdxItemGt){
            return lIdxItemGt;
        }
    }
    return -1;
}


#else
#warning "not compiled with AVX instruction set"
template<class Key_t>
uint16_t
idxToDescend(Key_t const aKey, Key_t const * aKeys, uint16_t const aNumKeys) {
    if (aNumKeys == 0) return 0;
    for (uint16_t i = 0; i < aNumKeys; i++) {
        if (aKeys[i] >= aKey) {
            return i;
        }
    }
    return -1;
}
#endif

#endif //CSBPLUSTREE_IDXTODESCEND_H
