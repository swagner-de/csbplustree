#ifndef CSBPLUSTREE_TEXTCONFIG_TT_H
#define CSBPLUSTREE_TEXTCONFIG_TT_H

#include <string>

using std::string;

struct TestConfig_tt{
    uint64_t    _numKeysToPreinsert;
    string      _insertMethod;
    uint64_t    _numKeysToInsert;
    uint64_t    _numKeysToLookup;
};

#endif //CSBPLUSTREE_TEXTCONFIG_TT_H
