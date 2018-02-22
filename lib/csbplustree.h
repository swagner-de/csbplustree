#ifndef CSBPLUSTREE_CSBPLUSTREE_H
#define CSBPLUSTREE_CSBPLUSTREE_H


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

    static constexpr uint16_t       kSizeFixedLeafEdgeNode = 10;
    static constexpr uint16_t       kNumMaxKeysLeafEdgeNode = (kSizeNode - kSizeFixedLeafEdgeNode) / (sizeof(Key_t) + sizeof(Tid_t));
    static constexpr uint16_t       kSizePaddingLeafEdgeNode = kSizeNode - (kNumMaxKeysLeafEdgeNode * (sizeof(Tid_t) + sizeof(Key_t)) + kSizeFixedLeafEdgeNode);


    static constexpr uint32_t       kSizeMemoryChunk = kSizeNode * kNumMaxKeysInnerNode * 100;

    typedef typename ChunkRefMemoryHandler::NodeMemoryManager_t<kSizeMemoryChunk, kSizeCacheLine, 1, kSizeNode, kSizeNode> TreeMemoryManager_t;
    typedef typename std::byte byte;

    byte* root_;
    TreeMemoryManager_t* tmm_;
    uint32_t depth_;

public:

    CsbTree_t();


    class CsbInnerNode_t{
    public:
        Key_t       keys_       [kNumMaxKeysInnerNode];
        uint16_t    numKeys_;
        byte*       children_;
        uint8_t     free_       [kSizePaddingInnerNode];  // padding

        CsbInnerNode_t();
        void shift(uint16_t aIdx);
        void allocateChildNodes(TreeMemoryManager_t* aTmm);
        void moveKeysAndChildren(CsbInnerNode_t* aNodeTarget, uint16_t aNumKeysRemaining);
        void remove(uint16_t aIdxToRemove);
        inline uint16_t getChildNodeIdx(byte* aChildNode);
        inline uint16_t getChildMaxKeys(uint32_t aChildDepth, byte* aChild);
        inline uint16_t getChildNumKeys(uint32_t aChildDepth, byte* aChild);
        inline bool childIsEdge(byte* aChildNode, uint32_t aDepth);
        inline bool childIsFull(byte* aChildNode);
        std::string asJson();

    } __attribute__((packed));

    class CsbLeafNodeGroup_t {
    public:
        CsbLeafNode_t           nodes_      [kNumMaxKeysInnerNode];
        CsbLeafNodeGroup_t*     previous_;
        CsbLeafNodeGroup_t      follwing_;
        byte                    padding_    [kSizeCacheLine - 16];
    };


    class CsbLeafNode_t {
    public:
        Key_t           keys_       [kNumMaxKeysLeafNode];
        Tid_t           tids_       [kNumMaxKeysLeafNode];
        uint16_t        numKeys_;
        uint8_t         free_       [kSizePaddingLeafNode];   // padding

        CsbLeafNode_t();
        void insert(Key_t aKey, Tid_t aTid);
        void moveKeysAndTids(CsbLeafNode_t* aNodeTarget, uint16_t aNumKeysRemaining);
        void remove(Key_t key, Tid_t tid);
        std::string asJson();

    } __attribute__((packed));

    class CsbLeafEdgeNode_t {
    public:
        Key_t           keys_       [kNumMaxKeysLeafEdgeNode];
        Tid_t           tids_       [kNumMaxKeysLeafEdgeNode];
        uint16_t        numKeys_;
        CsbLeafNode_t*  adjacent_;
        uint8_t         free_       [kSizePaddingLeafEdgeNode];   // padding

        CsbLeafEdgeNode_t();

        void insert(Key_t aKey, Tid_t aTid);
        void moveKeysAndTids(CsbLeafNode_t* aNodeTarget, uint16_t aNumKeysRemaining);
        void moveKeysAndTids(CsbLeafEdgeNode_t* aNodeTarget, uint16_t aNumKeysRemaining);
        void convertToLeafNode();
        void remove(Key_t key, Tid_t tid);
        std::string asJson();

    } __attribute__((packed));


    struct SplitResult_tt{
        byte*       _left;
        uint16_t    _splitIdx;
        bool        _edgeIndicator[2];
    };



    byte* findLeafNode(Key_t aKey, std::stack<CsbInnerNode_t*>* aPath= nullptr);
    Tid_t find(Key_t aKey);

    void insert(Key_t aKey, Tid_t aTid);
    void remove(Key_t aKey, Tid_t aTid);
    void split(byte* aNodeToSplit, uint32_t aDepth, std::stack<TreePath_tt>* aPath, SplitResult_tt* aResult);
    std::string getTreeAsJson();
    void saveTreeAsJson(std::string aPath);


    inline bool isLeaf(uint32_t aDepth);
    static inline uint16_t getNumKeys(byte* aNode);
    static inline std::string kThChildrenAsJson(uint32_t aK, byte* aFirstChild);
    static byte* getKthNode(uint16_t aK, byte* aNodeFirstChild);
    static uint16_t idxToDescend(Key_t aKey, Key_t aKeys[], uint16_t aNumKeys);

};


#endif //CSBPLUSTREE_CSBPLUSTREE_H