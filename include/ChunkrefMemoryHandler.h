#ifndef CSBPLUSTREE_CHUNKREFMEMORYHANDLER_H
#define CSBPLUSTREE_CHUNKREFMEMORYHANDLER_H

#include <vector>
#include <exception>

#include "global.h"


namespace ChunkRefMemoryHandler {

    class MemAllocFailedException : public std::exception {
        virtual const char *what() const throw() {
            return "Error allocating memory";
        }
    };

    class MemAddrNotPartofChunk : public std::exception {
        virtual const char *what() const throw() {
            return "The memory address is not part of this chunk.";
        }
    };

    class MemAllocTooLarge : public std::exception {
        virtual const char *what() const throw() {
            return "Requested memory is too large";
        }
    };

    struct MemUsageStats_t{
        uint32_t _totalBytesAlloc;
        uint32_t _totalBytesUsed;
        uint32_t _bytesFree;
    };

    class UnusedMemorySubchunk_t {
    public:
        UnusedMemorySubchunk_t* nextFree_;

        UnusedMemorySubchunk_t();
        UnusedMemorySubchunk_t(UnusedMemorySubchunk_t *aNextFree);
        inline byte* deliver();

    } __attribute__((packed));


    template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, uint32_t kSizeObject>
    class MemoryChunk_t {
    public:


        MemoryChunk_t();

        void freeChunk();
        inline uint32_t getBytesAllocated() const;
        byte * getMem(bool const aZeroed=false);
        bool verify() const;
        inline void release(byte * const aStartAddr);
        inline uint32_t getFree() const;
        inline bool contains(UnusedMemorySubchunk_t const * const aAddr) const;
        inline bool contains(byte const * const aAddr) const;
        inline bool isFullyUnallocated() const;

    private:
        byte *begin_;
        uint32_t freeItems_;
        UnusedMemorySubchunk_t *firstFree_;

        inline uint32_t roundUp(uint32_t aSize);
        byte *firstFit(uint32_t aSize);
    };

    template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, uint32_t kSizeObject>
    class MemoryHandler_t {
    public:
        MemoryHandler_t();
        ~MemoryHandler_t();

        byte* getMem(bool aZeroed=false);
        bool verifyPointers() const;
        void release(byte * const aStartAddr);
        std::vector<uint32_t>* getBytesAllocatedPerChunk() const;
        void printUsage() const;
        void getUsage(MemUsageStats_t& result) const;


    private:
        typedef MemoryChunk_t< kSizeChunk, kSizeCacheLine, kSizeObject> ThisMemoryChunk_t;
        std::vector<ThisMemoryChunk_t> chunks_;
    };
};

#include "../src/ChunkrefMemoryHandler.cxx"

#endif //CSBPLUSTREE_CHUNKREFMEMORYHANDLER_H
