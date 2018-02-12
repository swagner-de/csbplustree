#ifndef CSBPLUSTREE_CSBPLUSTREE_H
#define CSBPLUSTREE_CSBPLUSTREE_H


template <class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>

class CsbTree_t{
private:
    static constexpr uint16_t       kSizeCacheLine = 64;

    static constexpr uint32_t       kSizeInnerNode = kSizeCacheLine * kNumCacheLinesPerInnerNode;
    static constexpr uint16_t       kSizeFixedInnerNode = 10;
    static constexpr uint16_t       kNumMaxKeys = (kSizeInnerNode - kSizeFixedInnerNode) / sizeof(Key_t);
    static constexpr uint16_t       kSizePaddingInnerNode = kSizeInnerNode - (kNumMaxKeys * sizeof(Key_t) + kSizeFixedInnerNode);

    static constexpr uint16_t       kSizeFixedLeafNode = 18;
    static constexpr uint16_t       kNumCacheLinesPerLeafNode = ceil((kNumMaxKeys * (sizeof(Tid_t) + sizeof(Key_t)) + kSizeFixedLeafNode)/64.0);
    static constexpr uint32_t       kSizeLeafNode = kNumCacheLinesPerLeafNode * kSizeCacheLine;
    static constexpr uint16_t       kSizePaddingLeafNode = kSizeLeafNode - (kNumMaxKeys * (sizeof(Tid_t) + sizeof(Key_t)) + kSizeFixedLeafNode);
    static constexpr uint32_t       kSizeMemoryChunk = kSizeLeafNode * 1000;

    typedef typename ChunkRefMemoryHandler::NodeMemoryManager_t<kSizeMemoryChunk, kSizeCacheLine, 1, kSizeInnerNode, kSizeLeafNode> TreeMemoryManager_t;
    typedef typename std::byte byte;

    byte* root_;
    TreeMemoryManager_t* tmm_;

public:

    CsbTree_t();

    class CsbInnerNode_t{
    public:
        Key_t       keys_       [kNumMaxKeys];
        uint16_t    leaf_       :1;
        uint16_t    numKeys_    :15;
        byte*       children_;
        uint8_t     free_       [kSizePaddingInnerNode];  // padding

        CsbInnerNode_t();
        byte* insert(uint16_t aIdxToInsert, TreeMemoryManager_t* aTmm);
        void splitKeys(CsbInnerNode_t* aNodeToSplit, uint16_t lNumKeysRemaining);
        void remove(uint16_t aIdxToRemove);
        std::string asJson();

    } __attribute__((packed));


    class CsbLeafNode_t {
    public:
        Key_t           keys_       [kNumMaxKeys];
        uint16_t        leaf_       :1;
        uint16_t        numKeys_    :15;
        Tid_t           tids_       [kNumMaxKeys];
        CsbLeafNode_t*  preceding_node_;
        CsbLeafNode_t*  following_node_;
        uint8_t         free_       [kSizePaddingLeafNode];   // padding

        CsbLeafNode_t();

        void updatePointers(CsbLeafNode_t* preceding, CsbLeafNode_t* following);
        void insert(Key_t aKey, Tid_t aTid);
        void splitKeysAndTids(CsbLeafNode_t* aNodeToSplit, uint16_t aNumKeysRemaining);
        void remove(Key_t key, Tid_t tid);
        std::string asJson();

    } __attribute__((packed));



    CsbLeafNode_t* findLeafNode(Key_t aKey, std::stack<CsbInnerNode_t*>* aPath= nullptr);
    Tid_t find(Key_t aKey);

    void insert(Key_t aKey, Tid_t aTid);
    void remove(Key_t aKey, Tid_t aTid);
    byte* split(byte* aNodeToSplit, std::stack<CsbInnerNode_t*>* aPath);
    std::string getTreeAsJson();
    void saveTreeAsJson(std::string aPath);

    static inline uint16_t isLeaf(CsbInnerNode_t* aNode);
    static inline uint16_t isLeaf(CsbLeafNode_t* aNode);
    static inline uint16_t isLeaf(byte* aNode);
    static inline uint16_t childrenIsLeaf(byte* aNode);
    static inline uint16_t isFull(byte* aNode);
    static uint16_t getNumKeys(byte* aNode);
    static Key_t getLargesKey_t(byte* aNode);
    static inline std::string kThChildrenAsJson(uint32_t aK, byte* aFirstChild);
    static byte* getKthNode(uint16_t aK, byte* aNodeFirstChild);
    static uint16_t idxToDescend(Key_t aKey, Key_t aKeys[], uint16_t aNumKeys);
};


#endif //CSBPLUSTREE_CSBPLUSTREE_H