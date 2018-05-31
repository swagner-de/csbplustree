#include <iostream>
#include "art.h"

using std::pair;


template <class Key_t, class Tid_t>
ArtWrapper_t<Key_t, Tid_t>::
ArtWrapper_t() : tree_(){
    if (art_tree_init(&tree_) != 0){
      throw std::runtime_error("Could not initiate tree");
    }
}

template <class Key_t, class Tid_t>
ArtWrapper_t<Key_t, Tid_t>::
~ArtWrapper_t(){
    if (art_tree_destroy(&tree_) != 0){
        std::cout << "Could not destroy tree" << std::endl;
    }
}

template <class Key_t, class Tid_t>
typename ArtWrapper_t<Key_t, Tid_t>::iterator
ArtWrapper_t<Key_t, Tid_t>::
find(Key_t aKey){
    return (iterator) (Tid_t) art_search(&tree_, (unsigned char*) &aKey, sizeof(Key_t));
}

template <class Key_t, class Tid_t>
void
ArtWrapper_t<Key_t, Tid_t>::
insert(pair<Key_t, Tid_t> aKeyVal){
    art_insert(&tree_, (unsigned char*) &aKeyVal.first, sizeof(Key_t), (void*) aKeyVal.second);
}