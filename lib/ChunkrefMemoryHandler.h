#ifndef CSBPLUSTREE_CHUNKREFMEMORYHANDLER_H
#define CSBPLUSTREE_CHUNKREFMEMORYHANDLER_H

#include <stdint.h>
#include <stdio.h>
#include <vector>
#include <cstddef>
#include <exception>


using byte = std::byte;

namespace ChunkRefMemoryHandler {

    class MemAllocFailedException : public std::exception {
        virtual const char *what() const throw() {
            return "Error allocating memory";
        }
    };

    class MemAllocTooLarge : public std::exception {
        virtual const char *what() const throw() {
            return "Requested memory is too large";
        }
    };

    class UnusedMemorySubchunk_t {
    public:
        uint16_t _size; // 2B
        UnusedMemorySubchunk_t *_nextFree; // 8B

        UnusedMemorySubchunk_t(uint16_t aSize) {
            _size = aSize;
        }

        UnusedMemorySubchunk_t(uint16_t aSize, UnusedMemorySubchunk_t *aNextFree) {
            _nextFree = aNextFree;
            _size = aSize;
        }

        byte *deliver() {
            memset(this, 0, sizeof(UnusedMemorySubchunk_t));
            return (byte *) this;
        }
    } __attribute__((packed));


    template<uint16_t kChunkSize, uint8_t kCacheLineSize>
    class MemoryChunk_t {
    public:
        MemoryChunk_t() {
            if (posix_memalign((void **) &_begin, kCacheLineSize, kChunkSize)) {
                throw MemAllocFailedException();
            }
            new(_begin) UnusedMemorySubchunk_t(kChunkSize);
            _firstFree = (UnusedMemorySubchunk_t *) _begin;
        }

        ~MemoryChunk_t() {
            // TODO release memory
            //free((void *) _begin);
        }

        byte *getMem(uint16_t aSize) {
            // assert to have a size of at least one cache line to assure a UnusedMemorySubchunk will fit
            aSize = roundUp(aSize);
            return firstFit(aSize);
        };

        void release(byte *aStartAddr, uint16_t aSize) {

            aSize = roundUp(aSize);

            // find the free chunk immediately before the area to be released
            UnusedMemorySubchunk_t *lCurrentChunk = _firstFree;
            UnusedMemorySubchunk_t *lReleasedChunk;

            while ((byte *) lCurrentChunk < aStartAddr) {
                lCurrentChunk = lCurrentChunk->_nextFree;
            }

            // if the two chunks could be merged, do so
            if (aStartAddr + aSize == (byte *) lCurrentChunk->_nextFree) {
                lCurrentChunk->_size += aSize;
            } else {
                // create a new chunk and correct the links
                lReleasedChunk = new(aStartAddr) UnusedMemorySubchunk_t(aSize);
                lReleasedChunk->_nextFree = lCurrentChunk->_nextFree;
                lCurrentChunk->_nextFree = lReleasedChunk;
            }
        }

        inline bool contains(UnusedMemorySubchunk_t *aAddr) {
            return this->contains((byte *) aAddr);
        }

        inline bool contains(byte *aAddr) {
            return ((_begin <= aAddr)
                    &&
                    (_begin + kChunkSize > aAddr));
        }

        inline bool isFullyUnallocated() {
            return (_firstFree->_size == kChunkSize);
        }

    private:
        byte *_begin;
        UnusedMemorySubchunk_t *_firstFree;

        inline uint16_t roundUp(uint16_t aSize) {
            uint8_t remainder = aSize % kCacheLineSize;
            if (remainder == 0) return aSize;
            else return aSize + kCacheLineSize - remainder;
        }


