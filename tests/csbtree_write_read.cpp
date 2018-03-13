#include <iostream>
#include "../lib/csbplustree.h"


typedef CsbTree_t<uint32_t , uint64_t, 1> Tree_t;


inline uint64_t mapKey(uint32_t aKey){
    return ((uint64_t) aKey) * 1000;
}

inline void checkResult(Tree_t* aTree, uint32_t aKey, uint64_t aTid){
    uint64_t lRes;
    if (aTree->find(aKey, &lRes) != 0) {
        std::cout << "Key " << aKey << " not found" << std::endl;
    } else if (lRes != aTid){
        std::cout << "Value " << lRes << " at lookup of " << aKey << " incorrect" << std::endl;
    }
}

int main() {
    std::string basePath = "/home/sebas/dev/Uni/master_thesis/csbplustree/visual/trees/";
    uint32_t maxKeys = 10000;
    uint32_t savePoint = 9973;
    uint64_t numKeysTree;



    Tree_t tree1 =  Tree_t();

    std::cout << "Inserting from right to left" << std::endl;

    for (uint32_t i = maxKeys; i !=0 ; i--){
        tree1.insert(i, mapKey(i));
        numKeysTree = tree1.getNumKeys();
        if ((maxKeys - i + 1) != numKeysTree){
            std::cout << "#Keys does not match! Was " << numKeysTree << " but should be " << (maxKeys - i + 1) << std::endl;
            tree1.getNumKeys();
        }
        for (uint64_t j = i; j <= maxKeys; j++){
            checkResult(&tree1, j, mapKey(j));
        }

        if (i == savePoint){
            tree1.saveTreeAsJson(basePath + "tree1_at" + std::to_string(i) + ".json");
        }
    }

    std::cout << "Inserting from left to right" << std::endl;

    Tree_t tree2 =  Tree_t();


    for (uint32_t i = 0; i != maxKeys ; i++){
        tree2.insert(i, mapKey(i));

        for (uint64_t j = 0; j <= i; j++){
            checkResult(&tree2, j, mapKey(j));
        }
    }

    return 0;
}


