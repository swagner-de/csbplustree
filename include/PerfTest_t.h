#ifndef CSBPLUSTREE_PERFTEST_T_H
#define CSBPLUSTREE_PERFTEST_T_H

#include <cstdint>
#include <string>
#include <math.h>

#include "TestResult_tt.h"
#include "TestConfig_tt.h"

using std::string;
using std::pair;

template <class IndexStruc_t>
class PerfTest_t {
private:

    using ThisPerfTest_t = PerfTest_t<IndexStruc_t>;
    using MeasureFuncPt = void (ThisPerfTest_t::*)(uint64_t);
    using it = typename IndexStruc_t::iterator;
    using Key_t = typename IndexStruc_t::key_type;
    using Tid_t = typename IndexStruc_t::mapped_type;


    IndexStruc_t idxStr_;
    const TestConfig_tt &config_;
    uint64_t numKeysInserted_;
    uint64_t numKeysRead_;
    pair<Key_t, Tid_t> *keyTid_;
    Tid_t *tidFound_;

    void insertK(uint64_t k);

    void findK(uint64_t k);

    void genKeysAndTids();

    void inline prefill();

    double_t measure(MeasureFuncPt fToMeasure, uint64_t k);


public:
    static constexpr uint64_t _SEED_KEYS =  15831164130173156251;
    static constexpr uint64_t _SEED_TIDS =  1167110519021915224;

    PerfTest_t(const TestConfig_tt &aConfig);

    bool verifyAllRead();

    bool run(TestResult_tt &aResult);
};


#include "../src/PerfTest_t.cxx"

#endif //CSBPLUSTREE_PERFTEST_T_H

