#include <iostream>
#include "../lib/csbplustree.h"


int main() {

    CsbTree_t<uint32_t , uint32_t, 1>* tree = new CsbTree_t<uint32_t , uint32_t, 1>();

    for (uint32_t i= 10000; i !=0; i--){
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