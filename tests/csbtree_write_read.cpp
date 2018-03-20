#include <iostream>
#include <chrono>
#include <random>
#include <vector>
#include "../lib/csbplustree.h"


typedef CsbTree_t<uint32_t , uint64_t, 1> Tree_t;


inline uint64_t mapKey(uint32_t aKey){
    return ((uint64_t) aKey) * 1000;
}

inline void checkResult(Tree_t* aTree, uint32_t aKey, uint64_t aTid){
    uint64_t lRes;
    if (aTree->find(aKey, &lRes) != 0) {
        std::cout << "Key " << aKey << " not found" << std::endl;
        aTree->find(aKey, &lRes);
    } else if (lRes != aTid){
        std::cout << "Value " << lRes << " at lookup of " << aKey << " incorrect" << std::endl;
        aTree->find(aKey, &lRes);
    }
}

int insert_decr(uint32_t maxKeys, uint32_t savePoint, std::string basePath) {

    Tree_t tree = Tree_t();
    uint64_t numKeysTree;

    std::cout << "Inserting from right to left" << std::endl;

    for (uint32_t i = maxKeys; i != 0; i--) {
        tree.insert(i, mapKey(i));
        numKeysTree = tree.getNumKeys();
        if ((maxKeys - i + 1) != numKeysTree)
            std::cout << "#Keys does not match! Was " << numKeysTree << " but should be " << (maxKeys - i + 1) << std::endl;

        if (tree.getNumKeysBackwards() != numKeysTree)
            std::cout << "Backwards count incorrect" << std::endl;

        for (uint64_t j = i; j <= maxKeys; j++)
            checkResult(&tree, j, mapKey(j));


        if (i == savePoint)
            tree.saveTreeAsJson(basePath + "tree1_at" + std::to_string(i) + ".json");

        if (!tree.verifyOrder()) std::cout << "Tree out of order" << std::endl;
    }
    return 0;
}

int insert_incr(uint32_t maxKeys, uint32_t savePoint, std::string basePath) {
    std::cout << "Inserting from left to right" << std::endl;

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

        if (!tree.verifyOrder()) std::cout << "Tree out of order" << std::endl;
    }
    return 0;
}

int insert_rand(uint32_t maxKeys, uint32_t savePoint, std::string basePath,uint64_t seed){
    std::cout << "Inserting randomly with seed " << seed  << std::endl;

    Tree_t tree = Tree_t();

    std::minstd_rand0 random(seed);
    std::vector<uint32_t > inserted_vals;


    uint64_t numKeysTree;

    for (uint32_t i = 0; i != maxKeys; i++) {
        uint32_t  r_val = random();
        inserted_vals.push_back(r_val);
        if (i == 82) {
            std::cout << "";
        }
        tree.insert(r_val, mapKey(r_val));
        numKeysTree = tree.getNumKeys();
        if (i +1 != numKeysTree)
            std::cout << "#Keys does not match! Was " << numKeysTree << " but should be " << (i+1) << std::endl;

        if (tree.getNumKeysBackwards() != numKeysTree)
            std::cout << "Backwards count incorrect" << std::endl;

        for (auto iter = inserted_vals.begin(); iter != inserted_vals.end(); iter++)
            checkResult(&tree, *iter, mapKey(*iter));

        if (!tree.verifyOrder()) std::cout << "Tree out of order" << std::endl;

    }
    return 0;
}

int main() {
    std::string basePath = "/home/sebas/dev/Uni/master_thesis/csbplustree/visual/trees/";
    uint32_t maxKeys = 100000;
    uint32_t savePoint = 9973;
    uint32_t randomCnt = 5;

    for (uint32_t i = 0; i < randomCnt; i++)
        insert_rand(maxKeys, savePoint, basePath, std::chrono::system_clock::now().time_since_epoch().count());
    insert_decr(maxKeys, savePoint, basePath);
    insert_incr(maxKeys, savePoint, basePath);

    return 0;
}
