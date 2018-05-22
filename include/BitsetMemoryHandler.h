#ifndef CSBPLUSTREE_MEMORYHANDLER_H
#define CSBPLUSTREE_MEMORYHANDLER_H

#include <vector>
#include <bitset>
#include <cstdint>

#include "global.h"

namespace BitSetMemoryHandler {

    template<uint32_t kNodeSize, uint32_t kSizeCacheLine, uint32_t kNumNodesPerChunk>
    class MemoryHandler_t {
    public:

        MemoryHandler_t();
        ~MemoryHandler_t();

        byte *getMem(uint32_t aSize);
        void release(byte *aStartAddr, uint32_t aSize);
        std::vector<uint32_t>* getBytesAllocatedPerChunk();


    private:

        class MemoryChunk_t {
        public:

            MemoryChunk_t();

            void freeChunk();
            byte *getChunk(uint32_t aNumNodes);

            bool contains(byte *aAddr);
            bool isFullyUnallocated();
            void release(byte *aStartAddr, uint32_t aNumNodes);
            uint32_t getBytesAllocated();

        private:
            byte *begin_;
            std::bitset<kNumNodesPerChunk> used_;

            /*
             * sets the bits to the requested value including the start and end index
             */
            void setBits(uint32_t aBegin, uint32_t aEnd, bool value);
        };


        std::vector<MemoryChunk_t> chunks_;

        uint32_t inline roundNodes(uint32_t aSize);
    };
};

#include "../src/BitSetMemoryHandler.cxx"

#endif //CSBPLUSTREE_MEMORYHANDLER_H
