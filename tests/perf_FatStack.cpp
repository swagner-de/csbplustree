#include <cstdint>
#include <iostream>
#include <chrono>
#include <random>
#include <stack>
#include <math.h>

#include <fstream>

#include "../include/FatStack_t.h"

#define use(r) __asm__ __volatile__("" :: "m"(r));



using std::cout;
using std::cerr;
using std::endl;
using std::fstream;

constexpr uint32_t kNumMaxItems = 10000;
constexpr uint32_t kNumIter = 100000;
constexpr uint32_t kPrecision = 4;


using PushPopVal_t = std::pair<double_t, double_t>;
using ThisFatStack_t = FatStack_t<uint64_t, kNumMaxItems + 10000>;
using StdStack_t = std::stack<uint64_t>;


void generateRandom(uint64_t * const aRandArr, uint32_t const aNum){

    std::minstd_rand0 lRandGen(std::chrono::system_clock::now().time_since_epoch().count());
    for (uint32_t i= 0; i< aNum; i++){
        aRandArr[i] = lRandGen();
    }
}


template <class Stack_t>
void insertK(Stack_t * const aStack, uint64_t const * const aRandArr, uint32_t const aK){
    for (uint32_t i= 0; i< aK; i++){
        aStack->push(aRandArr[i]);
    }
}

template <class Stack_t>
void popK(Stack_t * const aStack, uint64_t const * const aRandArr, uint32_t const aK){
    for (uint32_t i= 0; i < aK; i++){
        uint64_t lRes = aStack->top();
        use(lRes);
        aStack->pop();
    }
}

template <class Stack_t>
double_t measure(void (*fToMeasure) (Stack_t * const, uint64_t const * const, uint32_t const), Stack_t * aStack, uint64_t const * const aRandArr, uint32_t const aK){
    using namespace std::chrono;
    time_point<high_resolution_clock> begin = high_resolution_clock::now();
    fToMeasure(aStack, aRandArr, aK);
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

template <class Stack_t>
PushPopVal_t
loopMeasure(uint64_t const * const aRandArr, uint64_t const aNumInsert);

template <>
PushPopVal_t
loopMeasure<ThisFatStack_t >(uint64_t const * const aRandArr, uint64_t const aNumInsert){

    ThisFatStack_t * lStack = new ThisFatStack_t(kNumMaxItems);

    auto * const lResVecIns = new double_t[kNumIter];
    auto * const lResVecPop = new double_t[kNumIter];
    for (uint32_t i = 0; i < kNumIter; i++){
        lResVecIns[i] = measure(&insertK<ThisFatStack_t>, lStack, aRandArr, aNumInsert);
        lResVecPop[i] = measure(&popK<ThisFatStack_t>, lStack, aRandArr, aNumInsert);
        delete lStack;
        lStack = new ThisFatStack_t(kNumMaxItems);
    }
    return std::make_pair(average(lResVecIns, kNumIter), average(lResVecPop, kNumIter));
}


template <>
PushPopVal_t
loopMeasure<StdStack_t >(uint64_t const * const aRandArr, uint64_t const aNumInsert){

    StdStack_t * lStack = new StdStack_t();

    auto * const lResVecIns = new double_t[kNumIter];
    auto * const lResVecPop = new double_t[kNumIter];
    for (uint32_t i = 0; i < kNumIter; i++){
        lResVecIns[i] = measure(&insertK<StdStack_t>, lStack, aRandArr, aNumInsert);
        lResVecPop[i] = measure(&popK<StdStack_t>, lStack, aRandArr, aNumInsert);
        delete lStack;
        lStack = new StdStack_t();
    }
    return std::make_pair(average(lResVecIns, kNumIter), average(lResVecPop, kNumIter));
}

void
flushLine(fstream& aCsv, uint32_t aNumItems, double_t aAvgTimePushFatStack, double_t aAvgTimePushStdStack, double_t aAvgTimePopFatStack, double_t aAvgTimePopStdStack){
    aCsv << aNumItems << "," << aAvgTimePushFatStack << "," << aAvgTimePushStdStack<< "," << aAvgTimePopFatStack << "," << aAvgTimePopStdStack << endl;
    cout << "numItems: " << aNumItems
         << "| AvgTimePushFatStack: " << aAvgTimePopStdStack
         << "| AvgTimePushStdStack: " << aAvgTimePushStdStack
         << "| AvgTimePopFatStack: " << aAvgTimePopFatStack
         << "| AvgTimePopStdStack: " << aAvgTimePopStdStack
         << endl;
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
    lCsvFile << "numItems,AvgTimePushFatStack,AvgTimePushStdStack,AvgTimePopFatStack,AvgTimePopStdStack" << endl;

    uint64_t * const lRandArr = new uint64_t[kNumMaxItems];
    generateRandom(lRandArr, kNumMaxItems);


    for (uint64_t i = 0; i < kNumMaxItems; i += kPrecision){
        PushPopVal_t lResStd = loopMeasure<StdStack_t> (lRandArr, i);
        PushPopVal_t lResFat = loopMeasure<ThisFatStack_t> (lRandArr, i);
        flushLine(lCsvFile, i, lResFat.first, lResStd.first, lResFat.second, lResStd.second);
    }




    return 0;
}