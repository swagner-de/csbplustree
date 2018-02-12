#include <iostream>
#include "../lib/csbplustree.cxx"


int main() {
    std::string basePath = "/home/sebas/dev/Uni/master_thesis/csbplustree/visual/trees/";
    uint32_t savePoint = 9915;


    CsbTree_t<uint32_t , uint32_t, 1>* tree = new CsbTree_t<uint32_t , uint32_t, 1>();

    for (uint32_t i= 10000; i !=0; i--){
        if (i == savePoint) {
            tree->saveTreeAsJson(basePath + std::to_string(i) + "insert_dump.json");
        }
        tree->insert(i, i*10000);
    }
    for (uint32_t i= 0; i<= 10000; i++){
        uint32_t retrieved = tree->find(i);
        if (retrieved != i*10000) {
            std::cout << "Error retrieving value for key " + i << std::endl;
        }
    }

    return 0;
}