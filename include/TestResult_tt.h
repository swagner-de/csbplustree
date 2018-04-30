
#ifndef CSBPLUSTREE_TESTRESULT_TT_H
#define CSBPLUSTREE_TESTRESULT_TT_H

#include <cstdint>
#include <math.h>
#include <string>

struct TestResult_tt{
    double_t        _measuredInsertedKeys;
    double_t        _measuredLookupKeys;
    uint64_t        _sizeKeyT;
    uint64_t        _sizeTidT;
    std::string     _name;
    std::string     _status;
};



#endif //CSBPLUSTREE_TESTRESULT_TT_H
