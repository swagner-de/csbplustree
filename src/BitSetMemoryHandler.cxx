#include "../include/BitsetMemoryHandler.h"

using namespace BitSetMemoryHandler;
template<uint32_t kNodeSize, uint32_t kSizeCacheLine, uint32_t kNumNodesPerChunk>
MemoryHandler_t<kNodeSize, kSizeCacheLine, kNumNodesPerChunk>::MemoryChunk_t::
MemoryChunk_t() {
    if (posix_memalign((void **) &this->begin_, kSizeCacheLine, kNumNodesPerChunk * kNodeSize)) {
        printf("Error allocating memory\n");
    }
    used_.reset();
}


template<uint32_t kNodeSize, uint32_t kSizeCacheLine, uint32_t kNumNodesPerChpunk>
void
MemoryHandler_t<kNodeSize, kSizeCacheLine, kNumNodesPerChunk>::MemoryChunk_t::
freeChunk() {
    free((void*) begin_);
}

template<uint32_t kNodeSize, uint32_t kSizeCacheLine, uint32_t kNumNodesPerChunk>
byte*
MemoryHandler_t<kNodeSize, kSizeCacheLine, kNumNodesPerChunk>::MemoryChunk_t::
getChunk(uint32_t aNumNodes) {
    if (used_.all()) {
        return nullptr;
    }
    uint32_t lSlot = 0;
    uint32_t lFreeCounter = 0;
    while (lSlot < used_.size() && lFreeCounter < aNumNodes) {
        if (!used_[lSlot]) {
            lFreeCounter++;
        } else {
            lFreeCounter = 0;
        }
        lSlot++;
    }
    if (lFreeCounter != aNumNodes) {
        return nullptr;
    }
    setBits((lSlot - lFreeCounter), lSlot - 1, true);
    return begin_ + ((lSlot - lFreeCounter) * kNodeSize);
}

template<uint32_t kNodeSize, uint32_t kSizeCacheLine, uint32_t kNumNodesPerChunk>
bool
MemoryHandler_t<kNodeSize, kSizeCacheLine, kNumNodesPerChunk>::MemoryChunk_t::
contains(byte *aAddr) {
    return ((begin_ <= aAddr)
            &&
            (aAddr < &begin_[kNumNodesPerChunk * kNodeSize])
    );
}

template<uint32_t kNodeSize, uint32_t kSizeCacheLine, uint32_t kNumNodesPerChunk>
bool
MemoryHandler_t<kNodeSize, kSizeCacheLine, kNumNodesPerChunk>::MemoryChunk_t::
isFullyUnallocated() {
    return used_.none();
};

template<uint32_t kNodeSize, uint32_t kSizeCacheLine, uint32_t kNumNodesPerChunk>
uint32_t
MemoryHandler_t<kNodeSize, kSizeCacheLine, kNumNodesPerChunk>::MemoryChunk_t::
getBytesAllocated() {
    uint32_t lBytesAllocated = 0;
    for (uint32_t i=0; i < used_.size(); i++){
        if (used_[i]) lBytesAllocated += kSizeCacheLine;
    }
    return lBytesAllocated;
}

template<uint32_t kNodeSize, uint32_t kSizeCacheLine, uint32_t kNumNodesPerChunk>
void
MemoryHandler_t<kNodeSize, kSizeCacheLine, kNumNodesPerChunk>::MemoryChunk_t::
release(byte *aStartAddr, uint32_t aNumNodes) {
    uint32_t lStartIdx = (aStartAddr - begin_) / kNodeSize;
    setBits(lStartIdx, lStartIdx + aNumNodes - 1, false);
}

template<uint32_t kNodeSize, uint32_t kSizeCacheLine, uint32_t kNumNodesPerChunk>
void
MemoryHandler_t<kNodeSize, kSizeCacheLine, kNumNodesPerChunk>::MemoryChunk_t::
setBits(uint32_t aBegin, uint32_t aEnd, bool value) {
    while (aBegin != aEnd + 1) {
        used_[aBegin] = value;
        aBegin++;
    };
}

template<uint32_t kNodeSize, uint32_t kSizeCacheLine, uint32_t kNumNodesPerChunk>
MemoryHandler_t<kNodeSize, kSizeCacheLine, kNumNodesPerChunk>::
MemoryHandler_t() {
    chunks_.push_back(MemoryChunk_t());
}


template<uint32_t kNodeSize, uint32_t kSizeCacheLine, uint32_t kNumNodesPerChunk>
MemoryHandler_t<kNodeSize, kSizeCacheLine, kNumNodesPerChunk>::
~MemoryHandler_t() {
    for (auto lIt = chunks_.begin(); lIt != chunks_.end(); ++lIt) {
        lIt->freeChunk();
    }
}

template<uint32_t kNodeSize, uint32_t kSizeCacheLine, uint32_t kNumNodesPerChunk>
uint32_t
MemoryHandler_t<kNodeSize, kSizeCacheLine, kNumNodesPerChunk>::
roundNodes(uint32_t aSize) {
    return (aSize + kNodeSize - 1) / kSizeCacheLine;
}

template<uint32_t kNodeSize, uint32_t kSizeCacheLine, uint32_t kNumNodesPerChunk>
byte*
MemoryHandler_t<kNodeSize, kSizeCacheLine, kNumNodesPerChunk>::
getMem(uint32_t aSize) {
    uint32_t lNumNodes = roundNodes(aSize);

    for (auto lIt = chunks_.begin(); lIt != chunks_.end(); ++lIt) {
        byte *lMemPtr = lIt->getChunk(lNumNodes);
        if (lMemPtr != nullptr) {
            return lMemPtr;
        }
    }
    chunks_.push_back(MemoryChunk_t());
    return chunks_.back().getChunk(lNumNodes);
}

template<uint32_t kNodeSize, uint32_t kSizeCacheLine, uint32_t kNumNodesPerChunk>
void
MemoryHandler_t<kNodeSize, kSizeCacheLine, kNumNodesPerChunk>::
release(byte *aStartAddr, uint32_t aSize) {
    for (auto lIt = chunks_.begin(); lIt != chunks_.end(); ++lIt) {
        if (lIt->contains(aStartAddr)) {
            lIt->release(aStartAddr, roundNodes(aSize));
            if (lIt->isFullyUnallocated() && chunks_.size() > 1){
                lIt->freeChunk();
                chunks_.erase(lIt);
            }
        }
    }
}

template<uint32_t kNodeSize, uint32_t kSizeCacheLine, uint32_t kNumNodesPerChunk>
std::vector<uint32_t>*
MemoryHandler_t<kNodeSize, kSizeCacheLine, kNumNodesPerChunk>::
getBytesAllocatedPerChunk(){
    std::vector<uint32_t>* lResults = new std::vector<uint32_t>();
    for (auto lIt = chunks_.begin(); lIt != chunks_.end(); ++lIt) {
        lResults->push_back(lIt->getBytesAllocated());
    }
    return lResults;
}