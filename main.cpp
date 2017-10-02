#include <iostream>
#include "csbplustree.h"
#include <list>
#include <map>



int main() {



    Tree<int> tree = Tree<int>(5, 5);
    std::map<int, TidList*> key_map;

    for (unsigned int i; i<INT32_MAX/4; i++){
        if(rand() & 1){
            TidList* tids;
            tids->push_back(i*10);
            key_map.insert(std::make_pair(i, tids));
        }
    }
    tree.bulk_insert(&key_map);
}