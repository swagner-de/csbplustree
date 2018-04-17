#include <cstdio>
#include <cstring>
#include <iostream>

using namespace ChunkRefMemoryHandler;

UnusedMemorySubchunk_t::
UnusedMemorySubchunk_t(uint32_t aSize) : size_(aSize), nextFree_(nullptr) {}

UnusedMemorySubchunk_t::
UnusedMemorySubchunk_t(uint32_t aSize, UnusedMemorySubchunk_t *aNextFree) : size_(aSize), nextFree_(aNextFree) {}

byte*
UnusedMemorySubchunk_t::
deliver() {
    return ((byte*) this);
}

template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, bool kBestFit>
MemoryChunk_t<kSizeChunk, kSizeCacheLine, kBestFit>::
MemoryChunk_t() {
    if (posix_memalign((void **) &this->begin_, kSizeCacheLine, kSizeChunk)) {
        throw MemAllocFailedException();
    }
    new(begin_) UnusedMemorySubchunk_t(kSizeChunk);
    firstFree_ = (UnusedMemorySubchunk_t *) begin_;
}

template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, bool kBestFit>
void
MemoryChunk_t<kSizeChunk, kSizeCacheLine, kBestFit>::
freeChunk(){
    free((void*) begin_);
}

template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, bool kBestFit>
uint32_t
MemoryChunk_t<kSizeChunk, kSizeCacheLine, kBestFit>::
getBytesAllocated(){
    uint32_t lSumAllocated = kSizeChunk;
    UnusedMemorySubchunk_t* lCurrent = firstFree_;
    while (lCurrent != nullptr) {
        lSumAllocated -= lCurrent->size_;
        lCurrent = lCurrent->nextFree_;
    }
    return lSumAllocated;
}

template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, bool kBestFit>
byte*
MemoryChunk_t<kSizeChunk, kSizeCacheLine, kBestFit>::
getMem(uint32_t aSize) {
    // assert to have a size of at least one cache line to assure a UnusedMemorySubchunk will fit
    aSize = roundUp(aSize);
    byte* lAddr =(kBestFit) ? bestFit(aSize) : firstFit(aSize);
    if (!contains(lAddr) && lAddr != NULL){
        throw MemAddrNotPartofChunk();
    } else return lAddr;
};

template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, bool kBestFit>
bool
MemoryChunk_t<kSizeChunk, kSizeCacheLine, kBestFit>::
verify() {
    UnusedMemorySubchunk_t *lCurrent = firstFree_;
    while (lCurrent != nullptr) {
        if (!this->contains(lCurrent)) return false;
        lCurrent = lCurrent->nextFree_;
    }
    return true;
}

