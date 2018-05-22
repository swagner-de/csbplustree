#define BOOST_TEST_MODULE ChunkrefMemHandler

#include <boost/test/unit_test.hpp>
#include "../include/BitsetMemoryHandler.h"

BOOST_AUTO_TEST_SUITE(BitSetMemoryHandler)

    BOOST_AUTO_TEST_CASE(FirstFit) {
            using MemHandler_t = BitSetMemoryHandler::MemoryHandler_t<64, 64, 200>;
            MemHandler_t memoryHandler = MemHandler_t();

        byte* first = memoryHandler.getMem(128);
        byte* previous = first;

        for (uint32_t i = 1; i < 100; i++){
            byte* current = memoryHandler.getMem(128);
            BOOST_CHECK_EQUAL(previous + 128, current);
            previous = current;
        }

        memoryHandler.release(first, 12800);
        BOOST_CHECK_EQUAL(first, memoryHandler.getMem(12800));
        memoryHandler.release(first, 12800);

        byte* first128 = memoryHandler.getMem(128);
        byte* second128 = memoryHandler.getMem(128);
        byte* third128 = memoryHandler.getMem(128);

        BOOST_CHECK(
         first128 +128 == second128 && second128 + 128 == third128
        );

        memoryHandler.release(second128, 128);
        BOOST_CHECK_EQUAL(memoryHandler.getMem(128), second128);
        memoryHandler.release(second128, 128);
        memoryHandler.release(first128, 128);

        BOOST_CHECK_EQUAL(memoryHandler.getMem(128), first128);
        BOOST_CHECK_EQUAL(memoryHandler.getMem(128), second128);
        BOOST_CHECK_EQUAL(memoryHandler.getMem(128), third128 + 128);


        BOOST_CHECK_EQUAL(
                memoryHandler.getBytesAllocatedPerChunk()->at(0),
                512
        );

        byte* remainingMemChunk0 = memoryHandler.getMem(12800 - 512);


        BOOST_CHECK(
                first < remainingMemChunk0 && first + 12800 > remainingMemChunk0
        );

        byte* someMemChunk1 =  memoryHandler.getMem(128);

        BOOST_CHECK(
                first > someMemChunk1 || first + 12800 < someMemChunk1
        );

        byte* largeMemory = memoryHandler.getMem(16384);
        memoryHandler.release(largeMemory, 16384);

    }
    BOOST_AUTO_TEST_SUITE_END()
