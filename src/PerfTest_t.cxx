#include <cstdint>
#include <random>
#include <chrono>
#include <iostream>


template <class IndexStruc_t>
void
PerfTest_t<IndexStruc_t>::
insertK(uint64_t k){
    for (uint64_t i = numKeysInserted_; i < numKeysInserted_ + k; i++){
        idxStr_.insert(keyTid_[i]);
    }
    numKeysInserted_ += k;
}

template <class IndexStruc_t>
void
PerfTest_t<IndexStruc_t>::
findK(uint64_t k){
    if (k > numKeysInserted_) k = numKeysInserted_;
    for (uint64_t i = 0; i < k; i++) {
        it lRes = idxStr_.find(keyTid_[i].first);
        tidFound_[i] = lRes->second;
    }
    numKeysRead_ = k;
}

template <class IndexStruc_t>
void
PerfTest_t<IndexStruc_t>::
genKeysAndTids(){
    std::minstd_rand0 lRandGenKey(_SEED_KEYS);
    std::minstd_rand0 lRandGenTid(_SEED_TIDS);
    for (uint64_t i= 0; i< config_._numKeysToPreinsert + config_._numKeysToInsert; i++){
        keyTid_[i].first = lRandGenKey();
        keyTid_[i].second = lRandGenTid();
    }
}

template <class IndexStruc_t>
void
PerfTest_t<IndexStruc_t>::
prefill(){
    insertK(config_._numKeysToPreinsert);
}

template <class IndexStruc_t>
double_t
PerfTest_t<IndexStruc_t>::
measure(MeasureFuncPt fToMeasure, uint64_t k){
    using namespace std::chrono;
    time_point<high_resolution_clock> begin = high_resolution_clock::now();
    (this->*fToMeasure)(k);
    time_point<high_resolution_clock> end = high_resolution_clock::now();
    duration<double_t, std::nano> elapsed(end - begin);
    return elapsed.count();
}



template <class IndexStruc_t>
PerfTest_t<IndexStruc_t>::
PerfTest_t(const TestConfig_tt& aConfig):
        idxStr_(), config_(aConfig), numKeysInserted_(0), numKeysRead_(0),
        keyTid_(new pair<Key_t, Tid_t>[config_._numKeysToPreinsert + config_._numKeysToInsert]),
        tidFound_(new Tid_t[config_._numKeysToPreinsert + config_._numKeysToInsert]) {
    genKeysAndTids();
};

template <class IndexStruc_t>
PerfTest_t<IndexStruc_t>::
~PerfTest_t(){
    delete[] keyTid_;
    delete[] tidFound_;
}


template <class IndexStruc_t>
bool
PerfTest_t<IndexStruc_t>::
verifyAllRead(){
    for (uint64_t i=0; i < numKeysRead_; ++i){
        if (keyTid_[i].second != tidFound_[i]){
            return false;
        };
    }
    return true;
}

template <class IndexStruc_t>
bool
PerfTest_t<IndexStruc_t>::
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
    if(!verifyAllRead()) return false;
    cout << "done" << endl;

    aResult._sizeKeyT = sizeof(Key_t);
    aResult._sizeTidT = sizeof(Tid_t);
    return true;

}