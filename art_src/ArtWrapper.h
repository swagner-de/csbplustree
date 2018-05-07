#ifndef CSBPLUSTREE_ARTWRAPPER_H
#define CSBPLUSTREE_ARTWRAPPER_H

#include <exception>

#include "art.h"

using std::pair;


template <class Key_t, class Tid_t>
class ArtWrapper_t {
private:
    art_tree tree_;

public:

    struct iterator_tt {
        Tid_t   second;
    };

    using iterator = iterator_tt*;
    using key_type = Key_t;
    using mapped_type = Tid_t;

    ArtWrapper_t();

    ~ArtWrapper_t();

    inline iterator find(Key_t aKey);

    inline void insert(pair<Key_t, Tid_t> aKeyVal);
};

#include "ArtWrapper.cxx"

#endif //CSBPLUSTREE_ARTWRAPPER_H
