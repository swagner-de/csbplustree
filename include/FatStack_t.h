#ifndef FATSTACK_T_H
#define FATSTACK_T_H


#include <cstdint>
#include "ChunkrefMemoryHandler.h"

template <class T, uint32_t kNumMax=16384>
class FatStack_t{

public:

    using StackMemoryHandler_t = ChunkRefMemoryHandler::MemoryHandler_t<sizeof(T) * kNumMax, 64, true>;

    FatStack_t(uint32_t aSizeMin, StackMemoryHandler_t* aMemoryHandler);

    ~FatStack_t();

    T top();
    bool empty();
    uint64_t size();

    void push(T item);
    void pop();

private:


    StackMemoryHandler_t* memoryHandler_;
    uint32_t sizeCurrent_;
    uint32_t sizeAllocated_;
    T* items_;

    void reallocate();
    inline T* currentHead();
};

#include "../src/FatStack.cxx"

#endif //FATSTACK_T_H