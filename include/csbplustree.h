#ifndef CSBPLUSTREE_CSBPLUSTREE_H
#define CSBPLUSTREE_CSBPLUSTREE_H


#include <cstddef>
#include <stack>

#include "global.h"
#include "ChunkrefMemoryHandler.h"

template <class Key_t, class Tid_t, uint16_t kNumCacheLinesPerNode>
class CsbTree_t{
    private:
    static constexpr uint32_t       kSizeNode = kSizeCacheLine * kNumCacheLinesPerNode;


    static constexpr uint16_t       kSizeFixedInnerNode = 2 + sizeof(void*);
    static constexpr uint16_t       kNumMaxKeysInnerNode = (kSizeNode - kSizeFixedInnerNode) / sizeof(Key_t);
    static constexpr uint16_t       kSizePaddingInnerNode = kSizeNode - (kNumMaxKeysInnerNode * sizeof(Key_t) + kSizeFixedInnerNode);

    static constexpr uint16_t       kSizeFixedLeafNode = 2;
    static constexpr uint16_t       kNumMaxKeysLeafNode = (kSizeNode - kSizeFixedLeafNode) / (sizeof(Key_t) + sizeof(Tid_t));
    static constexpr uint16_t       kSizePaddingLeafNode = kSizeNode - (kNumMaxKeysLeafNode * (sizeof(Tid_t) + sizeof(Key_t)) + kSizeFixedLeafNode);

    static constexpr uint16_t       kSizeFixedLeafEdgeNode = 2 + 2 * sizeof(void*);
    static constexpr uint16_t       kNumMaxKeysLeafEdgeNode = (kSizeNode - kSizeFixedLeafEdgeNode) / (sizeof(Key_t) + sizeof(Tid_t));
    static constexpr uint16_t       kSizePaddingLeafEdgeNode = kSizeNode - (kNumMaxKeysLeafEdgeNode * (sizeof(Tid_t) + sizeof(Key_t)) + kSizeFixedLeafEdgeNode);

    static constexpr uint32_t       kSizeMemoryChunk = kSizeNode * kNumMaxKeysInnerNode * 10000;

    typedef typename ChunkRefMemoryHandler::MemoryHandler_t<kSizeMemoryChunk, kSizeCacheLine, kSizeNode * kNumMaxKeysInnerNode> TreeMemoryManager_t;

    struct iterator_tt {
        Tid_t   second;
    };

    TreeMemoryManager_t tmm_;
    byte* root_;
    uint32_t depth_;

public:

    using iterator = iterator_tt*;
    using key_type = Key_t;
    using mapped_type = Tid_t;


    CsbTree_t();
    CsbTree_t(const CsbTree_t&) = default;
    CsbTree_t& operator=(const CsbTree_t&) = default;


    class CsbInnerNode_t{
    public:
        Key_t       keys_       [kNumMaxKeysInnerNode];
        uint16_t    numKeys_;
        byte*       children_;
        uint8_t     free_       [kSizePaddingInnerNode];  // padding

        CsbInnerNode_t();
        inline Key_t getLargestKey() const;
        void shift(uint16_t const aIdx);
        void allocateChildNodes(TreeMemoryManager_t * const aTmm);
        void moveKeysAndChildren(CsbInnerNode_t * const aNodeTarget, uint16_t const aNumKeysRemaining);
        inline void setStopMarker();
        void remove(uint16_t const aIdxToRemove);
        inline uint16_t getChildNodeIdx(byte const * const aChildNode) const;
        inline uint16_t getChildMaxKeys(uint32_t const aChildDepth, uint32_t const aTreeDepth, byte const * const aChild) const;
        inline uint16_t getChildNumKeys(uint32_t const aChildDepth, uint32_t const aTreeDepth, byte const * const aChild) const;
        inline uint16_t getChildNumKeys(bool const aIsLeaf, byte const * const aChild) const;
        inline bool childIsEdge(byte const * const aChildNode) const;
        inline bool childIsFull(uint32_t const aChildDepth, uint32_t const aTreeDepth, byte const * const aChildNode) const;
        std::string asJson(uint32_t const aNodeDepth, uint32_t const aTreeDepth) const;

    } __attribute__((packed));


    class CsbLeafNode_t {
    public:
        Key_t           keys_       [kNumMaxKeysLeafNode];
        Tid_t           tids_       [kNumMaxKeysLeafNode];
        uint16_t        numKeys_;
        uint8_t         free_       [kSizePaddingLeafNode];   // padding

        CsbLeafNode_t();
        inline Key_t getLargestKey() const;
        void insert(Key_t const aKey, Tid_t const aTid);
        void moveKeysAndTids(CsbLeafNode_t * const aNodeTarget, uint16_t const aNumKeysRemaining);
        void leftInsertKeysAndTids(CsbLeafNode_t * const aNodeSource, uint16_t const aNumKeys);
        void remove(Key_t const key, Tid_t const tid);
        void toLeafEdge();
        std::string asJson() const;

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
        inline Key_t getLargestKey() const;
        void insert(Key_t const aKey, Tid_t const aTid);
        void moveKeysAndTids(CsbLeafNode_t * const aNodeTarget, uint16_t const aNumKeysRemaining);
        std::string asJson() const;

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


    uint16_t getCacheLinesPerNode() const;
    inline Tid_t* findRef(Key_t const) const;
    inline iterator find(Key_t const) const;
    inline Tid_t operator [] (Key_t const aKey) const;
    void inline insert(Key_t const aKey, Tid_t const aTid);
    void insert(std::pair<Key_t, Tid_t> const);
    void saveTreeAsJson(std::string const aPath) const;
    void getMemoryUsage() const;


    double_t getFillDegree() const;
    uint64_t getNumKeys() const;
    uint64_t getNumKeysBackwards() const;
    bool verifyOrder() const;

private:

    struct SplitResult_tt{
        byte*       _left;
        uint16_t    _splitIdx;
        bool        _edgeIndicator;
    };


    void split(byte * aNodeToSplit, uint32_t aDepth, Stack_t<CsbInnerNode_t*> * const aPath, SplitResult_tt * const aResult);
    inline bool isLeaf(uint32_t const aDepth) const;
    std::string getTreeAsJson() const;
    void findLeafNode(Key_t const aKey, SearchResult_tt * const aResult) const;
    void findLeafForInsert(Key_t const aKey, SearchResult_tt * const aResult, Stack_t<CsbInnerNode_t*> * aPath) const;
    uint64_t countNodes(CsbInnerNode_t const * const aNode, uint32_t const aDepth) const;

    static std::string kThChildrenAsJson(uint32_t const aK, byte * const aFirstChild, uint32_t const aNodeDepth, uint32_t const aTreeDepth);
    static Key_t getLargestKey(byte const * const aNode, bool const aChild, bool const aEdge);
    static byte* getKthNode(uint16_t const aK, byte * const aNodeFirstChild);
};

#include "../src/csbplustree.cxx"

#endif //CSBPLUSTREE_CSBPLUSTREE_H