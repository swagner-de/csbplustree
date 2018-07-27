#include <sys/stat.h>
#include <cstring>
#include <iostream>

#include "../include/CsvWriter_t.h"

using std::string;
using std::cout;
using std::endl;

// from https://stackoverflow.com/questions/4316442/stdofstream-check-if-file-exists-before-writing
bool
CsvWriter_t::fileExists(const string &filename) {
    struct stat buf;
    if (stat(filename.c_str(), &buf) != -1){
        return true;
    }
    return false;
}


void
CsvWriter_t::insertHeaderRow(){
    for (uint32_t i= 0; i<lenHeaderFields_; ++i){
        fileStream_ << headerFields_[i];
        if (i + 1 != lenHeaderFields_){
            fileStream_ << ",";
        }
    }
    fileStream_ << endl;
}

CsvWriter_t::CsvWriter_t(const string& aFilePath) : fileStream_() {
    if (fileExists(aFilePath)){
        fileStream_.open(aFilePath, fstream::app);
    } else {
        fileStream_.open(aFilePath, fstream::app);
        insertHeaderRow();
    }
}

CsvWriter_t::~CsvWriter_t(){
    fileStream_.close();
}

void
CsvWriter_t::flushLine(const TestConfig_tt& aTestConfig, TestResult_tt& aPerfResult){
    fileStream_
            << aTestConfig._numKeysToPreinsert << ","
            << aTestConfig._insertMethod << ","
            << aTestConfig._numKeysToInsert << ","
            << aPerfResult._measuredInsertedKeys << ","
            << aTestConfig._numKeysToLookup << ","
            << aPerfResult._measuredLookupKeys << ","
            << aPerfResult._name << ","
            << aPerfResult._sizeKeyT << ","
            << aPerfResult._sizeTidT << ","
            << aPerfResult._status << endl;
    cout
            << aTestConfig._numKeysToPreinsert << ","
            << aTestConfig._insertMethod << ","
            << aTestConfig._numKeysToInsert << ","
            << aPerfResult._measuredInsertedKeys << ","
            << aTestConfig._numKeysToLookup << ","
            << aPerfResult._measuredLookupKeys << ","
            << aPerfResult._name << ","
            << aPerfResult._sizeKeyT << ","
            << aPerfResult._sizeTidT << ","
            << aPerfResult._status << endl;
}
