#ifndef CSBPLUSTREE_CSBPLUSTREE_H
#define CSBPLUSTREE_CSBPLUSTREE_H

#include "ChunkrefMemoryHandler.h"
#include <cstddef>
#include <stack>


typedef typename std::byte byte;
template <class Key_t, class Tid_t, uint16_t kNumCacheLinesPerNode>
class CsbTree_t{
    private:
    static constexpr uint16_t       kSizeCacheLine = 64;
    static constexpr uint32_t       kSizeNode = kSizeCacheLine * kNumCacheLinesPerNode;


    static constexpr uint16_t       kSizeFixedInnerNode = 10;
    static constexpr uint16_t       kNumMaxKeysInnerNode = (kSizeNode - kSizeFixedInnerNode) / sizeof(Key_t);
    static constexpr uint16_t       kSizePaddingInnerNode = kSizeNode - (kNumMaxKeysInnerNode * sizeof(Key_t) + kSizeFixedInnerNode);

    static constexpr uint16_t       kSizeFixedLeafNode = 2;
    static constexpr uint16_t       kNumMaxKeysLeafNode = (kSizeNode - kSizeFixedLeafNode) / (sizeof(Key_t) + sizeof(Tid_t));
    static constexpr uint16_t       kSizePaddingLeafNode = kSizeNode - (kNumMaxKeysLeafNode * (sizeof(Tid_t) + sizeof(Key_t)) + kSizeFixedLeafNode);

    static constexpr uint16_t       kSizeFixedLeafEdgeNode = 18;
    static constexpr uint16_t       kNumMaxKeysLeafEdgeNode = (kSizeNode - kSizeFixedLeafEdgeNode) / (sizeof(Key_t) + sizeof(Tid_t));
    static constexpr uint16_t       kSizePaddingLeafEdgeNode = kSizeNode - (kNumMaxKeysLeafEdgeNode * (sizeof(Tid_t) + sizeof(Key_t)) + kSizeFixedLeafEdgeNode);

    static constexpr uint32_t       kSizeMemoryChunk = kSizeNode * kNumMaxKeysInnerNode * 10000;

    typedef typename ChunkRefMemoryHandler::MemoryHandler_t<kSizeMemoryChunk, kSizeCacheLine, 1> TreeMemoryManager_t;

    byte* root_;
    TreeMemoryManager_t* tmm_;
    uint32_t depth_;

public:

    struct iterator_tt {
        Tid_t   second;
    };

    using iterator = iterator_tt*;
    using key_type = Key_t;
    using mapped_type = Tid_t;


    CsbTree_t();
    ~CsbTree_t();


    class CsbInnerNode_t{
    public:
        Key_t       keys_       [kNumMaxKeysInnerNode];
        uint16_t    numKeys_;
        byte*       children_;
        uint8_t     free_       [kSizePaddingInnerNode];  // padding

        CsbInnerNode_t();
        inline Key_t getLargestKey();
        void shift(uint16_t aIdx);
        void allocateChildNodes(TreeMemoryManager_t* aTmm);
        void moveKeysAndChildren(CsbInnerNode_t* aNodeTarget, uint16_t aNumKeysRemaining);
        inline void setStopMarker();
        void remove(uint16_t aIdxToRemove);
        inline uint16_t getChildNodeIdx(byte* aChildNode);
        inline uint16_t getChildMaxKeys(uint32_t aChildDepth, uint32_t aTreeDepth, byte* aChild);
        inline uint16_t getChildNumKeys(uint32_t aChildDepth, uint32_t aTreeDepth, byte* aChild);
        inline uint16_t getChildNumKeys(bool aIsLeaf, byte* aChild);
        inline bool childIsEdge(byte* aChildNode);
        inline bool childIsFull(uint32_t aChildDepth, uint32_t aTreeDepth, byte* aChildNode);
        std::string asJson(uint32_t aNodeDepth, uint32_t aTreeDepth);

    } __attribute__((packed));


