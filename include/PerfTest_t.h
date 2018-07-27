#ifndef CSBPLUSTREE_PERFTEST_T_H
#define CSBPLUSTREE_PERFTEST_T_H

#include <cstdint>
#include <string>
#include <math.h>

#include "TestResult_tt.h"
#include "TestConfig_tt.h"

#define _SEED_KEYS 1528311651u
#define _SEED_TIDS 1167121471u

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


    IndexStruc_t * idxStr_;
    const TestConfig_tt &config_;
    const uint32_t numIterations_;
    uint64_t numKeysInserted_;
    uint64_t numKeysRead_;
    pair<Key_t, Tid_t>* keyTid_;
    Tid_t *tidFound_;

    void insertK(uint64_t const k);

    void findK(uint64_t k);

    void genKeysAndTids();

    void inline prefill();

    double_t measure(MeasureFuncPt fToMeasure, uint64_t k);

    static double_t average(double_t const * const aVec, uint32_t aLenVec);


public:

    PerfTest_t(const TestConfig_tt &aConfig, uint32_t const aNumIterations);
    PerfTest_t(const PerfTest_t&) = default;
    PerfTest_t& operator=(const PerfTest_t&) = default;
    ~PerfTest_t();

    bool verifyAllRead() const;

    bool run(TestResult_tt &aResult, bool);
};


#include "../src/PerfTest_t.cxx"

#endif //CSBPLUSTREE_PERFTEST_T_H

