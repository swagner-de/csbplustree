#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <chrono>
#include <iostream>
#include <random>
#include <fstream>


uint32_t constexpr lNumMaxBytesToMove = 4096;
uint32_t constexpr lNumIter = 100000;


using std::cout;
using std::endl;
using std::fstream;

void *
getMem(size_t const aSize) {
    void *lPtrMem;
    if (posix_memalign(&lPtrMem, 64, aSize)) {
        return nullptr;
    }
    return lPtrMem;
}

template <class tKey>
void inline
moveMemMove(tKey * const aSrc, tKey * const aDest, uint32_t const aIdxBegin, uint32_t const aNumKeysToMove){
    if (0 == aNumKeysToMove) return;
    memmove(
            aDest,
            &aSrc[aIdxBegin],
            aNumKeysToMove
    );
}

template <class tKey>
void inline
moveLoop(tKey * const aSrc, tKey * const aDest, uint32_t const aIdxBegin, uint32_t const aNumKeysToMove){
    if (0 == aNumKeysToMove) return;
    for (uint32_t i = 0; i < aNumKeysToMove; i++){
        aDest[i] = aSrc[aIdxBegin + i];
    }
}

template <class tKey>
double_t measure(
        void (*fToMeasure)(tKey * const, tKey * const , uint32_t const, uint32_t const),
        tKey * const aSrc, tKey * const aDest, uint32_t const aIdxBegin, uint32_t const aNumKeysToMove) {
    using namespace std::chrono;
    time_point<high_resolution_clock> begin = high_resolution_clock::now();
    fToMeasure(aSrc, aDest, aIdxBegin, aNumKeysToMove);
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

template <class tKey>
void generateRandom(tKey * const aRandArr, uint32_t aNum){
    std::minstd_rand0 lRandGen(std::chrono::system_clock::now().time_since_epoch().count());
    for (uint32_t i= 0; i< aNum; i++){
        aRandArr[i] = lRandGen();
    }
}

void inline
flushLine(fstream& aCsv, uint32_t aSizeItem, uint32_t aNumBytes, double_t aAvg){
    aCsv << aSizeItem << "," << aNumBytes << "," << aAvg << "," << endl;
}


template <class tKey>
void
testLoop(fstream& aCsvFile){

    auto * const lSrcArray = (tKey *) getMem(lNumMaxBytesToMove);
    auto * const lDstArray = (tKey *) getMem(lNumMaxBytesToMove);

    auto lResLoop =  new double_t[lNumIter];

    generateRandom<tKey>(lSrcArray, lNumMaxBytesToMove / sizeof(tKey));


    for (uint32_t lNumBytesToMove = 0; lNumBytesToMove <=  lNumMaxBytesToMove; lNumBytesToMove+= sizeof(tKey)) {
        for (uint32_t i = 0; i < lNumIter; i++) {
            lResLoop[i] = measure(&moveLoop<tKey>, lSrcArray, lDstArray, 0, lNumBytesToMove / sizeof(tKey));
            memset(lDstArray, 0, lNumBytesToMove);
        }
        flushLine(aCsvFile, sizeof(tKey), lNumBytesToMove, average(lResLoop, lNumIter));
    }



    delete[](lSrcArray);
    delete[](lDstArray);

}

void
testMemMove(fstream& aCsvFile){

    using tKey = uint64_t;

    auto * const lSrcArray = (tKey *) getMem(lNumMaxBytesToMove);
    auto * const lDstArray = (tKey *) getMem(lNumMaxBytesToMove);

    auto lResMemMove =  new double_t[lNumIter];

    generateRandom<tKey>(lSrcArray, lNumMaxBytesToMove / sizeof(tKey));

    for (uint32_t lNumBytesToMove = 0; lNumBytesToMove <=  lNumMaxBytesToMove; lNumBytesToMove+= sizeof(tKey)) {
        for (uint32_t i = 0; i<lNumIter; i++){

            lResMemMove[i] = measure(&moveMemMove<tKey>, lSrcArray, lDstArray, 0, lNumBytesToMove / sizeof(tKey));
            memset(lDstArray, 0, lNumBytesToMove);
        }

        flushLine(aCsvFile, 0, lNumBytesToMove, average(lResMemMove, lNumIter));
    }

    delete[](lSrcArray);
    delete[](lDstArray);

}


int
main(){


    fstream lCsvFile;
    lCsvFile.open("perf_move.csv", fstream::app);
    lCsvFile << "BytePerItem,BytesMoved,avgTime" << endl;

    testMemMove(lCsvFile);
    testLoop<uint32_t >(lCsvFile);
    testLoop<uint64_t >(lCsvFile);

    lCsvFile.close();

    return 0;
}

