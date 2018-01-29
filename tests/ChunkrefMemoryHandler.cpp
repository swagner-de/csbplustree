#define BOOST_TEST_MODULE ChunkrefMemHandler

#include <boost/test/unit_test.hpp>
#include "../lib/ChunkrefMemoryHandler.h"

BOOST_AUTO_TEST_SUITE(MemoryHandler)

    BOOST_AUTO_TEST_CASE(BestFitChunkrefHandler) {

            using BestFitMemHandler_t = ChunkRefMemoryHandler::NodeMemoryManager_t<12800, 64, 1, 64, 128>;
            BestFitMemHandler_t memoryHandler = BestFitMemHandler_t();


            std::vector<byte *> lMemAssignments;

            lMemAssignments.push_back(memoryHandler.getMem(1, 0));
            lMemAssignments.push_back(memoryHandler.getMem(1, 0));
            lMemAssignments.push_back(memoryHandler.getMem(1, 0));
            lMemAssignments.push_back(memoryHandler.getMem(1, 1));
            lMemAssignments.push_back(memoryHandler.getMem(1, 0));
            lMemAssignments.push_back(memoryHandler.getMem(1, 1));

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

            byte *lInNewChunk = memoryHandler.getMem(12288 / 128 + 1,1); // allocate one node more than the chunk having space
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
            memoryHandler.release(lMemAssignments[1], 1, 0);
            memoryHandler.release(lMemAssignments[2], 1, 0);
            memoryHandler.release(lMemAssignments[4], 1, 0);

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

            byte *lRealloacted64B = memoryHandler.getMem(1, 0);
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
            memoryHandler.release(lMemAssignments[3], 1, 0);
            byte *lRealloacted192B = memoryHandler.getMem(3, 0);
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

    BOOST_AUTO_TEST_SUITE_END()
