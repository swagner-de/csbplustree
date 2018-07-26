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
    memmove(
            aDest,
            aSrc,
            aNumKeysToMove* sizeof(tKey)
    );
}

template <class tKey>
void
moveLoop(tKey * const aSrc, tKey * const aDest, uint32_t const aNumKeysToMove){
    for (uint32_t i = 0; i < aNumKeysToMove; i++){
        aDest[i] = aSrc[i];
    }
}

template <class tKey>
void
moveLoopOverlap(tKey * const aSrc, tKey * const aDest, uint32_t const aNumKeysToMove){
    for (int32_t i = aNumKeysToMove -1; i>=0; i--){
        aDest[i] = aSrc[i];
    }
}


template <class tKey>
double_t measure(void (*fToMeasure)(tKey * const, tKey * const , uint32_t const) , tKey * const aSrc, tKey * const aDest, uint32_t const aNumKeysToMove) {
    using namespace std::chrono;
    time_point<high_resolution_clock> begin = high_resolution_clock::now();
    for (uint32_t i = 0; i<kNumIter; i++) {
        fToMeasure(aSrc, aDest, aNumKeysToMove);
    }
    time_point<high_resolution_clock> end = high_resolution_clock::now();
    duration<double_t, std::nano> elapsed(end - begin);
    return elapsed.count()/kNumIter;
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
test(fstream& aCsvFile, void (*fToMeasure)(tKey * const, tKey * const , uint32_t const), bool aOverlap) {

    auto * const lSrcArray = (tKey *) getMem(kNumMaxBytesToMove + sizeof(tKey));
    tKey * lDstArray;
    uint32_t lSizeOverlap;

    if (aOverlap){
        lSizeOverlap = sizeof(tKey);
        lDstArray =  lSrcArray +1;
    }else{
        lSizeOverlap = 0;
        lDstArray = (tKey *) getMem(kNumMaxBytesToMove);
    }


    generateRandom<tKey>(lSrcArray, (kNumMaxBytesToMove / sizeof(tKey))+ 1);

    double_t lRes;

    for (uint32_t lNumBytesToMove = sizeof(tKey);
            lNumBytesToMove <= kNumMaxBytesToMove;
            lNumBytesToMove += sizeof(tKey)) {
        lRes = measure(fToMeasure, lSrcArray, lDstArray, lNumBytesToMove / sizeof(tKey));
        flushLine(aCsvFile, sizeof(tKey), lSizeOverlap, lNumBytesToMove, lRes);
    }

    delete[](lSrcArray);
    if (!lSizeOverlap) delete[](lDstArray);
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

    test<uint32_t>(lCsvFile, &moveLoop<uint32_t>, false);
    test<uint32_t>(lCsvFile, &moveLoopOverlap<uint32_t>, true);

    test<uint64_t>(lCsvFile, &moveLoop<uint64_t>, false);
    test<uint64_t>(lCsvFile, &moveLoopOverlap<uint64_t>, true);

    test<uint64_t>(lCsvFile, &moveMemMove<uint64_t>, false);
    test<uint64_t>(lCsvFile, &moveMemMove<uint64_t>, true);


    lCsvFile.close();

    return 0;
}