template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, bool kBestFit>
void
MemoryChunk_t<kSizeChunk, kSizeCacheLine, kBestFit>::
release(byte *aStartAddr, uint32_t aSize) {
if (firstFree_ == nullptr) {
    firstFree_ = new (aStartAddr) UnusedMemorySubchunk_t(aSize);
    return;
    }

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
template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, bool kBestFit>
uint32_t
MemoryChunk_t<kSizeChunk, kSizeCacheLine, kBestFit>::
getFree(){
    UnusedMemorySubchunk_t *lCurrent = firstFree_;
    uint32_t lFree = 0;
    while (lCurrent != nullptr) {
        lFree += lCurrent->size_;
        lCurrent = lCurrent->nextFree_;
    }
    return lFree;
}

template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, bool kBestFit>
bool
MemoryChunk_t<kSizeChunk, kSizeCacheLine, kBestFit>::
contains(UnusedMemorySubchunk_t *aAddr) {
    return this->contains((byte *) aAddr);
}


template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, bool kBestFit>
bool
MemoryChunk_t<kSizeChunk, kSizeCacheLine, kBestFit>::
contains(byte *aAddr) {
    return ((begin_ <= aAddr)
            &&
            (begin_ + kSizeChunk > aAddr));
}

template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, bool kBestFit>
bool
MemoryChunk_t<kSizeChunk, kSizeCacheLine, kBestFit>::
isFullyUnallocated() {
    return (this->firstFree_->size_ == kSizeChunk);
}

template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, bool kBestFit>
uint16_t
MemoryChunk_t<kSizeChunk, kSizeCacheLine, kBestFit>::
roundUp(uint32_t aSize) {
    uint8_t remainder = aSize % kSizeCacheLine;
    if (remainder == 0) return aSize;
    else return aSize + kSizeCacheLine - remainder;
}

template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, bool kBestFit>
byte*
MemoryChunk_t<kSizeChunk, kSizeCacheLine, kBestFit>::
firstFit(uint32_t aSize) {
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

template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, bool kBestFit>
byte*
MemoryChunk_t<kSizeChunk, kSizeCacheLine, kBestFit>::
bestFit(uint32_t aSize) {

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


template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, bool kBestFit>
MemoryHandler_t<kSizeChunk, kSizeCacheLine, kBestFit>::
MemoryHandler_t() {
    chunks_.push_back(ThisMemoryChunk_t());
}

template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, bool kBestFit>
MemoryHandler_t<kSizeChunk, kSizeCacheLine, kBestFit>::
~MemoryHandler_t() {
    for (auto lIt = chunks_.begin(); lIt != chunks_.end(); ++lIt) {
        lIt->freeChunk();
    }
}



template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, bool kBestFit>
byte*
MemoryHandler_t<kSizeChunk, kSizeCacheLine, kBestFit>::
getMem(uint32_t aSize) {
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

template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, bool kBestFit>
bool
MemoryHandler_t<kSizeChunk, kSizeCacheLine, kBestFit>::
verifyPointers(){
    for (auto lIt = chunks_.begin(); lIt != chunks_.end(); ++lIt) {
        if (!lIt->verify()) return false;
        return true;
    }
}

template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, bool kBestFit>
void
MemoryHandler_t<kSizeChunk, kSizeCacheLine, kBestFit>::
release(byte *aStartAddr, uint32_t aSize) {
    // find the chunk that contains the startAddr
    for (auto lIt = chunks_.begin(); lIt != chunks_.end(); ++lIt) {
        if (lIt->contains(aStartAddr)) {
            lIt->release(aStartAddr, aSize);
            return;
        }
    }
}

template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, bool kBestFit>
std::vector<uint32_t>*
MemoryHandler_t<kSizeChunk, kSizeCacheLine, kBestFit>::
getBytesAllocatedPerChunk(){
            std::vector<uint32_t>* lResults = new std::vector<uint32_t>();
            for (auto lIt = chunks_.begin(); lIt != chunks_.end(); ++lIt) {
                lResults->push_back(lIt->getBytesAllocated());
            }
            return lResults;
        }

template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, bool kBestFit>
void
MemoryHandler_t<kSizeChunk, kSizeCacheLine, kBestFit>::
getUsage(MemUsageStats_t &aResult) {
    aResult._numChunksAlloc = this->chunks_.size();
    aResult._bytesFree = 0;
    for (auto lIt = chunks_.begin(); lIt != chunks_.end(); ++lIt) {
        aResult._bytesFree += lIt->getFree();
    }

}

template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, bool kBestFit>
void
MemoryHandler_t<kSizeChunk, kSizeCacheLine, kBestFit>::
printUsage() {
    MemUsageStats_t lMemStats;
    this->getUsage(lMemStats);


    std::cout << " --- Memory Usage --- " << std::endl;
    std::cout << " Chunks allocated: " << lMemStats._numChunksAlloc << std::endl;
    std::cout << " Memory allocated: " << (lMemStats._numChunksAlloc * kSizeChunk) / 1024 << std::endl;
    std::cout << " Free            : " << lMemStats._bytesFree / 1024 << std::endl;
}