    class CsbLeafNode_t {
    public:
        Key_t           keys_       [kNumMaxKeysLeafNode];
        Tid_t           tids_       [kNumMaxKeysLeafNode];
        uint16_t        numKeys_;
        uint8_t         free_       [kSizePaddingLeafNode];   // padding

        CsbLeafNode_t();
        inline Key_t getLargestKey();
        void insert(Key_t aKey, Tid_t aTid);
        void moveKeysAndTids(CsbLeafNode_t* aNodeTarget, uint16_t aNumKeysRemaining);
        void leftInsertKeysAndTids(CsbLeafNode_t* aNodeSource, uint16_t aNumKeys);
        void remove(Key_t key, Tid_t tid);
        void toLeafEdge();
        std::string asJson();

    } __attribute__((packed));

    class CsbLeafEdgeNode_t {
    public:
        Key_t               keys_       [kNumMaxKeysLeafEdgeNode];
        Tid_t               tids_       [kNumMaxKeysLeafEdgeNode];
        uint16_t            numKeys_;
        CsbLeafEdgeNode_t*  previous_;
        CsbLeafEdgeNode_t*  following_;
        uint8_t             free_       [kSizePaddingLeafEdgeNode];   // padding

        CsbLeafEdgeNode_t();
        inline Key_t getLargestKey();
        void insert(Key_t aKey, Tid_t aTid);
        void moveKeysAndTids(CsbLeafNode_t* aNodeTarget, uint16_t aNumKeysRemaining);
        void remove(Key_t key, Tid_t tid);
        std::string asJson();

    } __attribute__((packed));


    struct SearchResult_tt {
        byte*       _node;
        uint16_t    _idx;
    };


    class TooManyKeysException : public std::exception {
        virtual const char *what() const throw() {
            return "The node has to many keys to perform the operation";
        }
    };


    const uint16_t getCacheLinesPerNode();
    inline Tid_t* findRef(Key_t);
    inline iterator find(Key_t);
    inline int32_t find(Key_t aKey, Tid_t* aResult);
    inline Tid_t operator [] (Key_t aKey);
    void inline insert(Key_t aKey, Tid_t aTid);
    void insert(std::pair<Key_t, Tid_t>);
    void remove(Key_t aKey, Tid_t aTid);
    void saveTreeAsJson(std::string aPath);
    void getMemoryUsage();
    uint64_t countNodes();

    uint64_t getNumKeys();
    uint64_t getNumKeysBackwards();
    bool verifyOrder();

private:

    struct SplitResult_tt{
        byte*       _left;
        uint16_t    _splitIdx;
        bool        _edgeIndicator;
    };

    void split(byte* aNodeToSplit, uint32_t aDepth, std::stack<CsbInnerNode_t*>* aPath, SplitResult_tt* aResult);
    inline bool isLeaf(uint32_t aDepth);
    std::string getTreeAsJson();
    void findLeafNode(Key_t aKey, SearchResult_tt* aResult);
    void findLeafForInsert(Key_t aKey, SearchResult_tt* aResult, std::stack<CsbInnerNode_t*>* aPath);
    uint64_t countNodes(CsbInnerNode_t* aNode, uint32_t aDepth);

    static std::string kThChildrenAsJson(uint32_t aK, byte* aFirstChild, uint32_t aNodeDepth, uint32_t aTreeDepth);
    static Key_t getLargestKey(byte* aNode, bool aChild, bool aEdge);
    static byte* getKthNode(uint16_t aK, byte* aNodeFirstChild);
    static uint16_t idxToDescend(Key_t aKey, Key_t aKeys[], uint16_t aNumKeys);
    static uint16_t idxToInsert(Key_t aKey, Key_t aKeys[], uint16_t aNumKeys);

};

#include "../src/csbplustree.cxx"

#endif //CSBPLUSTREE_CSBPLUSTREE_H