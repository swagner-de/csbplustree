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

static constexpr uint32_t kNumIterations = 100;


template<class IdxStruc_t>
int run_test_and_write_result(std::string aName, const TestConfig_tt& aConf, CsvWriter_t* aCsvWriter){
    int lReturnCode = 0;
    cout << "Test: " << aName
         << " | key_type: " << sizeof(typename IdxStruc_t::key_type) << " bytes"
         << " | mapped_type: " << sizeof(typename IdxStruc_t::mapped_type) << " bytes"
         << " | iteration: " << kNumIterations << endl;
    PerfTest_t<IdxStruc_t> lPTest(aConf, kNumIterations);
    TestResult_tt lResult = {};
    lResult._name = aName;

    if (!lPTest.run(lResult, false)) {
        lResult._status = "Error";
        cerr << "Error in Test " << aName << endl;
        lReturnCode = 1;
    } else{
        lResult._status = "OK";
    }

    aCsvWriter->flushLine(aConf, lResult);

    return lReturnCode;
}


int run_config_csb(const TestConfig_tt& aConf, CsvWriter_t* aCsvWriter){

    run_test_and_write_result<CsbTree_t_32_32_1>("CsbTree_t_1", aConf, aCsvWriter);
    run_test_and_write_result<CsbTree_t_32_32_2>("CsbTree_t_2", aConf, aCsvWriter);
    run_test_and_write_result<CsbTree_t_32_32_3>("CsbTree_t_3", aConf, aCsvWriter);
    run_test_and_write_result<CsbTree_t_32_32_3>("CsbTree_t_4", aConf, aCsvWriter);

    run_test_and_write_result<CsbTree_t_32_64_1>("CsbTree_t_1", aConf, aCsvWriter);
    run_test_and_write_result<CsbTree_t_32_64_2>("CsbTree_t_2", aConf, aCsvWriter);
    run_test_and_write_result<CsbTree_t_32_64_3>("CsbTree_t_3", aConf, aCsvWriter);
    run_test_and_write_result<CsbTree_t_32_64_3>("CsbTree_t_4", aConf, aCsvWriter);

    run_test_and_write_result<CsbTree_t_64_64_1>("CsbTree_t_1", aConf, aCsvWriter);
    run_test_and_write_result<CsbTree_t_64_64_2>("CsbTree_t_2", aConf, aCsvWriter);
    run_test_and_write_result<CsbTree_t_64_64_3>("CsbTree_t_3", aConf, aCsvWriter);
    run_test_and_write_result<CsbTree_t_64_64_3>("CsbTree_t_4", aConf, aCsvWriter);


    return 0;
}

int run_config_csb_single(const TestConfig_tt& aConf, CsvWriter_t* aCsvWriter) {

    run_test_and_write_result<CsbTree_t_64_64_3>("CsbTree_t_3", aConf, aCsvWriter);
    return 0;
}

int run_config_other(const TestConfig_tt& aConf, CsvWriter_t* aCsvWriter) {
    run_test_and_write_result<std::map<uint64_t , uint64_t >>("map", aConf, aCsvWriter);
    run_test_and_write_result<ArtWrapper_t<uint64_t , uint64_t >>("art", aConf, aCsvWriter);
    run_test_and_write_result<std::map<uint32_t , uint64_t >>("map", aConf, aCsvWriter);
    run_test_and_write_result<ArtWrapper_t<uint32_t , uint64_t >>("art", aConf, aCsvWriter);
    run_test_and_write_result<std::map<uint32_t , uint32_t >>("map", aConf, aCsvWriter);

    return 0;
}



int main(int argc, char *argv[]) {


    if (argc < 3) {
        cout
                << "Usage:" << endl
                << argv[0] << " <mode (all|csb|csbsingle)> <csvpath> <numKeys>" << endl;
        return 1;
    }

    CsvWriter_t *writer = new CsvWriter_t(argv[2]);
    uint32_t numKeys = std::stoi(argv[3]);


    const TestConfig_tt conf {
            0,            // _numKeysToPreinsert
            "random",           // _insertMethod
            numKeys,            // _numKeysToInsert
            numKeys,            // _numKeysToLookup
    };

    std::string mode = argv[1];

    if (mode == "all") {
        run_config_csb(conf, writer);
        run_config_other(conf, writer);
    }
    else if (mode == "csb") run_config_csb(conf, writer);
    else if ( mode == "csbsingle") run_config_csb_single(conf, writer);
    else cout << mode << " is not a valid mode" << endl;
}