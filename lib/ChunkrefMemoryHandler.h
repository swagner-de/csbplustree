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
        uint16_t size_; // 2B
        UnusedMemorySubchunk_t *nextFree_; // 8B

        UnusedMemorySubchunk_t(uint16_t aSize) {
            size_ = aSize;
            nextFree_= nullptr;
        }

        UnusedMemorySubchunk_t(uint16_t aSize, UnusedMemorySubchunk_t *aNextFree) {
            nextFree_ = aNextFree;
            size_ = aSize;
        }

        inline byte *deliver() {
            return (byte *) this;
        }
    } __attribute__((packed));


    template<uint16_t kSizeChunk, uint8_t kSizeCacheLine, bool kBestFit>
    class MemoryChunk_t {
    public:
        MemoryChunk_t() {
            if (posix_memalign((void **) &this->begin_, kSizeCacheLine, kSizeChunk)) {
                throw MemAllocFailedException();
            }
            new(begin_) UnusedMemorySubchunk_t(kSizeChunk);
            firstFree_ = (UnusedMemorySubchunk_t *) begin_;
        }

        ~MemoryChunk_t() {
            // TODO release memory
            //free((void *) begin_);
        }

        uint32_t getBytesAllocated(){
            uint32_t lSumAllocated = kSizeChunk;
            UnusedMemorySubchunk_t* lCurrent = firstFree_;
            while (lCurrent != nullptr) {
                lSumAllocated -= lCurrent->size_;
                lCurrent = lCurrent->nextFree_;
            }
            return lSumAllocated;
        }

        byte *getMem(uint16_t aSize) {
            // assert to have a size of at least one cache line to assure a UnusedMemorySubchunk will fit
            aSize = roundUp(aSize);
            byte* lAddr =(kBestFit) ? bestFit(aSize) : firstFit(aSize);
            if (!contains(lAddr)){
                throw MemAddrNotPartofChunk();
            } else return lAddr;
        };

        bool verify() {
            UnusedMemorySubchunk_t *lCurrent = firstFree_;
            while (lCurrent != nullptr) {
                if (!this->contains(lCurrent)) return false;
                lCurrent = lCurrent->nextFree_;
            }
            return true;
        }

        void release(byte *aStartAddr, uint16_t aSize) {
            UnusedMemorySubchunk_t *lPreviousChunk = firstFree_;
            UnusedMemorySubchunk_t *lFollowingChunk;
            UnusedMemorySubchunk_t *lReleasedChunk = (UnusedMemorySubchunk_t*) aStartAddr;

            aSize = roundUp(aSize);

            // find the previous chunk
            while (lPreviousChunk->nextFree_ != nullptr &&
                    (byte*) lPreviousChunk->nextFree_ < aStartAddr) {
                lPreviousChunk = lPreviousChunk->nextFree_;
            }

            // find the following chunk
            if (lPreviousChunk > lReleasedChunk){
                lFollowingChunk = firstFree_;
                firstFree_ = lReleasedChunk;
                lPreviousChunk = nullptr;
            } else {
                lFollowingChunk= lPreviousChunk->nextFree_;
            }

            // check if it can be merged with the previous chunk
            if (lPreviousChunk != nullptr &&
                    ((byte*)lPreviousChunk) + lPreviousChunk->size_ == aStartAddr){
                lPreviousChunk->size_ += aSize;
                lReleasedChunk = lPreviousChunk;
            } else {
                // create a new chunk then
                new (aStartAddr) UnusedMemorySubchunk_t(aSize, lFollowingChunk);

                // update the nextFree pointer of the previous chunk
                if (lPreviousChunk != nullptr){
                    lPreviousChunk->nextFree_ = lReleasedChunk;
                }

            }
            // check if it can be merged with the following chunk
            if (lFollowingChunk != nullptr
                && ((byte*) lReleasedChunk) + lReleasedChunk->size_ == (byte*)lFollowingChunk){
                lReleasedChunk->size_ += lFollowingChunk->size_;
                lReleasedChunk->nextFree_ = lFollowingChunk->nextFree_;
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
                uint32_t _remainingSize;
            };

            BestFittingChunk_t lBestFitting = BestFittingChunk_t();
            lBestFitting._remainingSize = UINT32_MAX;

            UnusedMemorySubchunk_t **lPreviousChunkNextFree = &firstFree_;
            UnusedMemorySubchunk_t *lCurrent = firstFree_;

            while (lCurrent != nullptr && this->contains(lCurrent)) {
                int32_t lRemainingSize = lCurrent->size_ - aSize;
                if (lRemainingSize >= 0 &&
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
                return lBestFitting._self->deliver();
            }
        }
    };

    template<uint16_t kSizeChunk, uint8_t kSizeCacheLine, bool kBestFit>
    class MemoryHandler_t {
    public:
        MemoryHandler_t() {
            chunks_.push_back(ThisMemoryChunk_t());
        }

        byte *getMem(uint32_t aSize) {
            if (aSize > kSizeChunk){
                throw MemAllocTooLarge();
            }
            byte *lFreeMem = nullptr;
            for (auto lIt = chunks_.begin(); lIt != chunks_.end(); ++lIt) {
                lFreeMem = lIt->getMem(aSize);
                if (lFreeMem != nullptr) {
                    return lFreeMem;
                }
            }
            chunks_.push_back(ThisMemoryChunk_t());
            return chunks_.back().getMem(aSize);
        }

        bool verifyPointers(){
            for (auto lIt = chunks_.begin(); lIt != chunks_.end(); ++lIt) {
                if (!lIt->verify()) return false;
                return true;
            }
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

        std::vector<uint32_t>* getBytesAllocatedPerChunk(){
            std::vector<uint32_t>* lResults = new std::vector<uint32_t>();
            for (auto lIt = chunks_.begin(); lIt != chunks_.end(); ++lIt) {
                lResults->push_back(lIt->getBytesAllocated());
            }
            return lResults;
        }

    private:
        using ThisMemoryChunk_t = MemoryChunk_t< kSizeChunk, kSizeCacheLine, kBestFit>;
        std::vector<ThisMemoryChunk_t> chunks_;

    };
    
    template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, bool kBestFit, uint16_t kSizeInnerNode, uint16_t kSizeLeafNode>
    class NodeMemoryManager_t{
    public:
        NodeMemoryManager_t(){
            handler_ = ThisMemoryHandler_t();
        }

        byte * getMem(uint16_t aNumNodes, bool aLeafIndicator) {
            uint16_t lSizeNode = (aLeafIndicator) ? kSizeLeafNode : kSizeInnerNode;
            uint32_t lSizeAlloc = lSizeNode * aNumNodes;
            return handler_.getMem(lSizeAlloc);
        }

        void release(byte * aStartAddr, uint16_t aNumNodes, bool aLeafIndicator) {
            handler_.release(aStartAddr, aNumNodes * (aLeafIndicator) ? kSizeLeafNode : kSizeInnerNode);
        }

        bool verifyPointers(){
            return this->handler_.verifyPointers();
        }

        std::vector<uint32_t>* getBytesAllocatedPerChunk(){
            return handler_.getBytesAllocatedPerChunk();
        }






    private:
        using ThisMemoryHandler_t = MemoryHandler_t<kSizeChunk, kSizeCacheLine, kBestFit>;
        ThisMemoryHandler_t handler_;
    };
};
#endif //CSBPLUSTREE_CHUNKREFMEMORYHANDLER_H
