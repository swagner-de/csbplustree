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
        uint16_t size_; // 2B
        UnusedMemorySubchunk_t *nextFree_; // 8B

        UnusedMemorySubchunk_t(uint16_t aSize) {
            size_ = aSize;
        }

        UnusedMemorySubchunk_t(uint16_t aSize, UnusedMemorySubchunk_t *aNextFree) {
            nextFree_ = aNextFree;
            size_ = aSize;
        }

        byte *deliver() {
            memset(this, 0, sizeof(UnusedMemorySubchunk_t));
            return (byte *) this;
        }
    } __attribute__((packed));


    template<uint16_t kSizeChunk, uint8_t kSizeCacheLine, bool kBestFit>
    class MemoryChunk_t {
    public:
        MemoryChunk_t() {
            if (posix_memalign((void **) &begin_, kSizeCacheLine, kSizeChunk)) {
                throw MemAllocFailedException();
            }
            new(begin_) UnusedMemorySubchunk_t(kSizeChunk);
            firstFree_ = (UnusedMemorySubchunk_t *) begin_;
        }

        ~MemoryChunk_t() {
            // TODO release memory
            //free((void *) begin_);
        }

        byte *getMem(uint16_t aSize) {
            // assert to have a size of at least one cache line to assure a UnusedMemorySubchunk will fit
            aSize = roundUp(aSize);
            return (kBestFit) ? bestFit(aSize) : firstFit(aSize);
        };

        void release(byte *aStartAddr, uint16_t aSize) {

            aSize = roundUp(aSize);

            // find the free chunk immediately before the area to be released
            UnusedMemorySubchunk_t *lCurrentChunk = firstFree_;
            UnusedMemorySubchunk_t *lReleasedChunk;

            while ((byte *) lCurrentChunk < aStartAddr) {
                lCurrentChunk = lCurrentChunk->nextFree_;
            }

            // if the two chunks could be merged, do so
            if (aStartAddr + aSize == (byte *) lCurrentChunk->nextFree_) {
                lCurrentChunk->size_ += aSize;
            } else {
                // create a new chunk and correct the links
                lReleasedChunk = new(aStartAddr) UnusedMemorySubchunk_t(aSize);
                lReleasedChunk->nextFree_ = lCurrentChunk->nextFree_;
                lCurrentChunk->nextFree_ = lReleasedChunk;
            }
        }

        inline bool contains(UnusedMemorySubchunk_t *aAddr) {
            return this->contains((byte *) aAddr);
        }

        inline bool contains(byte *aAddr) {
            return ((begin_ <= aAddr)
                    &&
                    (begin_ + kSizeChunk > aAddr));
        }

        inline bool isFullyUnallocated() {
            return (firstFree_->size_ == kSizeChunk);
        }

    private:
        byte *begin_;
        UnusedMemorySubchunk_t *firstFree_;

        inline uint16_t roundUp(uint16_t aSize) {
            uint8_t remainder = aSize % kSizeCacheLine;
            if (remainder == 0) return aSize;
            else return aSize + kSizeCacheLine - remainder;
        }


        byte *firstFit(uint16_t aSize) {
            UnusedMemorySubchunk_t **lPreviousChunkNextFree = &firstFree_;
            UnusedMemorySubchunk_t *lCurrent = firstFree_;
            UnusedMemorySubchunk_t *lRemaining;
            while (lCurrent != nullptr && this->contains(lCurrent)) {

                int16_t lRemainingSize = lCurrent->size_ - aSize;

                if (lRemainingSize >= 0) {
                    // if remaining memory is larger than the requested create a new UnusedMemorySubchunk
                    if (lRemainingSize > 0) {
                        // create a new UnusedMemorySubchunk at the end of the requested memory
                        lRemaining = new(((byte *) lCurrent) + aSize)
                                UnusedMemorySubchunk_t(lRemainingSize, lCurrent->nextFree_);
                        *lPreviousChunkNextFree = lRemaining;
                    } else {
                        // if no space remains
                        *lPreviousChunkNextFree = lCurrent->nextFree_;
                    }
                    return lCurrent->deliver();
                }
                lPreviousChunkNextFree = &lCurrent->nextFree_;
                lCurrent = lCurrent->nextFree_;
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

            UnusedMemorySubchunk_t **lPreviousChunkNextFree = &firstFree_;
            UnusedMemorySubchunk_t *lCurrent = firstFree_;

            while (lCurrent != nullptr && this->contains(lCurrent)) {
                int16_t lRemainingSize = lCurrent->size_ - aSize;
                if (lRemainingSize >= aSize &&
                    lRemainingSize < lBestFitting._remainingSize) {
                    lBestFitting._remainingSize = lRemainingSize;
                    lBestFitting._self = lCurrent;
                    lBestFitting._previousChunkNextFree = lPreviousChunkNextFree;

                    if (lRemainingSize == 0) break;
                }
                lPreviousChunkNextFree = &lCurrent->nextFree_;
                lCurrent = lCurrent->nextFree_;
            }

            if (lBestFitting._self == nullptr) {
                return nullptr;
            } else if (lBestFitting._remainingSize == 0) {
                *lBestFitting._previousChunkNextFree = lBestFitting._self->nextFree_;
                return lBestFitting._self->deliver();
            } else {
                *lBestFitting._previousChunkNextFree =
                        new(((byte *) lBestFitting._self) + aSize)
                                UnusedMemorySubchunk_t(lBestFitting._remainingSize, lBestFitting._self->nextFree_);
            }
        }
    };

    template<uint16_t kSizeChunk, uint8_t kSizeCacheLine, bool kBestFit>
    class MemoryHandler_t {
    public:
        MemoryHandler_t() {
            chunks_.push_back(ThisMemoryChunk_t());
        }

        byte *getMem(uint16_t aSize) {
            if (aSize > kSizeChunk){
                throw MemAllocTooLarge();
            }
            byte *lFreeMem;
            for (auto lIt = chunks_.begin(); lIt != chunks_.end(); ++lIt) {
                lFreeMem = lIt->getMem(aSize);
                if (lFreeMem != nullptr) {
                    return lFreeMem;
                }
            }
            chunks_.push_back(ThisMemoryChunk_t());
            return chunks_.back().getMem(aSize);
        }

        void release(byte *aStartAddr, uint16_t aSize) {

            // find the chunk that contains the startAddr
            for (auto lIt = chunks_.begin(); lIt != chunks_.end(); ++lIt) {
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
        using ThisMemoryChunk_t = MemoryChunk_t< kSizeChunk, kSizeCacheLine, kBestFit>;
        std::vector<ThisMemoryChunk_t> chunks_;

    };
    
    template<uint16_t kSizeChunk, uint8_t kSizeCacheLine, bool kBestFit, uint16_t kSizeInnerNode, uint16_t kSizeLeafNode>
    class NodeMemoryManager{
        NodeMemoryManager(){
            handler_ = ThisMemoryHandler_t();
        }

        byte * getMem(uint16_t aNumNodes, bool aLeafIndicator) {
            return handler_.getMem(aNumNodes * (aLeafIndicator) ? kSizeLeafNode : kSizeInnerNode);
        }

        void release(byte * aStartAddr, uint16_t aNumNodes, bool aLeafIndicator) {
            handler_.release(aStartAddr, aNumNodes * (aLeafIndicator) ? kSizeLeafNode : kSizeInnerNode);
        }


    private:
        using ThisMemoryHandler_t = MemoryHandler_t<kSizeChunk, kSizeCacheLine, kBestFit>;
        ThisMemoryHandler_t handler_;
    };
};
#endif //CSBPLUSTREE_CHUNKREFMEMORYHANDLER_H
