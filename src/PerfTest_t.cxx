#include <cstdint>
#include <random>
#include <chrono>
#include <iostream>


template <class IndexStruc_t, class Key_t, class Tid_t>
void
PerfTest_t<IndexStruc_t, Key_t, Tid_t>::
insertK(uint64_t k){
    for (uint64_t i = numKeysInserted_; i < numKeysInserted_ + k; i++){
        idxStr_.insert(keyTid_[i]);
    }
    numKeysInserted_ += k;
}

template <class IndexStruc_t, class Key_t, class Tid_t>
void
PerfTest_t<IndexStruc_t, Key_t, Tid_t>::
findK(uint64_t k){
    if (k > numKeysInserted_) k = numKeysInserted_;
    for (uint64_t i = 0; i < k; i++) {
        tidFound_[i] = idxStr_[keyTid_[i].first];
    }
    numKeysRead_ += k;
}

template <class IndexStruc_t, class Key_t, class Tid_t>
void
PerfTest_t<IndexStruc_t, Key_t, Tid_t>::
genKeysAndTids(){
    uint64_t lSeed = std::chrono::system_clock::now().time_since_epoch().count();
    std::minstd_rand0 lRandGen(lSeed);
    for (uint64_t i= 0; i< config_._numKeysToPreinsert + config_._numKeysToInsert; i++){
        keyTid_[i].first = lRandGen();
        keyTid_[i].second = lRandGen();
    }
}



template <class IndexStruc_t, class Key_t, class Tid_t>
void
PerfTest_t<IndexStruc_t, Key_t, Tid_t>::
prefill(){
    insertK(config_._numKeysToPreinsert);
}

template <class IndexStruc_t, class Key_t, class Tid_t>
double_t
PerfTest_t<IndexStruc_t, Key_t, Tid_t>::
measure(MeasureFuncPt fToMeasure, uint64_t k){
    using namespace std::chrono;
    time_point begin = high_resolution_clock::now();
    (this->*fToMeasure)(k);
    time_point end = high_resolution_clock::now();
    duration<double_t> elapsed = end - begin;
    return elapsed.count();
}



template <class IndexStruc_t, class Key_t, class Tid_t>
PerfTest_t<IndexStruc_t, Key_t, Tid_t>::
PerfTest_t(const TestConfig_tt& aConfig) : config_(aConfig), numKeysInserted_(0), numKeysRead_(0) {
    keyTid_ = new pair<Key_t, Tid_t>[config_._numKeysToPreinsert + config_._numKeysToInsert];
    tidFound_ = new Tid_t[config_._numKeysToPreinsert + config_._numKeysToInsert];
    genKeysAndTids();
};



template <class IndexStruc_t, class Key_t, class Tid_t>
bool
PerfTest_t<IndexStruc_t, Key_t, Tid_t>::
verifyAllRead(){
    for (uint64_t i; i < numKeysInserted_; ++i){
        if (keyTid_[i].second != tidFound_[i]) return false;
    }
    return true;
}

template <class IndexStruc_t, class Key_t, class Tid_t>
bool
PerfTest_t<IndexStruc_t, Key_t, Tid_t>::
run(TestResult_tt& aResult){
    using namespace std;

    cout << "prefilling..." << endl;
    prefill();

    cout << "measuring insert..." << endl;
    aResult._measuredInsertedKeys = measure(
            &ThisPerfTest_t::insertK,
            config_._numKeysToInsert
    );


    cout << "measuring lookup" << endl;
    aResult._measuredLookupKeys = measure(
            &ThisPerfTest_t::findK,
            config_._numKeysToInsert
    );

    cout << "verifying read values" << endl;
    cout << "done" << endl;

    aResult._sizeKeyT = sizeof(Key_t);
    aResult._sizeTidT = sizeof(Tid_t);
    return true;

}