#include <stdio.h>
#include "csbplustree.h"

#include "ChunkrefMemoryHandler.h"


using namespace ChunkRefMemoryHandler;

int main() {

    MemoryHandler_t<1024, 64> memoryHandler = MemoryHandler_t<1024, 64>();

    byte  *someAddr, *someAddr2, *someAddr3, *someAddr4, *someAddr5, *someAddr6, *someAddr7;
    someAddr = memoryHandler.getMem(128);
    someAddr2 = memoryHandler.getMem(128);

    if (someAddr + 128 != someAddr2){
        printf("Error 1\n");
    }

    memoryHandler.release(someAddr, 128);
    someAddr = memoryHandler.getMem(192);


    if (someAddr2 + 128 != someAddr){
        printf("Error 2\n");
    }

    someAddr3 = memoryHandler.getMem(64);
    someAddr4 = memoryHandler.getMem(32);

    if (someAddr3 + 64 != someAddr4 && someAddr3 + 128 != someAddr2){
        printf("Error 3\n");
    }


    someAddr5 = memoryHandler.getMem(900);
    someAddr6 = memoryHandler.getMem(64);


    if (someAddr5 + 960 != someAddr6){
        printf("Error 4\n");
    }

    memoryHandler.release(someAddr5, 900);

    someAddr7 = memoryHandler.getMem(830);

    if (someAddr7 != someAddr5){
        printf("Error 5\n");
    }


    return 0;
}