#define BOOST_TEST_MODULE ChunkrefMemHandler

#include <boost/test/unit_test.hpp>
#include "../lib/ChunkrefMemoryHandler.h"

BOOST_AUTO_TEST_SUITE(MemoryHandler)

    BOOST_AUTO_TEST_CASE(BestFit) {

            using BestFitMemHandler_t = ChunkRefMemoryHandler::MemoryHandler_t<12800, 64, 1>;
            BestFitMemHandler_t memoryHandler = BestFitMemHandler_t();


            std::vector<byte *> lMemAssignments;

            lMemAssignments.push_back(memoryHandler.getMem(64));
            lMemAssignments.push_back(memoryHandler.getMem(64));
            lMemAssignments.push_back(memoryHandler.getMem(64));
            lMemAssignments.push_back(memoryHandler.getMem(128));
            lMemAssignments.push_back(memoryHandler.getMem(64));
            lMemAssignments.push_back(memoryHandler.getMem(128));

            byte *lBeginChunk0 = lMemAssignments[0];
            byte *lEndChunk0 = lBeginChunk0 + 12800;

            BOOST_CHECK_EQUAL(
              memoryHandler.getBytesAllocatedPerChunk()->at(0),
              512
            );

            BOOST_CHECK_EQUAL(lMemAssignments[1] - 64, lMemAssignments[0]);
            BOOST_CHECK_EQUAL(lMemAssignments[4] - 128, lMemAssignments[3]);

                /*
                * memory_chunk: 12800 Bytes
                *      [64u,64u,64u,128u,64u,128u,12288f]
                */

            byte *lInNewChunk = memoryHandler.getMem(12288 + 128); // allocate one node more than the chunk having space
            BOOST_CHECK_EQUAL(
                    memoryHandler.getBytesAllocatedPerChunk()->at(1),
                    (12288 / 128 + 1) * 128
            );


                /*
                * memory_chunk: 12800 Bytes
                * [64u,64u,64u,128u,64u,128u,12288f]
                *
                * memory_chunk: 12800 Bytes
                * [12416u,384f]
                */

            BOOST_CHECK(!(lBeginChunk0<=lInNewChunk && lInNewChunk<lEndChunk0));

            // release some memory
            memoryHandler.release(lMemAssignments[1], 64);
            memoryHandler.release(lMemAssignments[2], 64);
            memoryHandler.release(lMemAssignments[4], 64);

            BOOST_CHECK_EQUAL(
                    memoryHandler.getBytesAllocatedPerChunk()->at(0),
                    512-3*64
            );
                /*
                * memory_chunk: 12800 Bytes
                * [64u,128f,128u,64f,128u,12288f]
                *
                * memory_chunk: 12800 Bytes
                * [12416u,384f]
                */
            // after memory has been returned, a new memory request should return the old pointer

            byte *lRealloacted64B = memoryHandler.getMem(64);
            BOOST_CHECK_EQUAL(lRealloacted64B, lMemAssignments[4]);
            BOOST_CHECK_EQUAL(
                    memoryHandler.getBytesAllocatedPerChunk()->at(0),
                    512-2*64
            );

                /*
                * memory_chunk: 12800 Bytes
                * [64u,128f,128u,64u,128u,12288f]
                *
                * memory_chunk: 12800 Bytes
                * [12416u,384f]
                */

            //allocate a 192B chunk after clearing 64B one
            memoryHandler.release(lMemAssignments[3], 64);
            byte *lRealloacted192B = memoryHandler.getMem(192);
            BOOST_CHECK_EQUAL(
                    memoryHandler.getBytesAllocatedPerChunk()->at(0),
                    512-3*64+192
            );

                /*
                * memory_chunk: 12800 Bytes
                * [64u,192u,64f,64u,128u,12288f]
                *
                * memory_chunk: 12800 Bytes
                * [12416u,384f]
                */
            BOOST_CHECK_EQUAL(lRealloacted192B, lMemAssignments[1]);


    }

    BOOST_AUTO_TEST_CASE(FirstFit) {
        using FirstFitMemHandler_t = ChunkRefMemoryHandler::MemoryHandler_t<12800, 64, 0>;
        FirstFitMemHandler_t memoryHandler = FirstFitMemHandler_t();

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

    }
    BOOST_AUTO_TEST_SUITE_END()
