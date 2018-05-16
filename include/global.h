#ifndef CSBPLUSTREE_GLOBAL_H
#define CSBPLUSTREE_GLOBAL_H

#include <cstddef>
#include <cstdint>

#if __cpp_lib_byte
    #define ptr_t std::byte
#else
    #define ptr_t char
#endif

using byte = ptr_t;

#endif //CSBPLUSTREE_GLOBAL_H
