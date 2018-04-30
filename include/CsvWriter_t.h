#ifndef CSBPLUSTREE_CSVWRITER_T_H
#define CSBPLUSTREE_CSVWRITER_T_H

#include <cstdint>
#include <string>
#include <fstream>

#include "TestResult_tt.h"
#include "TestConfig_tt.h"

using std::fstream;


class CsvWriter_t {
private:
    fstream fileStream_;
    const string *headerFields_;
    const uint32_t lenHeaderFields_;

    bool fileExists(const string &filename);

    void insertHeaderRow();


public:
    CsvWriter_t(const string &aFilePath, const string *aHeaderFields, const uint32_t aLenHeaderFields);

    ~CsvWriter_t();

    void flushLine(const TestConfig_tt &aTestConfig, TestResult_tt &aPerfResult);
};

#endif //CSBPLUSTREE_CSVWRITER_T_H
