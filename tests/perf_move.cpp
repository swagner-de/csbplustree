#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <chrono>
#include <iostream>
#include <random>
#include <fstream>


uint32_t constexpr kNumMaxBytesToMove = 4096;
uint32_t constexpr kNumIter = 100000;


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
void print(tKey const * const aSrc, tKey const * const aDst, uint32_t aLen){
    for (uint32_t i =0; i < aLen; i++){
        cout << aSrc[i] << " == " << aDst[i]<< endl;
    }
    cout << "Len: " << aLen << endl << "-------" << endl;
}

template <class tKey>
void
moveMemMove(tKey * const aSrc, tKey * const aDest, uint32_t const aNumKeysToMove){
    if (0 == aNumKeysToMove) return;
    memmove(
            aDest,
            aSrc,
            aNumKeysToMove* sizeof(tKey)
    );
}

template <class tKey>
void
moveLoop(tKey * const aSrc, tKey * const aDest, uint32_t const aNumKeysToMove){
    if (0 == aNumKeysToMove) return;
    for (uint32_t i = 0; i < aNumKeysToMove; i++){
        aDest[i] = aSrc[i];
    }
}

template <class tKey>
void
moveLoopOverlap(tKey * const aSrc, tKey * const aDest, uint32_t const aNumKeysToMove){
    if (0 == aNumKeysToMove) return;
    for (int32_t i = aNumKeysToMove -1; i>=0; i--){
        aDest[i] = aSrc[i];
    }
}


template <class tKey>
double_t measure(
        void (*fToMeasure)(tKey * const, tKey * const , uint32_t const),
        tKey * const aSrc, tKey * const aDest, uint32_t const aNumKeysToMove) {
    using namespace std::chrono;
    time_point<high_resolution_clock> begin = high_resolution_clock::now();
    fToMeasure(aSrc, aDest, aNumKeysToMove);
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
flushLine(fstream& aCsv, uint32_t aSizeItem, uint32_t aOverLapDistance, uint32_t aNumBytes, double_t aAvg){
    aCsv << aSizeItem << "," << aOverLapDistance << "," << aNumBytes << "," << aAvg << "," << endl;
    cout << "sizeItem: " << aSizeItem
         << " | overlapDist: " << aOverLapDistance
         << " | numBytes: " << aNumBytes
         << " | avg: " << aAvg
         << endl;
}


template <class tKey>
void
testLoop(fstream& aCsvFile){

    auto * const lSrcArray = (tKey *) getMem(kNumMaxBytesToMove);
    auto * const lDstArray = (tKey *) getMem(kNumMaxBytesToMove);

    auto lResLoop =  new double_t[kNumIter];

    generateRandom<tKey>(lSrcArray, kNumMaxBytesToMove / sizeof(tKey));


    for (uint32_t lNumBytesToMove = sizeof(tKey);lNumBytesToMove <=  kNumMaxBytesToMove; lNumBytesToMove+= sizeof(tKey)) {
        for (uint32_t i = 0; i < kNumIter; i++) {
            lResLoop[i] = measure(&moveLoop<tKey>, lSrcArray, lDstArray, lNumBytesToMove / sizeof(tKey));
            memset(lDstArray, 0, lNumBytesToMove);
        }
        flushLine(aCsvFile, sizeof(tKey), 0, lNumBytesToMove, average(lResLoop, kNumIter));
    }



    delete[](lSrcArray);
    delete[](lDstArray);

}

void
testMemMove(fstream& aCsvFile){

    using tKey = uint64_t;

    auto * const lSrcArray = (tKey *) getMem(kNumMaxBytesToMove);
    auto * const lDstArray = (tKey *) getMem(kNumMaxBytesToMove);

    auto lResMemMove =  new double_t[kNumIter];

    generateRandom<tKey>(lSrcArray, kNumMaxBytesToMove / sizeof(tKey));

    for (uint32_t lNumBytesToMove = sizeof(tKey);lNumBytesToMove <=  kNumMaxBytesToMove; lNumBytesToMove+= sizeof(tKey)) {
        for (uint32_t i = 0; i<kNumIter; i++){

            lResMemMove[i] = measure(&moveMemMove<tKey>, lSrcArray, lDstArray, lNumBytesToMove / sizeof(tKey));
            memset(lDstArray, 0, lNumBytesToMove);
        }

        flushLine(aCsvFile, 0, 0, lNumBytesToMove, average(lResMemMove, kNumIter));
    }

    delete[](lSrcArray);
    delete[](lDstArray);

}



template <class tKey>
void
testMemMoveOverlap(fstream& aCsvFile){


    auto * const lSrcArray = (tKey *) getMem(kNumMaxBytesToMove + sizeof(tKey));
    auto * const lBackupArray = (tKey *) getMem(kNumMaxBytesToMove + sizeof(tKey));

    auto lResMemMove =  new double_t[kNumIter];

    generateRandom<tKey>(lBackupArray, (kNumMaxBytesToMove+1) / sizeof(tKey));

    for (uint32_t lNumBytesToMove = sizeof(tKey);lNumBytesToMove <=  kNumMaxBytesToMove; lNumBytesToMove+= sizeof(tKey)) {
        for (uint32_t i = 0; i<kNumIter; i++){
            lResMemMove[i] = measure(&moveMemMove<tKey>, lSrcArray, lSrcArray+1, lNumBytesToMove / sizeof(tKey));
            memcpy(lSrcArray, lBackupArray, lNumBytesToMove + sizeof(tKey));
        }

        flushLine(aCsvFile, 0, sizeof(tKey), lNumBytesToMove, average(lResMemMove, kNumIter));
    }

    delete[](lSrcArray);
}

template <class tKey>
void
testLoopOverlap(fstream& aCsvFile){


    auto * const lSrcArray = (tKey *) getMem(kNumMaxBytesToMove + sizeof(tKey));
    auto * const lBackupArray = (tKey *) getMem(kNumMaxBytesToMove + sizeof(tKey));

    auto lResLoop =  new double_t[kNumIter];

    generateRandom<tKey>(lBackupArray, (kNumMaxBytesToMove+1) / sizeof(tKey));
    memcpy(lSrcArray, lBackupArray, kNumMaxBytesToMove + sizeof(tKey));

    for (uint32_t lNumBytesToMove = sizeof(tKey);lNumBytesToMove <=  kNumMaxBytesToMove; lNumBytesToMove+= sizeof(tKey)) {
        for (uint32_t i = 0; i<kNumIter; i++){
            lResLoop[i] = measure(&moveLoopOverlap<tKey>, lSrcArray, lSrcArray+1, lNumBytesToMove / sizeof(tKey));
            memcpy(lSrcArray, lBackupArray, lNumBytesToMove + sizeof(tKey));
        }

        flushLine(aCsvFile, sizeof(tKey), sizeof(tKey), lNumBytesToMove, average(lResLoop, kNumIter));
    }

    delete[](lSrcArray);
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
    lCsvFile << "BytePerItem,OverlapDistance,BytesMoved,avgTime" << endl;

    testMemMove(lCsvFile);
    testMemMoveOverlap<uint32_t >(lCsvFile);
    testMemMoveOverlap<uint64_t >(lCsvFile);
    testLoop<uint32_t >(lCsvFile);
    testLoop<uint64_t >(lCsvFile);
    testLoopOverlap<uint32_t >(lCsvFile);
    testLoopOverlap<uint64_t >(lCsvFile);

    lCsvFile.close();

    return 0;
}

