#include <cstdio>
#include <cstring>
#include <iostream>

using namespace ChunkRefMemoryHandler;

UnusedMemorySubchunk_t::
UnusedMemorySubchunk_t() : nextFree_(nullptr) {}

UnusedMemorySubchunk_t::
UnusedMemorySubchunk_t(UnusedMemorySubchunk_t *aNextFree) : nextFree_(aNextFree) {}

byte*
UnusedMemorySubchunk_t::
deliver() {
    return ((byte*) this);
}

template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, uint32_t kSizeObject>
MemoryChunk_t<kSizeChunk, kSizeCacheLine, kSizeObject>::
MemoryChunk_t() : freeItems_(kSizeChunk/kSizeObject) {
    if (posix_memalign((void **) &this->begin_, kSizeCacheLine, kSizeChunk)) {
        throw MemAllocFailedException();
    }
    new(begin_) UnusedMemorySubchunk_t();
    firstFree_ = (UnusedMemorySubchunk_t *)  begin_;
}

template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, uint32_t kSizeObject>
void
MemoryChunk_t<kSizeChunk, kSizeCacheLine, kSizeObject>::
freeChunk(){
    free((void*) begin_);
}

template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, uint32_t kSizeObject>
uint32_t
MemoryChunk_t<kSizeChunk, kSizeCacheLine, kSizeObject>::
getBytesAllocated(){
    uint32_t lSumAllocated = kSizeChunk;
    UnusedMemorySubchunk_t* lCurrent = firstFree_;
    while (lCurrent != nullptr) {
        lSumAllocated -= kSizeChunk;
        lCurrent = lCurrent->nextFree_;
    }
    return lSumAllocated;
}

template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, uint32_t kSizeObject>
byte*
MemoryChunk_t<kSizeChunk, kSizeCacheLine, kSizeObject>::
getMem(bool aZeroed) {
    if (0 == freeItems_) return nullptr;

    byte* lAddr = firstFree_->deliver();
    if (!contains(lAddr) && lAddr != NULL){
        throw MemAddrNotPartofChunk();
    } else {
        freeItems_--;
        if (nullptr == firstFree_->nextFree_){
            firstFree_ = new ((byte*)firstFree_ + kSizeObject) UnusedMemorySubchunk_t();
        }
        else {
            firstFree_ = firstFree_->nextFree_;
        }
        if (aZeroed) memset(lAddr, 0, kSizeChunk);
        return lAddr;
    }
};

template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, uint32_t kSizeObject>
bool
MemoryChunk_t<kSizeChunk, kSizeCacheLine, kSizeObject>::
verify() {
    UnusedMemorySubchunk_t *lCurrent = firstFree_;
    while (lCurrent != nullptr) {
        if (!this->contains(lCurrent)) return false;
        lCurrent = lCurrent->nextFree_;
    }
    return true;
}

template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, uint32_t kSizeObject>
void
MemoryChunk_t<kSizeChunk, kSizeCacheLine, kSizeObject>::
release(byte *aStartAddr) {
    // find chunk UnusedMemorySubchunk before item
    UnusedMemorySubchunk_t *lCurrent = firstFree_;
    UnusedMemorySubchunk_t *lPrevious = nullptr;
    while ((byte*) lCurrent < aStartAddr && lCurrent != nullptr){
        lPrevious = lCurrent;
        lCurrent = lCurrent->nextFree_;
    }
    
    freeItems_++;

    // Chunk is before the _firstFree ptr
    if (lPrevious == nullptr){
        firstFree_ = new (aStartAddr) UnusedMemorySubchunk_t(firstFree_);
        return;
    }
    lPrevious->nextFree_ = new (aStartAddr) UnusedMemorySubchunk_t(lPrevious->nextFree_);
}

template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, uint32_t kSizeObject>
uint32_t
MemoryChunk_t<kSizeChunk, kSizeCacheLine, kSizeObject>::
getFree(){
    UnusedMemorySubchunk_t *lCurrent = firstFree_;
    uint32_t lFree = 0;
    while (lCurrent != nullptr) {
        lFree += kSizeChunk;
        lCurrent = lCurrent->nextFree_;
    }
    return lFree;
}

template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, uint32_t kSizeObject>
bool
MemoryChunk_t<kSizeChunk, kSizeCacheLine, kSizeObject>::
contains(UnusedMemorySubchunk_t *aAddr) {
    return this->contains((byte *) aAddr);
}


