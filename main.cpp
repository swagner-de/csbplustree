#include <iostream>
#include "csbplustree.h"



int main() {

    CsbTree<uint32_t , uint32_t, 5>* tree = new CsbTree<uint32_t , uint32_t, 5>();

    tree->insert(1, 3622);
    tree->insert(2, 44654564);

    std::cout
            << tree->find(2) << std::endl
            << tree->find(1);

    return 0;

}