        byte *firstFit(uint16_t aSize) {
            UnusedMemorySubchunk_t **lPreviousChunkNextFree = &_firstFree;
            UnusedMemorySubchunk_t *lCurrent = _firstFree;
            UnusedMemorySubchunk_t *lRemaining;
            while (lCurrent != nullptr && this->contains(lCurrent)) {

                int16_t lRemainingSize = lCurrent->_size - aSize;

                if (lRemainingSize >= 0) {
                    // if remaining memory is larger than the requested create a new UnusedMemorySubchunk
                    if (lRemainingSize > 0) {
                        // create a new UnusedMemorySubchunk at the end of the requested memory
                        lRemaining = new(((byte *) lCurrent) + aSize)
                                UnusedMemorySubchunk_t(lRemainingSize, lCurrent->_nextFree);
                        *lPreviousChunkNextFree = lRemaining;
                    } else {
                        // if no space remains
                        *lPreviousChunkNextFree = lCurrent->_nextFree;
                    }
                    return lCurrent->deliver();
                }
                lPreviousChunkNextFree = &lCurrent->_nextFree;
                lCurrent = lCurrent->_nextFree;
            }
            return nullptr;
        }


        byte *bestFit(uint16_t aSize) {

            struct BestFittingChunk_t {
                UnusedMemorySubchunk_t *_self;
                UnusedMemorySubchunk_t **_previousChunkNextFree;
                uint16_t _remainingSize;
            };

            BestFittingChunk_t lBestFitting = BestFittingChunk_t();
            lBestFitting._remainingSize = UINT16_MAX;

            UnusedMemorySubchunk_t **lPreviousChunkNextFree = &_firstFree;
            UnusedMemorySubchunk_t *lCurrent = _firstFree;

            while (lCurrent != nullptr && this->contains(lCurrent)) {
                int16_t lRemainingSize = lCurrent->_size - aSize;
                if (lRemainingSize >= aSize &&
                    lRemainingSize < lBestFitting._remainingSize) {
                    lBestFitting._remainingSize = lRemainingSize;
                    lBestFitting._self = lCurrent;
                    lBestFitting._previousChunkNextFree = lPreviousChunkNextFree;

                    if (lRemainingSize == 0) break;
                }
                lPreviousChunkNextFree = &lCurrent->_nextFree;
                lCurrent = lCurrent->_nextFree;
            }

            if (lBestFitting._self == nullptr) {
                return nullptr;
            } else if (lBestFitting._remainingSize == 0) {
                *lBestFitting._previousChunkNextFree = lBestFitting._self->_nextFree;
                return lBestFitting._self->deliver();
            } else {
                *lBestFitting._previousChunkNextFree =
                        new(((byte *) lBestFitting._self) + aSize)
                                UnusedMemorySubchunk_t(lBestFitting._remainingSize, lBestFitting._self->_nextFree);
            }
        }
    };

    template<uint16_t kChunkSize, uint8_t kCacheLineSize>
    class MemoryHandler_t {
    public:
        MemoryHandler_t() {
            _chunks.push_back(MemoryChunk_t<kChunkSize, kCacheLineSize>());
        }

        byte *getMem(uint16_t aSize) {
            if (aSize > kChunkSize){
                throw MemAllocTooLarge();
            }
            byte *lFreeMem;
            for (auto lIt = _chunks.begin(); lIt != _chunks.end(); ++lIt) {
                lFreeMem = lIt->getMem(aSize);
                if (lFreeMem != nullptr) {
                    return lFreeMem;
                }
            }
            _chunks.push_back(MemoryChunk_t<kChunkSize, kCacheLineSize>());
            return _chunks.back().getMem(aSize);
        }

        void release(byte *aStartAddr, uint16_t aSize) {

            // find the chunk that contains the startAddr
            for (auto lIt = _chunks.begin(); lIt != _chunks.end(); ++lIt) {
                if (lIt->contains(aStartAddr)) {
                    lIt->release(aStartAddr, aSize);
                    if (lIt->isFullyUnallocated()) {
                        // TODO how do is safely delete an item without breaking the array while looping
                    }
                    return;
                }
            }
        }

    private:
        std::vector<MemoryChunk_t< kChunkSize, kCacheLineSize>> _chunks;

    };
};
#endif //CSBPLUSTREE_CHUNKREFMEMORYHANDLER_H