template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, uint32_t kSizeObject>
bool
MemoryChunk_t<kSizeChunk, kSizeCacheLine, kSizeObject>::
contains(byte *aAddr) {
    return ((begin_ <= aAddr)
            &&
            (begin_ + kSizeChunk > aAddr));
}

template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, uint32_t kSizeObject>
bool
MemoryChunk_t<kSizeChunk, kSizeCacheLine, kSizeObject>::
isFullyUnallocated() {
    return (freeItems_ == kSizeChunk/kSizeObject);
}



template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, uint32_t kSizeObject>
MemoryHandler_t<kSizeChunk, kSizeCacheLine, kSizeObject>::
MemoryHandler_t() {
    static_assert(kSizeChunk % kSizeObject == 0, "kSizeChunk is not an integer multiple of kSizeObject");
    chunks_.push_back(ThisMemoryChunk_t());
}

template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, uint32_t kSizeObject>
MemoryHandler_t<kSizeChunk, kSizeCacheLine, kSizeObject>::
~MemoryHandler_t() {
    for (auto lIt = chunks_.begin(); lIt != chunks_.end(); ++lIt) {
        lIt->freeChunk();
    }
}


template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, uint32_t kSizeObject>
byte*
MemoryHandler_t<kSizeChunk, kSizeCacheLine, kSizeObject>::
getMem(bool aZeroed) {
    byte *lFreeMem = nullptr;
    for (auto lIt = chunks_.begin(); lIt != chunks_.end(); ++lIt) {
        lFreeMem = lIt->getMem(aZeroed);
        if (lFreeMem != nullptr) {
            return lFreeMem;
        }
    }
    chunks_.push_back(ThisMemoryChunk_t());
    return chunks_.back().getMem(aZeroed);
}

template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, uint32_t kSizeObject>
bool
MemoryHandler_t<kSizeChunk, kSizeCacheLine, kSizeObject>::
verifyPointers(){
    for (auto lIt = chunks_.begin(); lIt != chunks_.end(); ++lIt) {
        if (!lIt->verify()) return false;
    }
    return true;
}

template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, uint32_t kSizeObject>
void
MemoryHandler_t<kSizeChunk, kSizeCacheLine, kSizeObject>::
release(byte *aStartAddr) {
    // find the chunk that contains the startAddr
    for (auto lIt = chunks_.begin(); lIt != chunks_.end(); ++lIt) {
        if (lIt->contains(aStartAddr)) {
            lIt->release(aStartAddr);
            if (lIt->isFullyUnallocated() && chunks_.size() > 1){
                lIt->freeChunk();
                chunks_.erase(lIt);
            }
            return;
        }
    }
}

template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, uint32_t kSizeObject>
std::vector<uint32_t>*
MemoryHandler_t<kSizeChunk, kSizeCacheLine, kSizeObject>::
getBytesAllocatedPerChunk(){
            std::vector<uint32_t>* lResults = new std::vector<uint32_t>();
            for (auto lIt = chunks_.begin(); lIt != chunks_.end(); ++lIt) {
                lResults->push_back(lIt->getBytesAllocated());
            }
            return lResults;
}

template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, uint32_t kSizeObject>
void
MemoryHandler_t<kSizeChunk, kSizeCacheLine, kSizeObject>::
getUsage(MemUsageStats_t &aResult) {
    aResult._numChunksAlloc = this->chunks_.size();
    aResult._totalBytesAlloc = 0;
    aResult._bytesFree = 0;
    for (auto lIt = chunks_.begin(); lIt != chunks_.end(); ++lIt) {
        aResult._bytesFree += lIt->getFree();
        aResult._totalBytesAlloc += lIt->getSize();
    }

}

template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, uint32_t kSizeObject>
void
MemoryHandler_t<kSizeChunk, kSizeCacheLine, kSizeObject>::
printUsage() {
    MemUsageStats_t lMemStats;
    this->getUsage(lMemStats);

    std::cout << " --- Memory Usage --- " << std::endl;
    std::cout << " Chunks allocated: " << lMemStats._numChunksAlloc << std::endl;
    std::cout << " Memory allocated: " << lMemStats._totalBytesAlloc / 1024 << std::endl;
    std::cout << " Free            : " << lMemStats._bytesFree / 1024 << std::endl;
}

