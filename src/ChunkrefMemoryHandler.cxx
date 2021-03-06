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
MemoryChunk_t() : begin_(nullptr), freeItems_(kSizeChunk/kSizeObject), firstFree_(nullptr) {
    if (posix_memalign((void **) &this->begin_, kSizeCacheLine, kSizeChunk)) {
        throw MemAllocFailedException();
    }
    firstFree_ = (UnusedMemorySubchunk_t *)  begin_;

    // create chunks from back to front
    UnusedMemorySubchunk_t * lPrevious = nullptr;
    for (byte * i = (begin_ + kSizeChunk - kSizeObject); i>=begin_; i-=kSizeObject){
        lPrevious = new (i) UnusedMemorySubchunk_t(lPrevious);
    }

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
getBytesAllocated() const{
    return ((kSizeChunk/kSizeObject) - freeItems_) * kSizeObject;
}

template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, uint32_t kSizeObject>
byte*
MemoryChunk_t<kSizeChunk, kSizeCacheLine, kSizeObject>::
getMem(bool const aZeroed) {
    if (0 == freeItems_) return nullptr;
    byte* lAddr = firstFree_->deliver();
    freeItems_--;
    firstFree_ = firstFree_->nextFree_;
    if (aZeroed) memset(lAddr, 0, kSizeChunk);
    return lAddr;

};

template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, uint32_t kSizeObject>
bool
MemoryChunk_t<kSizeChunk, kSizeCacheLine, kSizeObject>::
verify() const {
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
release(byte * const aStartAddr) {

    firstFree_ = new (aStartAddr) UnusedMemorySubchunk_t(firstFree_);
    freeItems_++;
}

template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, uint32_t kSizeObject>
uint32_t
MemoryChunk_t<kSizeChunk, kSizeCacheLine, kSizeObject>::
getFree() const{
    return freeItems_ * kSizeObject;
}

template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, uint32_t kSizeObject>
bool
MemoryChunk_t<kSizeChunk, kSizeCacheLine, kSizeObject>::
contains(UnusedMemorySubchunk_t const * const aAddr) const {
    return this->contains((byte *) aAddr);
}


template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, uint32_t kSizeObject>
bool
MemoryChunk_t<kSizeChunk, kSizeCacheLine, kSizeObject>::
contains(byte const * const aAddr) const {
    return ((begin_ <= aAddr)
            &&
            (begin_ + kSizeChunk > aAddr));
}

template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, uint32_t kSizeObject>
bool
MemoryChunk_t<kSizeChunk, kSizeCacheLine, kSizeObject>::
isFullyUnallocated() const {
    return (freeItems_ == kSizeChunk/kSizeObject);
}



template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, uint32_t kSizeObject>
MemoryHandler_t<kSizeChunk, kSizeCacheLine, kSizeObject>::
MemoryHandler_t()  : chunks_(10){
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
getMem(bool const aZeroed) {
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
verifyPointers() const {
    for (auto lIt = chunks_.begin(); lIt != chunks_.end(); ++lIt) {
        if (!lIt->verify()) return false;
    }
    return true;
}

template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, uint32_t kSizeObject>
void
MemoryHandler_t<kSizeChunk, kSizeCacheLine, kSizeObject>::
release(byte * const aStartAddr) {
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
getBytesAllocatedPerChunk() const{
            std::vector<uint32_t>* lResults = new std::vector<uint32_t>();
            for (auto lIt = chunks_.begin(); lIt != chunks_.end(); ++lIt) {
                lResults->push_back(lIt->getBytesAllocated());
            }
            return lResults;
}

template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, uint32_t kSizeObject>
void
MemoryHandler_t<kSizeChunk, kSizeCacheLine, kSizeObject>::
getUsage(MemUsageStats_t &aResult) const {
    aResult._totalBytesAlloc = this->chunks_.size() * kSizeChunk;
    aResult._totalBytesUsed = 0;
    aResult._bytesFree = 0;
    for (auto lIt = chunks_.begin(); lIt != chunks_.end(); ++lIt) {
        aResult._bytesFree += lIt->getFree();
        aResult._totalBytesUsed += lIt->getBytesAllocated();
    }

}

template<uint32_t kSizeChunk, uint8_t kSizeCacheLine, uint32_t kSizeObject>
void
MemoryHandler_t<kSizeChunk, kSizeCacheLine, kSizeObject>::
printUsage() const {
    MemUsageStats_t lMemStats;
    this->getUsage(lMemStats);

    std::cout << " --- Memory Usage --- " << std::endl;
    std::cout << " Memory used     : " << lMemStats._totalBytesUsed /1024 << std::endl;
    std::cout << " Memory allocated: " << lMemStats._totalBytesAlloc / 1024 << std::endl;
    std::cout << " Free            : " << lMemStats._bytesFree / 1024 << std::endl;
}
