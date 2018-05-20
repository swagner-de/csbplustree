#include <cstdint>
#include <iostream>
#include <chrono>
#include <random>
#include <stack>
#include <math.h>

#include "../include/FatStack_t.h"

using std::cout;
using std::cerr;
using std::endl;


constexpr uint32_t numRand = 100000000;
using ThisFatStack_t = FatStack_t<uint64_t, numRand + 10000>;
using StdStack_t = std::stack<uint64_t>;
using ThisFatStackMemHandler_t = ThisFatStack_t::StackMemoryHandler_t;


void generateRandom(uint64_t* aRandArr, uint32_t aNum){

    std::minstd_rand0 lRandGen(std::chrono::system_clock::now().time_since_epoch().count());
    for (uint32_t i= 0; i< aNum; i++){
        aRandArr[i] = lRandGen();
    }
}


template <class Stack_t>
void insert(Stack_t *s, uint64_t *aRandArr, uint32_t aNum){
    for (uint32_t i= 0; i< aNum; i++){
        s->push(aRandArr[i]);
    }
}

template <class Stack_t>
void read(Stack_t *s, uint64_t *aRandArr, uint32_t aNum){
    for (uint32_t i= aNum -1; i != 0; i--){
        uint64_t lRes = s->top();
        if (lRes != aRandArr[i]){
            cerr << aNum - i << " item from back should be " << aRandArr[i] << " but was " << lRes << endl;
        }
        s->pop();
    }
}

template <class Stack_t>
double_t measure(void (*fToMeasure) (Stack_t*, uint64_t*, uint32_t), Stack_t *s, uint64_t *aRandArr, uint32_t aNum){
    using namespace std::chrono;
    time_point<high_resolution_clock> begin = high_resolution_clock::now();
    fToMeasure(s, aRandArr, aNum);
    time_point<high_resolution_clock> end = high_resolution_clock::now();
    duration<double_t, std::nano> elapsed(end - begin);
    return elapsed.count();
}



int main(){

    uint64_t *lRandArr = new uint64_t[numRand];
    generateRandom(lRandArr, numRand);

    ThisFatStackMemHandler_t memHandler;
    ThisFatStack_t lFs(numRand, &memHandler);

    double_t lDurFatIns = measure(&insert<ThisFatStack_t>, &lFs, lRandArr, numRand);
    double_t lDurFatRead = measure(&read<ThisFatStack_t>, &lFs, lRandArr, numRand);


    StdStack_t stdStack;
    double_t lDurStdIns = measure(&insert<StdStack_t >, &stdStack, lRandArr, numRand);
    double_t lDurStdRead = measure(&read<StdStack_t >, &stdStack, lRandArr, numRand);


    cout << "Insert in FatStack took: " << lDurFatIns << " nanosecs" << endl
         << "Insert in StdStack took: " << lDurStdIns << " nanosecs" << endl
         << "Read from FatStack took: " << lDurFatRead << " nanosecs" << endl
         << "Read from StdStack took: " << lDurStdRead << " nanosecs" << endl;

    return 0;
}