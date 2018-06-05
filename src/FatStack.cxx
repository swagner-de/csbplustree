#include <memory>

#include "../include/global.h"
#include "../include/FatStack_t.h"

template <class T, uint32_t kNumMax>
FatStack_t<T, kNumMax>::
FatStack_t(uint32_t aSizeMin)
        : sizeCurrent_(0), sizeAllocated_(aSizeMin), items_(new T[aSizeMin]){}


template <class T, uint32_t kNumMax>
FatStack_t<T, kNumMax>::
~FatStack_t() {
   delete[] items_;
}


template <class T, uint32_t kNumMax>
void
FatStack_t<T, kNumMax>::
push(T const item) {
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
    --sizeCurrent_;
}


template <class T, uint32_t kNumMax>
T
FatStack_t<T, kNumMax>::
top() const {
    if (sizeCurrent_ == 0){
        throw EmptyStackException();
    }
    return *currentHead();
}

template <class T, uint32_t kNumMax>
uint64_t
FatStack_t<T, kNumMax>::
size() const {
    return sizeCurrent_;
};


template <class T, uint32_t kNumMax>
bool
FatStack_t<T, kNumMax>::
empty() const {
    return (sizeCurrent_ == 0);
};


template <class T, uint32_t kNumMax>
void
FatStack_t<T, kNumMax>::
reallocate()  {
    uint32_t lSizeNewAlloc = (uint64_t) (sizeAllocated_ * 1.5);
    T* lNewItemsPtr = new T[lSizeNewAlloc];
    memmove(
            lNewItemsPtr,
            items_,
            sizeCurrent_ * sizeof(T)
    );
    sizeAllocated_ = lSizeNewAlloc;
    delete[] items_;
    items_ = lNewItemsPtr;
}

template <class T, uint32_t kNumMax>
T*
FatStack_t<T, kNumMax>::
currentHead() const {
    if (sizeCurrent_ == 0) return items_;
    return items_ + (sizeCurrent_ - 1);
}