#define BOOST_TEST_MODULE ChunkrefMemHandler

#include <boost/test/unit_test.hpp>
#include "../include/ChunkrefMemoryHandler.h"

BOOST_AUTO_TEST_SUITE(MemoryHandler)

    BOOST_AUTO_TEST_CASE(Allocate) {
        struct TestStruct_tt{
            uint64_t numbers[8];
        };

        constexpr uint32_t sizeTestStruct_tt = sizeof(TestStruct_tt);
        constexpr uint32_t kNumItemsChunk = 1024;

        using ThisMemoryHandler = ChunkRefMemoryHandler::MemoryHandler_t<sizeTestStruct_tt * kNumItemsChunk, 64, sizeTestStruct_tt>;

        ThisMemoryHandler handler;


        // allocate in 1024 TestStuct_tt
        for (uint32_t i=0; i < kNumItemsChunk; i++){
            BOOST_CHECK_EQUAL(handler.getMem() + sizeTestStruct_tt, handler.getMem());
            BOOST_CHECK_EQUAL(handler.getBytesAllocatedPerChunk()->at(0), 0);
        }


        byte** addresses = new byte*[4];
        addresses[0] = handler.getMem();
        addresses[1] = handler.getMem();
        addresses[2] = handler.getMem();


        handler.release(addresses[1]);
        BOOST_CHECK_EQUAL(handler.getMem(), addresses[1]);

        handler.release(addresses[0]);
        handler.release(addresses[2]);
        BOOST_CHECK_EQUAL(handler.getMem(), addresses[0]);
        BOOST_CHECK_EQUAL(handler.getMem(), addresses[2]);



        // return every second chunk and then reallocate it



    }
    BOOST_AUTO_TEST_SUITE_END()
