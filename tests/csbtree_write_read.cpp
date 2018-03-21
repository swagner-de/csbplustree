#include <iostream>
#include <chrono>
#include <random>
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

int insert_decr(uint32_t maxKeys, uint32_t savePoint, std::string basePath) {

    Tree_t tree = Tree_t();
    uint64_t numKeysTree;

    std::cout << "Inserting keys decreasingly" << std::endl;

    for (uint32_t i = maxKeys; i != 0; i--) {
        tree.insert(i, mapKey(i));
        numKeysTree = tree.getNumKeys();
        if ((maxKeys - i + 1) != numKeysTree)
            std::cout << "#Keys does not match! Was " << numKeysTree << " but should be " << (maxKeys - i + 1) << std::endl;

        if (tree.getNumKeysBackwards() != numKeysTree)
            std::cout << "Backwards count incorrect" << std::endl;

        for (uint64_t j = i; j <= maxKeys; j++)
            checkResult(&tree, j, mapKey(j));


        if (numKeysTree == savePoint)
            tree.saveTreeAsJson(basePath + "tree_decr_at" + std::to_string(numKeysTree) + ".json");

        if (!tree.verifyOrder()) std::cout << "Tree out of order" << std::endl;
    }
    return 0;
}

int insert_incr(uint32_t maxKeys, uint32_t savePoint, std::string basePath) {
    std::cout << "Inserting keys increasingly" << std::endl;

    Tree_t tree = Tree_t();
    uint64_t numKeysTree;

    for (uint32_t i = 0; i != maxKeys; i++) {
        tree.insert(i, mapKey(i));
        numKeysTree = tree.getNumKeys();
        if (i + 1 != numKeysTree)
            std::cout << "#Keys does not match! Was " << numKeysTree << " but should be " << i + 1 << std::endl;


        if (tree.getNumKeysBackwards() != numKeysTree)
            std::cout << "Backwards count incorrect" << std::endl;


        for (uint64_t j = 0; j <= i; j++)
            checkResult(&tree, j, mapKey(j));

        if (numKeysTree == savePoint)
            tree.saveTreeAsJson(basePath + "tree_incr_at" + std::to_string(numKeysTree) + ".json");

        if (!tree.verifyOrder()) std::cout << "Tree out of order" << std::endl;
    }
    return 0;
}

int insert_rand(uint32_t maxKeys, uint32_t savePoint, std::string basePath,uint64_t seed){
    std::cout << "Inserting randomly with seed " << seed  << std::endl;

    Tree_t tree = Tree_t();

    std::minstd_rand0 random(seed);
    uint32_t inserted_vals[maxKeys];


    uint64_t numKeysTree;

    for (uint32_t i = 0; i != maxKeys; i++) {
        uint32_t  r_val = random();
        inserted_vals[i] = r_val;
        tree.insert(r_val, mapKey(r_val));
        numKeysTree = tree.getNumKeys();
        if (i +1 != numKeysTree)
            std::cout << "#Keys does not match! Was " << numKeysTree << " but should be " << (i+1) << std::endl;

        if (tree.getNumKeysBackwards() != numKeysTree)
            std::cout << "Backwards count incorrect" << std::endl;

        for (uint32_t j = 0; j != i; j++)
            checkResult(&tree, inserted_vals[j], mapKey(inserted_vals[j]));

        if (numKeysTree == savePoint)
            tree.saveTreeAsJson(basePath + "tree_rand_"+ std::to_string(seed) + "_at" + std::to_string(numKeysTree) + ".json");

        if (!tree.verifyOrder())
            std::cout << "Tree out of order" << std::endl;

    }
    return 0;
}

int main() {
    std::string basePath = "/home/sebas/dev/Uni/master_thesis/csbplustree/visual/trees/";
    uint32_t maxKeys = 10000;
    uint32_t savePoint = 100;
    uint32_t randomCnt = 5;

    for (uint32_t i = 0; i < randomCnt; i++)
        insert_rand(maxKeys, savePoint, basePath, std::chrono::system_clock::now().time_since_epoch().count());
    insert_decr(maxKeys, savePoint, basePath);
    insert_incr(maxKeys, savePoint, basePath);

    return 0;
}
