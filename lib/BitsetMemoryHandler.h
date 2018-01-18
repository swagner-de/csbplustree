#ifndef CSBPLUSTREE_MEMORYHANDLER_H
#define CSBPLUSTREE_MEMORYHANDLER_H

#include <stdint.h>
#include <vector>
#include <bitset>
#include <stdio.h>
#include <cstddef>


namespace BitSetMemoryHandler {

    using byte = std::byte;

    template<uint32_t kNodeSize, uint32_t kChunkSize>
    struct MemoryChunk_t {

        MemoryChunk_t() {
            if (posix_memalign((void **) &_begin, 64, kNodeSize * kChunkSize)) {
                printf("Error allocating memory\n");
            }
            _used.reset();
        }


        // TODO throw exception here
        byte *getChunk(uint16_t aNumNodes) {
            if (_used.all()) {
                return nullptr;
            }
            uint32_t lSlot = 0;
            uint32_t lFreeCounter = 0;
            while (lSlot < _used.size() && lFreeCounter < aNumNodes) {
                if (!_used[lSlot]) {
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
            return _begin + ((lSlot - lFreeCounter) * kNodeSize);
        }

        bool contains(byte *aAddr) {
            return ((_begin <= aAddr)
                    &&
                    (aAddr < &_begin[kChunkSize * kNodeSize])
            );
        }

        void release(byte *aStartAddr, uint16_t aNumNodes) {
            uint32_t lStartIdx = (aStartAddr - _begin) / kNodeSize;
            setBits(lStartIdx, lStartIdx + aNumNodes - 1, false);
        }

    private:
        byte *_begin;
        std::bitset<kChunkSize> _used;

        /*
         * sets the bits to the requested value including the start and end index
         */
        void setBits(uint32_t aBegin, uint32_t aEnd, bool value) {
            while (aBegin != aEnd + 1) {
                _used[aBegin] = value;
                aBegin++;
            };
        }
    };

    template<uint32_t kInnerNodeSize, uint32_t kLeafNodeSize, uint32_t kChunkSize>
    struct MemoryManager_t {
        using InnerNodeMemoryChunk_t = MemoryChunk_t<kInnerNodeSize, kChunkSize>;
        using LeafNodeMemoryChunk_t = MemoryChunk_t<kLeafNodeSize, kChunkSize>;


        byte *getMem(uint16_t aNumNodes, bool aLeafIndicator) {
            return (aLeafIndicator) ? getMemLeaf(aNumNodes) : getMemInner(aNumNodes);
        }

        void release(byte *aStartAddr, uint16_t aNumNodes, bool aLeafIndicator) {
            (aLeafIndicator) ? releaseLeaf(aStartAddr, aNumNodes) : releaseInner(aStartAddr, aNumNodes);
            return;
        }


    private:
        std::vector<InnerNodeMemoryChunk_t> _InnerNodeChunks;
        std::vector<LeafNodeMemoryChunk_t> _LeafNodeChunks;


        // TODO use function template instead
        byte *getMemLeaf(uint16_t aNumNodes) {
            for (auto lIt = _LeafNodeChunks.begin(); lIt != _LeafNodeChunks.end(); ++lIt) {
                byte *lMemPtr = lIt->getChunk(aNumNodes);
                if (lMemPtr != nullptr) {
                    return lMemPtr;
                }
            }
            _LeafNodeChunks.push_back(LeafNodeMemoryChunk_t());
            return _LeafNodeChunks.back().getChunk(aNumNodes);
        }

        byte *getMemInner(uint16_t aNumNodes) {
            for (auto lIt = _LeafNodeChunks.begin(); lIt != _LeafNodeChunks.end(); ++lIt) {
                byte *lMemPtr = lIt->getChunk(aNumNodes);
                if (lMemPtr != nullptr) {
                    return lMemPtr;
                }
            }
            _LeafNodeChunks.push_back(LeafNodeMemoryChunk_t());
            return _LeafNodeChunks.back().getChunk(aNumNodes);
        }

        // TODO use function template instead
        void releaseLeaf(byte *aStartAddr, uint16_t aNumNodes) {
            for (auto lIt = _LeafNodeChunks.begin(); lIt != _LeafNodeChunks.end(); ++lIt) {
                if (lIt->contains(aStartAddr)) {
                    lIt->release(aStartAddr, aNumNodes);
                }
            }
        }

        void releaseInner(byte *aStartAddr, uint16_t aNumNodes) {
            for (auto lIt = _InnerNodeChunks.begin(); lIt != _InnerNodeChunks.end(); ++lIt) {
                if (lIt->contains(aStartAddr)) {
                    lIt->release(aStartAddr, aNumNodes);
                }
            }
        }
    };
};

#endif //CSBPLUSTREE_MEMORYHANDLER_H
