#include <iostream>
#include "csbplustree.h"



int main() {

    CsbTree<uint32_t , uint32_t, 1>* tree = new CsbTree<uint32_t , uint32_t, 1>();

    std::cout << tree->total_leaf_node_size << std::endl;
    std::cout << tree->num_cachelines_leaf_node << std::endl;
    std::cout << tree->total_inner_node_size<<std::endl;


    for (uint32_t i= 10000; i !=0; i--){
        tree->insert(i, i*10000);
    }
    for (uint32_t i= 0; i<= 10000; i++){
        std::cout << tree->find(i) << std::endl;
    }

    return 0;

}