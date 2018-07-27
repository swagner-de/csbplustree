#ifndef CSBPLUSTREE_CSVWRITER_T_H
#define CSBPLUSTREE_CSVWRITER_T_H

#include <cstdint>
#include <string>
#include <fstream>

#include "TestResult_tt.h"
#include "TestConfig_tt.h"

using std::fstream;

static const string headerFields_[] = {
        "preInsertedKeys",
        "insertMethod",
        "numInsertedKeys",
        "measuredInsertedKeys",
        "lookupMethod",
        "numLookupKey",
        "measuredLookupKeys",
        "name",
        "cacheLines",
        "KeyT",
        "TidT",
        "status"
};

static const uint32_t lenHeaderFields_ = 11;

class CsvWriter_t {
private:
    fstream fileStream_;


    bool fileExists(const string &filename);

    void insertHeaderRow();


public:
    CsvWriter_t(const string &aFilePath);

    ~CsvWriter_t();

    void flushLine(const TestConfig_tt &aTestConfig, TestResult_tt &aPerfResult);
};

#endif //CSBPLUSTREE_CSVWRITER_T_H
