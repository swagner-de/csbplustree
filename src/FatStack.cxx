#include "../include/global.h"
#include "../include/FatStack_t.h"

template <class T, uint32_t kNumMax>
FatStack_t<T, kNumMax>::
FatStack_t(uint32_t aSizeMin, StackMemoryHandler_t *aMemoryHandler)
        : memoryHandler_(aMemoryHandler), sizeCurrent_(0), sizeAllocated_(aSizeMin)
{
    items_ = (T*) memoryHandler_->getMem(aSizeMin * sizeof(T));
}

template <class T, uint32_t kNumMax>
FatStack_t<T, kNumMax>::
~FatStack_t() {
    memoryHandler_->release((byte*) items_, sizeAllocated_ * sizeof(T));
}


template <class T, uint32_t kNumMax>
void
FatStack_t<T, kNumMax>::
push(T item) {
    if (sizeCurrent_ == sizeAllocated_){
        reallocate();
    }
    if (sizeCurrent_ == 0) *items_ = item;
    else *(currentHead() + 1) = item;
    sizeCurrent_++;
}


template <class T, uint32_t kNumMax>
void
FatStack_t<T, kNumMax>::
pop() {
    if (sizeCurrent_ == 0){
        return;
    }
    if (sizeCurrent_ == 1){
        sizeCurrent_ = 0;
        return;
    }
    --sizeCurrent_;
}


template <class T, uint32_t kNumMax>
T
FatStack_t<T, kNumMax>::
top() {
    if (sizeCurrent_ == 0){
        return NULL;
    }
    return *currentHead();
}

template <class T, uint32_t kNumMax>
uint64_t
FatStack_t<T, kNumMax>::
size(){
    return sizeCurrent_;
};


template <class T, uint32_t kNumMax>
bool
FatStack_t<T, kNumMax>::
empty() {
    return (sizeCurrent_ == 0);
};


template <class T, uint32_t kNumMax>
void
FatStack_t<T, kNumMax>::
reallocate()  {
    uint32_t lSizeNewAlloc = (uint64_t) (sizeAllocated_ * 1.5);
    T* lNewItemsPtr = (T*) memoryHandler_->getMem(lSizeNewAlloc * sizeof(T));
    memmove(
            lNewItemsPtr,
            items_,
            sizeCurrent_ * sizeof(T)
    );
    sizeAllocated_ = lSizeNewAlloc;
    memoryHandler_->release((byte* )items_, sizeCurrent_ * sizeof(T));
    items_ = lNewItemsPtr;
}

template <class T, uint32_t kNumMax>
T*
FatStack_t<T, kNumMax>::
currentHead() {
    if (sizeCurrent_ == 0) return items_;
    return items_ + (sizeCurrent_ - 1);
}