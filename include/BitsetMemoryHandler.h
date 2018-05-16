#ifndef CSBPLUSTREE_MEMORYHANDLER_H
#define CSBPLUSTREE_MEMORYHANDLER_H

#include <vector>
#include <bitset>
#include <stdio.h>

#include "global.h"

namespace BitSetMemoryHandler {

    using byte = std::byte;

    template<uint32_t kNodeSize, uint32_t kSizeCacheLine, uint32_t kChunkSize>
    struct MemoryChunk_t {

        MemoryChunk_t() {
            if (posix_memalign((void **) &this->begin_, kSizeCacheLine, kChunkSize)) {
                printf("Error allocating memory\n");
            }
            used_.reset();
        }


        // TODO throw exception here
        byte *getChunk(uint16_t aNumNodes) {
            if (used_.all()) {
                return nullptr;
            }
            uint32_t lSlot = 0;
            uint32_t lFreeCounter = 0;
            while (lSlot < used_.size() && lFreeCounter < aNumNodes) {
                if (!used_[lSlot]) {
                    lFreeCounter += 1;
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

        bool contains(byte *aAddr) {
            return ((begin_ <= aAddr)
                    &&
                    (aAddr < &begin_[kChunkSize * kNodeSize])
            );
        }

        void release(byte *aStartAddr, uint16_t aNumNodes) {
            uint32_t lStartIdx = (aStartAddr - begin_) / kNodeSize;
            setBits(lStartIdx, lStartIdx + aNumNodes - 1, false);
        }

    private:
        byte *begin_;
        std::bitset<kChunkSize> used_;

        /*
         * sets the bits to the requested value including the start and end index
         */
        void setBits(uint32_t aBegin, uint32_t aEnd, bool value) {
            while (aBegin != aEnd + 1) {
                used_[aBegin] = value;
                aBegin++;
            };
        }
    };

    template<uint32_t kInnerNodeSize, uint32_t kLeafNodeSize, uint32_t kChunkSize, uint32_t kSizeCacheLine>
    struct MemoryManager_t {
        using InnerNodeMemoryChunk_t = MemoryChunk_t<kInnerNodeSize, kChunkSize, kSizeCacheLine>;
        using LeafNodeMemoryChunk_t = MemoryChunk_t<kLeafNodeSize, kChunkSize, kSizeCacheLine>;


        byte *getMem(uint16_t aNumNodes, bool aLeafIndicator) {
            return (aLeafIndicator) ? getMemLeaf(aNumNodes) : getMemInner(aNumNodes);
        }

        void release(byte *aStartAddr, uint16_t aNumNodes, bool aLeafIndicator) {
            (aLeafIndicator) ? releaseLeaf(aStartAddr, aNumNodes) : releaseInner(aStartAddr, aNumNodes);
            return;
        }


    private:
        std::vector<InnerNodeMemoryChunk_t> InnerNodeChunks_;
        std::vector<LeafNodeMemoryChunk_t> LeafNodeChunks_;


        byte *getMemLeaf(uint16_t aNumNodes) {
            for (auto lIt = LeafNodeChunks_.begin(); lIt != LeafNodeChunks_.end(); ++lIt) {
                byte *lMemPtr = lIt->getChunk(aNumNodes);
                if (lMemPtr != nullptr) {
                    return lMemPtr;
                }
            }
            LeafNodeChunks_.push_back(LeafNodeMemoryChunk_t());
            return LeafNodeChunks_.back().getChunk(aNumNodes);
        }

        byte *getMemInner(uint16_t aNumNodes) {
            for (auto lIt = LeafNodeChunks_.begin(); lIt != LeafNodeChunks_.end(); ++lIt) {
                byte *lMemPtr = lIt->getChunk(aNumNodes);
                if (lMemPtr != nullptr) {
                    return lMemPtr;
                }
            }
            LeafNodeChunks_.push_back(LeafNodeMemoryChunk_t());
            return LeafNodeChunks_.back().getChunk(aNumNodes);
        }

        void releaseLeaf(byte *aStartAddr, uint16_t aNumNodes) {
            for (auto lIt = LeafNodeChunks_.begin(); lIt != LeafNodeChunks_.end(); ++lIt) {
                if (lIt->contains(aStartAddr)) {
                    lIt->release(aStartAddr, aNumNodes);
                }
            }
        }

        void releaseInner(byte *aStartAddr, uint16_t aNumNodes) {
            for (auto lIt = InnerNodeChunks_.begin(); lIt != InnerNodeChunks_.end(); ++lIt) {
                if (lIt->contains(aStartAddr)) {
                    lIt->release(aStartAddr, aNumNodes);
                }
            }
        }
    };
};

#endif //CSBPLUSTREE_MEMORYHANDLER_H
