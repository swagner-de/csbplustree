#ifndef CSBPLUSTREE_GLOBAL_H
#define CSBPLUSTREE_GLOBAL_H

#include <cstddef>


#if __cpp_lib_byte
    #define ptr_t std::byte
#else
    #define ptr_t char
#endif

using byte = ptr_t;


#include <cstdint>
#include <stack>

#include "FatStack_t.h"

static constexpr uint16_t       kSizeCacheLine = 64;

template <class T> using Stack_t = FatStack_t<T>;

#endif //CSBPLUSTREE_GLOBAL_H
