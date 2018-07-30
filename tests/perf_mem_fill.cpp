#include <iostream>
#include <cstdint>
#include <random>
#include <fstream>


#include "../include/csbplustree.h"
#include "../include/ChunkrefMemoryHandler.h"

using std::cout;
using std::cerr;
using std::endl;
using std::fstream;


#define _SEED_KEYS 1528311651u
#define _SEED_TIDS 1167121471u


uint32_t const  lKeyCounts [] = {
        64000,
        300000,
        1000000,
        5000000,
        10000000,
        20000000,
        300000000
};

uint32_t const lLenKeyCounts = 7;
ChunkRefMemoryHandler::MemUsageStats_t gMemUsageStats;
fstream gCsv;

template <class tKey, class tTid, uint16_t kSzeNode>
void
fill_tree_rand(){
    using ThisCsbTree_t = CsbTree_t<tKey, tTid, kSzeNode >;

    ThisCsbTree_t lTreeRand;

    std::linear_congruential_engine<tKey , 16807UL, 0UL, 2147483647UL> lRandGenKey(_SEED_KEYS);
    uint64_t i = 0;
    for (uint64_t c= 0; c<lLenKeyCounts; c++){
        for (/**/; i< lKeyCounts[c]; i++){
            lTreeRand.insert(lRandGenKey(),i*100);
        }
        lTreeRand.getMemoryUsage(gMemUsageStats);

        gCsv << lTreeRand.getFillDegree()
             << "," << i
             << "," << gMemUsageStats._totalBytesUsed
             << "," << gMemUsageStats._totalBytesAlloc
             << "," << sizeof(tKey)
             << "," << sizeof(tTid)
             << "," << kSzeNode
             << ",random"
                << endl;

    }
}

template <class tKey, class tTid, uint16_t kSzeNode>
void
fill_tree_incr(){
    using ThisCsbTree_t = CsbTree_t<tKey, tTid, kSzeNode >;

    ThisCsbTree_t lTreeRand;

    uint64_t lCntInserted = 0;

    uint64_t i = 0;
    for (uint64_t c= 0; c<lLenKeyCounts; c++){
        for (/**/; i< lKeyCounts[c]; i++){
            lTreeRand.insert(i,i*100);
        }

        lTreeRand.getMemoryUsage(gMemUsageStats);
        gCsv << lTreeRand.getFillDegree()
             << "," << lCntInserted
             << "," << gMemUsageStats._totalBytesUsed
             << "," << gMemUsageStats._totalBytesAlloc
             << "," << sizeof(tKey)
             << "," << sizeof(tTid)
             << "," << kSzeNode
             << ",incr"
             << endl;
    }
}

template <class tKey, class tTid, uint16_t kSzeNode>
void
fill_tree_decr(){
    using ThisCsbTree_t = CsbTree_t<tKey, tTid, kSzeNode >;

    ThisCsbTree_t lTreeRand;

    uint64_t lCntInserted = 0;
    uint64_t i = 0;
    for (uint64_t c= 0; c<lLenKeyCounts; c++){
        for (/**/; i< lKeyCounts[c]; i++){
            lTreeRand.insert(UINT64_MAX-i,i*100);
        }
        lTreeRand.getMemoryUsage(gMemUsageStats);

        gCsv << lTreeRand.getFillDegree()
             << "," << lCntInserted
             << "," << gMemUsageStats._totalBytesUsed
             << "," << gMemUsageStats._totalBytesAlloc
             << "," << sizeof(tKey)
             << "," << sizeof(tTid)
             << "," << kSzeNode
             << ",decr"
             << endl;
        lCntInserted = lKeyCounts[c];
    }
}







int main(int argc, char *argv[]){
    if (argc < 2) {
        cout
                << "Usage:" << endl
                << argv[0] << " <csvpath>" << endl;
        return 1;
    }

    gCsv.open(argv[1], fstream::app);

    gCsv << "fillDegree,countKeys,MemoryUsed,MemoryAllocated,KeySize,TidSize,CacheLines,method" << endl;


    fill_tree_rand<uint64_t, uint32_t, 1>();
    fill_tree_rand<uint64_t, uint64_t, 1>();
    fill_tree_rand<uint64_t, uint32_t, 2>();
    fill_tree_rand<uint64_t, uint64_t, 2>();
    fill_tree_rand<uint64_t, uint32_t, 3>();
    fill_tree_rand<uint64_t, uint64_t, 3>();
    fill_tree_rand<uint64_t, uint32_t, 4>();
    fill_tree_rand<uint64_t, uint64_t, 4>();
    fill_tree_rand<uint64_t, uint32_t, 5>();
    fill_tree_rand<uint64_t, uint64_t, 5>();
    fill_tree_rand<uint64_t, uint32_t, 6>();
    fill_tree_rand<uint64_t, uint64_t, 6>();
    fill_tree_rand<uint64_t, uint32_t, 7>();
    fill_tree_rand<uint64_t, uint64_t, 7>();
    fill_tree_rand<uint64_t, uint32_t, 8>();
    fill_tree_rand<uint64_t, uint64_t, 8>();

    fill_tree_incr<uint64_t, uint32_t, 1>();
    fill_tree_incr<uint64_t, uint64_t, 1>();
    fill_tree_incr<uint64_t, uint32_t, 2>();
    fill_tree_incr<uint64_t, uint64_t, 2>();
    fill_tree_incr<uint64_t, uint32_t, 3>();
    fill_tree_incr<uint64_t, uint64_t, 3>();
    fill_tree_incr<uint64_t, uint32_t, 4>();
    fill_tree_incr<uint64_t, uint64_t, 4>();
    fill_tree_incr<uint64_t, uint32_t, 5>();
    fill_tree_incr<uint64_t, uint64_t, 5>();
    fill_tree_incr<uint64_t, uint32_t, 6>();
    fill_tree_incr<uint64_t, uint64_t, 6>();
    fill_tree_incr<uint64_t, uint32_t, 7>();
    fill_tree_incr<uint64_t, uint64_t, 7>();
    fill_tree_incr<uint64_t, uint32_t, 8>();
    fill_tree_incr<uint64_t, uint64_t, 8>();

    fill_tree_decr<uint64_t, uint32_t, 1>();
    fill_tree_decr<uint64_t, uint64_t, 1>();
    fill_tree_decr<uint64_t, uint32_t, 2>();
    fill_tree_decr<uint64_t, uint64_t, 2>();
    fill_tree_decr<uint64_t, uint32_t, 3>();
    fill_tree_decr<uint64_t, uint64_t, 3>();
    fill_tree_decr<uint64_t, uint32_t, 4>();
    fill_tree_decr<uint64_t, uint64_t, 4>();
    fill_tree_decr<uint64_t, uint32_t, 5>();
    fill_tree_decr<uint64_t, uint64_t, 5>();
    fill_tree_decr<uint64_t, uint32_t, 6>();
    fill_tree_decr<uint64_t, uint64_t, 6>();
    fill_tree_decr<uint64_t, uint32_t, 7>();
    fill_tree_decr<uint64_t, uint64_t, 7>();
    fill_tree_decr<uint64_t, uint32_t, 8>();
    fill_tree_decr<uint64_t, uint64_t, 8>();




    return 1;

}