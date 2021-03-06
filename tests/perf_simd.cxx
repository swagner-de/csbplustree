#include <cstdint>
#include <immintrin.h>
#include <random>
#include <chrono>
#include <fstream>
#include <cstdlib>
#include <iostream>
#include <sys/param.h>
#include <cstring>
#include <cstddef>

#define __load__mm256i _mm256_stream_load_si256((__m256i*) &aKeys[i])

#define use(r) __asm__ __volatile__("" :: "m"(r));

constexpr uint32_t kNumMaxKeys = 1000;
constexpr uint32_t kPrecision = 2;
constexpr uint32_t kNumIter = 1000000;


using std::fstream;
using std::cout;
using std::endl;




template <class Key_t>
uint32_t compareSimd(Key_t const aKey, Key_t const * aKeys, uint32_t const aNumKeys);

template <>
uint32_t
compareSimd<uint64_t >(uint64_t const aKey, uint64_t const * aKeys, uint32_t const aNumKeys) {
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
uint32_t
compareSimd<uint32_t >(uint32_t const aKey, uint32_t const * aKeys, uint32_t const aNumKeys) {
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

template <class tKey>
uint32_t
compare(tKey const aKey, tKey const * aKeys, uint32_t const aNumKeys) {
    for (uint16_t i = 0; i < aNumKeys; i++) {
        if (aKeys[i] >= aKey) {
            return i;
        }
    }
    return -1;
}

void *
getMem(size_t const aSize) {
    void *lPtrMem;
    if (posix_memalign(&lPtrMem, 64, aSize)) {
        return nullptr;
    }
    return lPtrMem;
}

template <class tKey>
double_t measure(
        uint32_t (*fToMeasure)(tKey const, tKey const *, uint32_t const),
        tKey const aKey, tKey const * aKeys, uint32_t const aNumKeys) {
    using namespace std::chrono;
    time_point<high_resolution_clock> begin = high_resolution_clock::now();
    uint32_t lRes;
    for (uint32_t i=0; i<kNumIter; i++){
        lRes = fToMeasure(aKey, aKeys, aNumKeys);
        use(lRes);
    }
    time_point<high_resolution_clock> end = high_resolution_clock::now();
    duration<double_t, std::nano> elapsed(end - begin);
    return elapsed.count();
}

double_t
average(double_t const * const aItems, const uint32_t aNumItems){
    double_t lSum = 0;
    for (uint32_t i = 0; i < aNumItems; i++){
        lSum += aItems[i];
    }
    return lSum/aNumItems;
}

double_t std_dev(double_t const * const aItems, const uint32_t aNumItems, double_t aAvg){
    double_t lSum = 0;
    for (uint32_t i = 0; i < aNumItems; i++){
        lSum += pow(aItems[i] - aAvg, 2.0);
    }
    return sqrt(lSum/aNumItems);
}


template <class tKey>
void fillArray(tKey * const aFillArr, uint32_t aNum){
    for (uint32_t i= 0; i< aNum; i++){
        aFillArr[i] = i;
    }
}

template <class tKey>
void inline
rewriteKeys(tKey * const aKeys, tKey * const aLookup){
    fillArray<tKey>(aKeys, kNumMaxKeys);
    memcpy(
            aLookup,
            aKeys,
            sizeof(tKey) * kNumMaxKeys
    );
}

void inline
flushLine(fstream& aCsv, uint32_t aSizeItem, uint32_t aIdxMatch, double_t aAvgSimd, double_t aStdDevSimd, double_t aAvgRegular, double_t aStdDevRegular){
    aCsv << aSizeItem << "," << aIdxMatch << "," << aAvgSimd << "," << aStdDevSimd << "," << aAvgRegular << "," << aStdDevRegular << "," << endl;
}

void inline
flushCache(std::byte const *  aPtr, size_t const aSize){
    std::byte const * const lPtrEnd = aPtr + aSize;
    while (aPtr <= lPtrEnd){
        _mm_clflush((void *) aPtr);
        aPtr += 8;
    }
}

template <class tKey>
void
test(fstream& aCsvFile) {

    auto * const lKeys = (tKey *) getMem(kNumMaxKeys* sizeof(tKey));
    auto * const lLookup = (tKey *) getMem(kNumMaxKeys* sizeof(tKey));

    rewriteKeys(lKeys, lLookup);

    double_t lAvgSimd = 0, lAvgRegular = 0;



    // for match in every position
    for (uint32_t lPosMatch = 0; lPosMatch < kNumMaxKeys; lPosMatch += kPrecision){
        lAvgSimd = measure<tKey>(&compareSimd<tKey>, lLookup[lPosMatch], lKeys, kNumMaxKeys) / kNumIter;
        lAvgRegular = measure<tKey>(&compare<tKey>, lLookup[lPosMatch], lKeys, kNumMaxKeys) / kNumIter;

        cout << "MatchIdx: " << lPosMatch << "   |   AvgSimd: " << lAvgSimd << "   |   AvgReg: " << lAvgRegular << endl;


        flushLine(
                aCsvFile,
                sizeof(tKey),
                lPosMatch,
                lAvgSimd,
                0.0,
                lAvgRegular,
                0.0
        );
    }

    delete[](lKeys);
    delete[](lLookup);
}

int main(int argc, char *argv[]){
    if (argc < 2) {
        cout
                << "Usage:" << endl
                << argv[0] << " <csvpath>" << endl;
        return 1;
    }

    fstream lCsvFile;
    lCsvFile.open(argv[1], fstream::app);
    lCsvFile << "BytePerItem,posItemMatching,avgTimeSimd,stdDevSimd,avgTimeRegular,stdDevRegular" << endl;

    test<uint64_t>(lCsvFile);
    test<uint32_t>(lCsvFile);

    return 0;
}
