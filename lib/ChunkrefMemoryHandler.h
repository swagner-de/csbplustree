#ifndef CSBPLUSTREE_CHUNKREFMEMORYHANDLER_H
#define CSBPLUSTREE_CHUNKREFMEMORYHANDLER_H

#include <cstdint>
#include <vector>
#include <cstddef>
#include <exception>



typedef std::byte byte;


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

    class UnusedMemorySubchunk_t {
    public:
        uint16_t                size_;
        UnusedMemorySubchunk_t* nextFree_;

        UnusedMemorySubchunk_t(uint16_t aSize);
        UnusedMemorySubchunk_t(uint16_t aSize, UnusedMemorySubchunk_t *aNextFree);
        inline byte* deliver();

    } __attribute__((packed));


    template<uint16_t kSizeChunk, uint8_t kSizeCacheLine, bool kBestFit>
    class MemoryChunk_t {
    public:
        MemoryChunk_t();

        void freeChunk();
        uint32_t getBytesAllocated();
        byte *getMem(uint16_t aSize);
        bool verify();
        void release(byte *aStartAddr, uint16_t aSize);
        uint32_t getFree();
        inline bool contains(UnusedMemorySubchunk_t *aAddr);
        inline bool contains(byte *aAddr);
        inline bool isFullyUnallocated();

    private:
        byte *begin_;
        UnusedMemorySubchunk_t *firstFree_;

        inline uint16_t roundUp(uint16_t aSize);
        byte *firstFit(uint16_t aSize);
        byte *bestFit(uint16_t aSize);
    };

    template<uint16_t kSizeChunk, uint8_t kSizeCacheLine, bool kBestFit>
    class MemoryHandler_t {
    public:
        MemoryHandler_t();
        ~MemoryHandler_t();

        byte* getMem(uint32_t aSize);
        bool verifyPointers();
        void release(byte *aStartAddr, uint16_t aSize);
        std::vector<uint32_t>* getBytesAllocatedPerChunk();
        void printUsage();

    private:
        typedef MemoryChunk_t< kSizeChunk, kSizeCacheLine, kBestFit> ThisMemoryChunk_t;
        std::vector<ThisMemoryChunk_t> chunks_;
    };
};

#include "ChunkrefMemoryHandler.cxx"

#endif //CSBPLUSTREE_CHUNKREFMEMORYHANDLER_H
