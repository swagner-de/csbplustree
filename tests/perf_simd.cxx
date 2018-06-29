#include <cstdint>
#include <immintrin.h>
#include <random>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sys/param.h>


using std::fstream;
using std::cout;
using std::endl;

constexpr uint32_t kNumMaxKeys = 30;
constexpr uint32_t kNumIter = 100000;


template <class tKey>
uint32_t
compareSimd(tKey const aKey, tKey const * aKeys, uint16_t const aNumKeys) {
    __m256i lVecKeys;
    __m256i lVecRes;
    __m256i const lVecComp= _mm256_set1_epi64x(aKey);
    uint16_t lIdxItemGt;
    uint16_t const lNumItemsIter = 4;
    uint32_t lMaskRes;

    for (uint16_t i = 0; i < aNumKeys; i+= lNumItemsIter){
        lVecKeys = _mm256_stream_load_si256((__m256i*) &aKeys[i]);

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

template <class tKey>
uint32_t
compare(tKey const aKey, tKey const * aKeys, uint16_t const aNumKeys) {
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
        uint32_t (*fToMeasure)(tKey const aKey, tKey const * aKeys, uint16_t const aNumKeys),
        tKey const aKey, tKey const * aKeys, uint16_t const aNumKeys) {
    using namespace std::chrono;
    time_point<high_resolution_clock> begin = high_resolution_clock::now();
    fToMeasure(aKey, aKeys, aNumKeys);
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
        lSum += exp2(aItems[i] - aAvg);
    }
    return sqrt(lSum/aNumItems);
}

template <class tKey>
void generateRandom(tKey * const aRandArr, uint32_t aNum){
    std::minstd_rand0 lRandGen(std::chrono::system_clock::now().time_since_epoch().count());
    for (uint32_t i= 0; i< aNum; i++){
        aRandArr[i] = lRandGen();
    }
}

void inline
flushLine(fstream& aCsv, uint32_t aSizeItem, uint32_t aIdxMatch, double_t aAvgSimd, double_t aStdDevSimd, double_t aAvgRegular, double_t aStdDevRegular){
    aCsv << aSizeItem << "," << aIdxMatch << "," << aAvgSimd << "," << aStdDevSimd << "," << aAvgRegular << "," << aStdDevRegular << "," << endl;
}


template <class tKey>
void
test(fstream& aCsvFile) {

    auto * const lKeys = (tKey *) getMem(kNumMaxKeys* sizeof(tKey));
    generateRandom<tKey>(lKeys, kNumMaxKeys);

    auto lResSimd =  new double_t[kNumIter];
    auto lResRegular =  new double_t[kNumIter];
    double_t lAvgSimd, lAvgRegular;


    // for match in every position
    for (uint16_t lPosMatch = 0; lPosMatch < kNumMaxKeys; lPosMatch++){
        for (uint32_t i = 0; i< kNumIter; i++){
            lResSimd[i] = measure<tKey>(compareSimd<tKey>, lKeys[lPosMatch], lKeys, kNumMaxKeys);
            lResRegular[i] = measure<tKey>(compare<tKey>, lKeys[lPosMatch], lKeys, kNumMaxKeys);
        }
        lAvgSimd = average(lResSimd, kNumIter);
        lAvgRegular = average(lResRegular, kNumIter);

        flushLine(
                aCsvFile,
                sizeof(tKey),
                lPosMatch,
                lAvgSimd,
                std_dev(lResSimd, kNumIter, lAvgSimd),
                lAvgRegular,
                std_dev(lResSimd, kNumIter, lAvgRegular)
        );
    }

free(lKeys);

}

int main(){


    fstream lCsvFile;
    lCsvFile.open("perf_simd.csv", fstream::app);
    lCsvFile << "BytePerItem,posItemMatching,avgTimeSimd,stdDevSimd,avgTimeRegular,stdDevRegular" << endl;

    test<int64_t>(lCsvFile);
    test<int32_t>(lCsvFile);
    test<int16_t>(lCsvFile);

    return 0;
}
