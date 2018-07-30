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
using CsbTree_t_64_64_1 = CsbTree_t<uint64_t, uint64_t, 1>;

using CsbTree_t_32_64_2 = CsbTree_t<uint32_t, uint64_t, 2>;
using CsbTree_t_64_64_2 = CsbTree_t<uint64_t, uint64_t, 2>;

using CsbTree_t_32_64_3 = CsbTree_t<uint32_t, uint64_t, 3>;
using CsbTree_t_64_64_3 = CsbTree_t<uint64_t, uint64_t, 3>;

using CsbTree_t_32_64_4 = CsbTree_t<uint32_t, uint64_t, 4>;
using CsbTree_t_64_64_4 = CsbTree_t<uint64_t, uint64_t, 4>;

using CsbTree_t_32_64_5 = CsbTree_t<uint32_t, uint64_t, 5>;
using CsbTree_t_64_64_5 = CsbTree_t<uint64_t, uint64_t, 5>;


using CsbTree_t_32_64_6 = CsbTree_t<uint32_t, uint64_t, 6>;
using CsbTree_t_64_64_6 = CsbTree_t<uint64_t, uint64_t, 6>;


using CsbTree_t_32_64_7 = CsbTree_t<uint32_t, uint64_t, 7>;
using CsbTree_t_64_64_7 = CsbTree_t<uint64_t, uint64_t, 7>;

using CsbTree_t_32_64_8 = CsbTree_t<uint32_t, uint64_t, 8>;
using CsbTree_t_64_64_8 = CsbTree_t<uint64_t, uint64_t, 8>;


bool aVerify;
uint32_t aNumIterations;


template<class IdxStruc_t>
void run_test_and_write_result(std::string const aName, uint32_t const aCacheLines, const TestConfig_tt& aConf, CsvWriter_t* aCsvWriter){
    PerfTest_t<IdxStruc_t> lPTest(aConf, aNumIterations);
    TestResult_tt lResult = {};
    lResult._cacheLines = aCacheLines;
    lResult._name = aName;

    if (lPTest.run(lResult, aVerify)){
        lResult._status = "OK";
    }else{
        lResult._status = "ERROR";
    }

    aCsvWriter->flushLine(aConf, lResult);
}


int run_config_csb(const TestConfig_tt& aConf, CsvWriter_t* aCsvWriter){

    run_test_and_write_result<CsbTree_t_32_64_1>("CsbTree_t", 1, aConf, aCsvWriter);
    run_test_and_write_result<CsbTree_t_32_64_2>("CsbTree_t", 2, aConf, aCsvWriter);
    run_test_and_write_result<CsbTree_t_32_64_3>("CsbTree_t", 3, aConf, aCsvWriter);
    run_test_and_write_result<CsbTree_t_32_64_4>("CsbTree_t", 4, aConf, aCsvWriter);
    run_test_and_write_result<CsbTree_t_32_64_5>("CsbTree_t", 5, aConf, aCsvWriter);
    run_test_and_write_result<CsbTree_t_32_64_6>("CsbTree_t", 6, aConf, aCsvWriter);
    run_test_and_write_result<CsbTree_t_32_64_7>("CsbTree_t", 7, aConf, aCsvWriter);
    run_test_and_write_result<CsbTree_t_32_64_8>("CsbTree_t", 8, aConf, aCsvWriter);

    run_test_and_write_result<CsbTree_t_64_64_1>("CsbTree_t", 1, aConf, aCsvWriter);
    run_test_and_write_result<CsbTree_t_64_64_2>("CsbTree_t", 2, aConf, aCsvWriter);
    run_test_and_write_result<CsbTree_t_64_64_3>("CsbTree_t", 3, aConf, aCsvWriter);
    run_test_and_write_result<CsbTree_t_64_64_4>("CsbTree_t", 4, aConf, aCsvWriter);
    run_test_and_write_result<CsbTree_t_64_64_5>("CsbTree_t", 5, aConf, aCsvWriter);
    run_test_and_write_result<CsbTree_t_64_64_6>("CsbTree_t", 6, aConf, aCsvWriter);
    run_test_and_write_result<CsbTree_t_64_64_7>("CsbTree_t", 7, aConf, aCsvWriter);
    run_test_and_write_result<CsbTree_t_64_64_8>("CsbTree_t", 8, aConf, aCsvWriter);



    return 0;
}

int run_config_csb_single(const TestConfig_tt& aConf, CsvWriter_t* aCsvWriter) {

    run_test_and_write_result<CsbTree_t_64_64_3>("CsbTree_t", 3, aConf, aCsvWriter);
    return 0;
}

int run_config_other(const TestConfig_tt& aConf, CsvWriter_t* aCsvWriter) {
    run_test_and_write_result<std::map<uint64_t , uint64_t >>("map", 0, aConf, aCsvWriter);
    run_test_and_write_result<ArtWrapper_t<uint64_t , uint64_t >>("art", 0, aConf, aCsvWriter);
    run_test_and_write_result<std::map<uint32_t , uint64_t >>("map", 0, aConf, aCsvWriter);
    run_test_and_write_result<ArtWrapper_t<uint32_t , uint64_t >>("art", 0, aConf, aCsvWriter);

    return 0;
}



int main(int argc, char *argv[]) {


    if (argc < 6) {
        cout
                << "Usage:" << endl
                << argv[0] << " <mode (all|csb|csbsingle)> <csvpath> <numKeys> <numIterations> <verify (0|1)>" << endl;
        return 1;
    }

    CsvWriter_t *writer = new CsvWriter_t(argv[2]);
    uint32_t numKeys = std::stoi(argv[3]);
    aNumIterations = std::stoi(argv[4]);
    aVerify = std::stoi(argv[5]);
    std::string mode = argv[1];




    TestConfig_tt conf {
            0,            // _numKeysToPreinsert
            "incr",           // _insertMethod
            numKeys,            // _numKeysToInsert
            "existing",          // lookup method
            numKeys,            // _numKeysToLookup
    };

    if (mode == "all") {
        run_config_csb(conf, writer);
        run_config_other(conf, writer);
        conf._insertMethod = "decr";
        run_config_csb(conf, writer);
        run_config_other(conf, writer);
    }
    else if (mode == "csb") run_config_csb(conf, writer);
    else if ( mode == "csbsingle") run_config_csb_single(conf, writer);
    else cout << mode << " is not a valid mode" << endl;
}