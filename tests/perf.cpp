#include <cstdint>
#include <string>
#include <fstream>
#include <sys/stat.h>
#include <random>
#include <chrono>
#include <map>

#include "../include/PerfTest_t.h"
#include "../include/CsvWriter_t.h"
#include "../include/csbplustree.h"
#include "../art_src/ArtWrapper.h"

using std::cout;
using std::cerr;
using std::endl;
using std::map;
using std::fstream;
using std::chrono::duration;

using CsbTree_t_32_64_1 = CsbTree_t<uint32_t, uint64_t, 1>;
using CsbTree_t_32_32_1 = CsbTree_t<uint32_t, uint32_t, 1>;
using CsbTree_t_64_64_1 = CsbTree_t<uint64_t, uint64_t, 1>;

using CsbTree_t_32_64_2 = CsbTree_t<uint32_t, uint64_t, 2>;
using CsbTree_t_32_32_2 = CsbTree_t<uint32_t, uint32_t, 2>;
using CsbTree_t_64_64_2 = CsbTree_t<uint64_t, uint64_t, 2>;

using CsbTree_t_32_64_3 = CsbTree_t<uint32_t, uint64_t, 3>;
using CsbTree_t_32_32_3 = CsbTree_t<uint32_t, uint32_t, 3>;
using CsbTree_t_64_64_3 = CsbTree_t<uint64_t, uint64_t, 3>;

using CsbTree_t_32_64_4 = CsbTree_t<uint32_t, uint64_t, 4>;
using CsbTree_t_32_32_4 = CsbTree_t<uint32_t, uint32_t, 4>;
using CsbTree_t_64_64_4 = CsbTree_t<uint64_t, uint64_t, 4>;


template<class IdxStruc_t>
int run_test_and_write_result(std::string aName, const TestConfig_tt& aConf, CsvWriter_t* aCsvWriter, uint32_t aCount=3){
    int lReturnCode = 0;
    for (uint32_t i=0; i < aCount; i++){
        cout << "Test: " << aName
             << " | key_type: " << sizeof(typename IdxStruc_t::key_type) << " bytes"
             << " | mapped_type: " << sizeof(typename IdxStruc_t::mapped_type) << " bytes"
             << " | iteration: " << i << endl;
        PerfTest_t<IdxStruc_t> lPTest(aConf);
        TestResult_tt lResult = {};
        lResult._name = aName;

        if (!lPTest.run(lResult)) {
            lResult._status = "Error";
            cerr << "Error in Test " << aName << " in iteration " << i << endl;
            lReturnCode = 1;
        } else{
            lResult._status = "OK";
        }
        aCsvWriter->flushLine(aConf, lResult);
    }
    return lReturnCode;
}


int run_config(const TestConfig_tt& aConf, CsvWriter_t* aCsvWriter, uint32_t aAverageCount){

    run_test_and_write_result<CsbTree_t_32_32_1>("CsbTree_t_1", aConf, aCsvWriter, aAverageCount);
    run_test_and_write_result<CsbTree_t_32_32_2>("CsbTree_t_2", aConf, aCsvWriter, aAverageCount);
    run_test_and_write_result<CsbTree_t_32_32_3>("CsbTree_t_3", aConf, aCsvWriter, aAverageCount);
    run_test_and_write_result<CsbTree_t_32_32_3>("CsbTree_t_4", aConf, aCsvWriter, aAverageCount);
    run_test_and_write_result<std::map<uint32_t , uint32_t >>("map", aConf, aCsvWriter, aAverageCount);

    run_test_and_write_result<CsbTree_t_32_64_1>("CsbTree_t_1", aConf, aCsvWriter, aAverageCount);
    run_test_and_write_result<CsbTree_t_32_64_2>("CsbTree_t_2", aConf, aCsvWriter, aAverageCount);
    run_test_and_write_result<CsbTree_t_32_64_3>("CsbTree_t_3", aConf, aCsvWriter, aAverageCount);
    run_test_and_write_result<CsbTree_t_32_64_3>("CsbTree_t_4", aConf, aCsvWriter, aAverageCount);
    run_test_and_write_result<std::map<uint32_t , uint64_t >>("map", aConf, aCsvWriter, aAverageCount);
    run_test_and_write_result<ArtWrapper_t<uint32_t , uint64_t >>("art", aConf, aCsvWriter, aAverageCount);

    run_test_and_write_result<CsbTree_t_64_64_1>("CsbTree_t_1", aConf, aCsvWriter, aAverageCount);
    run_test_and_write_result<CsbTree_t_64_64_2>("CsbTree_t_2", aConf, aCsvWriter, aAverageCount);
    run_test_and_write_result<CsbTree_t_64_64_3>("CsbTree_t_3", aConf, aCsvWriter, aAverageCount);
    run_test_and_write_result<CsbTree_t_64_64_3>("CsbTree_t_4", aConf, aCsvWriter, aAverageCount);
    run_test_and_write_result<std::map<uint64_t , uint64_t >>("map", aConf, aCsvWriter, aAverageCount);
    run_test_and_write_result<ArtWrapper_t<uint64_t , uint64_t >>("art", aConf, aCsvWriter, aAverageCount);


    return 0;
}


int main(int argc, char *argv[]) {


    if (argc < 3) {
        cout
                << "Usage:" << endl
                << argv[0] << " <csvpath> <# iterations>" << endl;
        return 1;
    }

    CsvWriter_t *writer = new CsvWriter_t(argv[1]);
    uint32_t iterations = std::stoi(argv[2]);

    const uint64_t someNum = 10000000;

    const TestConfig_tt conf {
            0,            // _numKeysToPreinsert
            "random",           // _insertMethod
            someNum,            // _numKeysToInsert
            "orderOfInsert",    // _lookupMethod
            someNum,            // _numKeysToLookup
    };

    run_config(conf, writer, iterations);

}