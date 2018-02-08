#ifndef CSBPLUSTREE_GROUPEDCSBPLUSTREE_H
#define CSBPLUSTREE_GROUPEDCSBPLUSTREE_H


#define CACHE_LINE_SIZE 64
#define LEAF_kSizeFixedInnerNode 20
#define NUM_INNER_NODES_TO_ALLOC 1000
#define CHUNK_SIZE 100

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stack>
#include <memory.h>
#include "BitsetMemoryHandler.h"
#include "ChunkrefMemoryHandler.h"
#include <cstddef>
#include <fstream>
#include <stddef.h> //offsetof
#include <string>
#include <iostream>



template <class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>

class CsbTree_t{
private:
    static constexpr uint16_t       kSizeCacheLine = 64;

    static constexpr uint16_t       kSizePreambleInnerNodeGroup = 4;
    static constexpr uint16_t       kSizePreambleLeafNodeGroup = 20;

    static constexpr uint32_t       kSizeInnerNode = kSizeCacheLine * kNumCacheLinesPerInnerNode;
    static constexpr uint16_t       kSizeFixedInnerNode = 10;
    static constexpr uint16_t       kNumMaxKeys = (kSizeInnerNode - kSizeFixedInnerNode) / sizeof(Key_t);
    static constexpr uint16_t       kSizePaddingInnerNode = kSizeInnerNode - (kNumMaxKeys * sizeof(Key_t) + kSizeFixedInnerNode);

    static constexpr uint16_t       kSizeFixedLeafNode = 2;
    static constexpr uint16_t       kNumCacheLinesPerLeafNode = ceil((kNumMaxKeys * (sizeof(Tid_t) + sizeof(Key_t)) + kSizeFixedLeafNode)/64.0);
    static constexpr uint32_t       kSizeLeafNode = kNumCacheLinesPerLeafNode * kSizeCacheLine;
    static constexpr uint16_t       kSizePaddingLeafNode = kSizeLeafNode - (kNumMaxKeys * (sizeof(Tid_t) + sizeof(Key_t)) + kSizeFixedLeafNode);
    static constexpr uint32_t       kSizeMemoryChunk = kSizeLeafNode * 1000;

    using TreeMemoryManager_t   = ChunkRefMemoryHandler::MemoryHandler_t<kSizeMemoryChunk, kSizeCacheLine, true>;
    using byte                  = std::byte;


    byte*                   root_;
    TreeMemoryManager_t*    tmm_;

public:

    class CsbInnerNode_t{
    public:
        Key_t       keys_       [kNumMaxKeys];
        uint16_t    numKeys_;
        byte*       children_;
        uint8_t     free_       [kSizePaddingInnerNode];  // padding
        
        CsbInnerNode_t(){
            numKeys_ = 0;
        };


        /*
         * shifts everything past the index
         */
        void shift(uint16_t aIdx) {
            memmove(
                    &keys_[aIdx + 1],
                    &keys_[aIdx],
                    (numKeys_ - aIdx) * sizeof(Key_t)
            );
            numKeys_ += 1;
        }

        void moveKeys(CsbInnerNode_t* aNodeDestination, uint16_t lNumKeysRemaining){
            memmove(
                    &aNodeDestination->keys_,
                    &this->keys_[lNumKeysRemaining],
                    sizeof(Key_t)*(this->numKeys_ - lNumKeysRemaining)
            ); // keys
            aNodeDestination->numKeys_= this->numKeys_ - lNumKeysRemaining;
            aNodeDestination->children_ = getKthNode(lNumKeysRemaining, this->children_);
            this->numKeys_= lNumKeysRemaining;
        }

        void removeKey(uint16_t aIdx){
            memmove(
                    &this->keys_[aIdx],
                    &this->keys_[aIdx + 1],
                    (this->num_keys - 1 - aIdx) * sizeof(Key_t)
            );
            this->numKeys_ -= 1;
        }

        std::string asJson(){
            /*
             * "text": {"name": "Parent node"},
             *  "children": [....]
             *
             */

            std::string lStringKeys = "";
            std:: string lStringChildren = "\"children\": [";
            // attach keys and children
            for (int32_t i= 0; i<numKeys_; i++){
                if (i!=0){
                    lStringKeys.append(",");
                }
                lStringKeys.append(std::to_string(keys_[i]));
                lStringChildren.append(kThChildrenAsJson(i, children_));
                if (i+1 < numKeys_){
                    lStringChildren.append(",");
                }
            }
            lStringChildren.append("]");
            return "{\"stackChildren\":true, \"text\": {\"name\": \"" + lStringKeys +"\"}," + lStringChildren + "}";;
        }
    } __attribute__((packed));


    class CsbLeafNode_t {
    public:
        Key_t           keys_       [kNumMaxKeys];
        uint16_t        numKeys_;
        Tid_t           tids_       [kNumMaxKeys];
        uint8_t         free_       [kSizePaddingLeafNode];   // padding



        CsbLeafNode_t() {
            numKeys_= 0;
        };


        void insert(Key_t aKey, Tid_t aTid){
            uint16_t lIdxToInsert = idxToDescend(aKey, this->keys_, this->numKeys_);

            memmove(&this->keys_[lIdxToInsert + 1],
                    &this->keys_[lIdxToInsert],
                    (this->numKeys_- lIdxToInsert) * sizeof(Key_t)
            );
            memmove(&this->tids_[lIdxToInsert + 1],
                    &this->tids_[lIdxToInsert],
                    (this->numKeys_ - lIdxToInsert) * sizeof(Tid_t)
            );
            this->keys_[lIdxToInsert] = aKey;
            this->tids_[lIdxToInsert] = aTid;
            this->numKeys_ += 1;
        }

        void moveKeysAndTids(CsbLeafNode_t* aNodeDestination, uint16_t aNumKeysRemaining){
            memmove(
                    &aNodeDestination->keys_,
                    &this->keys_[aNumKeysRemaining],
                    sizeof(Key_t)*(this->numKeys_ - aNumKeysRemaining)
            ); // keys
            memmove(
                    &aNodeDestination->tids_,
                    &this->tids_[aNumKeysRemaining],
                    sizeof(Tid_t)*(this->numKeys_ - aNumKeysRemaining)
            ); // tids
            aNodeDestination->numKeys_= this->numKeys_- aNumKeysRemaining;
            this->numKeys_= aNumKeysRemaining;
        }

        void removeTuple(Key_t key, Tid_t tid) {
            uint16_t lIdxToRemove = idxToDescend(key, this->keys_, this->numKeys_);
            while (this->tids_[lIdxToRemove] != tid && lIdxToRemove < this->numKeys_ && this->keys_[lIdxToRemove] == key)
                lIdxToRemove++;
            if (this->tids_[lIdxToRemove] != tid){
                return;
            }
            memmove(
                    this->keys_[lIdxToRemove],
                    this->keys_[lIdxToRemove + 1],
                    (this->numKeys_ - 1 - lIdxToRemove) * sizeof(Key_t)
            );
            memmove(this->tids_[lIdxToRemove],
                    this->tids_[lIdxToRemove + 1],
                    (this->numKeys_ - 1 - lIdxToRemove) * sizeof(Tid_t)
            );
            this->numKeys_-=1;
        }

        std::string asJson(){
            /*
             * "text": {"name": "Parent node"},
             *  "children": [....]
             *
             */
            std::string lJsonObj = "{\"text\":{\"name\":\"";
            lJsonObj.append("keys: " + std::to_string(keys_[0]) + " - " + std::to_string(keys_[numKeys_ - 1]));
            return lJsonObj.append("\"}}");
        }


    } __attribute__((packed));


    class CsbInnerNodeGroup_t{

    public:
        uint16_t                leaf_;                          //2B
        uint16_t                numNodes_;                      //2B
        byte                    padding_    [kSizeCacheLine - kSizePreambleInnerNodeGroup];
        CsbInnerNode_t          members_    [];                 //4B

        CsbInnerNodeGroup_t(uint16_t aNumNodes){
            leaf_ = 0;
            numNodes_ = aNumNodes;
        }

        void inline release(TreeMemoryManager_t* aTmm){
            aTmm->release((byte*) this, this->calcMemoryUsage());
        }


        /*
         * this function shifts all nodes past the index
         *  the subsequent shift within the parents group has to be done manually
         */
        CsbInnerNodeGroup_t* shift(uint16_t aIdx, TreeMemoryManager_t* aTmm){

            CsbInnerNodeGroup_t* lNodeGroupNew = new (aTmm->getMem(calcMemoryUsage(numNodes_ + 1))) CsbInnerNodeGroup_t(numNodes_ + 1);

            memcpy(
                    lNodeGroupNew->members_,
                    this->members_,
                    (aIdx + 1) * kSizeInnerNode
            ); // leading nodes

            memcpy(
                    &lNodeGroupNew->members_[aIdx + 2],
                    &this->members_[aIdx + 1],
                    (numNodes_ - aIdx - 1) * kSizeInnerNode
            ); // trailing nodes


            this->release(aTmm);

            return lNodeGroupNew;
        }

        void splitNodeGroup(uint16_t aNumNodesRemaining, TreeMemoryManager_t* aTmm, byte* aNewGroups[2]){
            uint16_t             lNumNodesRight  = this->numNodes_ - aNumNodesRemaining;
            CsbInnerNodeGroup_t* lNodeGroupLeft  = nullptr;
            CsbInnerNodeGroup_t* lNodeGroupRight = nullptr;

            // create 2 new children node groups
            lNodeGroupLeft = new (aTmm->getMem(calcMemoryUsage(aNumNodesRemaining)))
                    CsbInnerNodeGroup_t(aNumNodesRemaining);

            lNodeGroupRight = new (aTmm->getMem(calcMemoryUsage(lNumNodesRight)))
                    CsbInnerNodeGroup_t(lNumNodesRight);


            // copy the content of the original node group accordingly
            memcpy(
                    &lNodeGroupLeft->members_,
                    &this->members_[0],
                    aNumNodesRemaining * kSizeInnerNode
            );

            memcpy(
                    &lNodeGroupLeft->members_,
                    &this->members_[aNumNodesRemaining],
                    lNodeGroupRight->numNodes_ * kSizeInnerNode
            );

            this->release(aTmm);

            aNewGroups[0] = (byte*) lNodeGroupLeft;
            aNewGroups[1] = (byte*) lNodeGroupRight;

            return;
        }


        CsbInnerNodeGroup_t* splitNode(uint16_t aIdxNode, uint16_t aNumKeysRemaining, TreeMemoryManager_t* aTmm){

            // create a new node group with a one item larger than the old and copy all contents of this to it
            CsbInnerNodeGroup_t* lNodeGroupNew          = this->shift(aIdxNode, aTmm);

            // set the pointers for the node to be splitted
            CsbInnerNode_t*     lNodeSplittedLeft       = &lNodeGroupNew->members_[aIdxNode];
            CsbInnerNode_t*     lNodeSplittedRight      = &lNodeGroupNew->members_[aIdxNode + 1];

            new (lNodeSplittedRight) CsbInnerNode_t;

            // move the keys to the created new node
            lNodeSplittedLeft->moveKeys(lNodeSplittedRight, aNumKeysRemaining);

            // move the child node group to 2 other groups
            byte* lNodeGroupChildrens[2]; // result array
            if (isLeaf(lNodeSplittedLeft->children_)){
                ((CsbLeafNodeGroup_t*) lNodeSplittedLeft->children_)->splitNodeGroup(aNumKeysRemaining, aTmm, lNodeGroupChildrens);
            } else {
                ((CsbInnerNodeGroup_t*) lNodeSplittedLeft->children_)->splitNodeGroup(aNumKeysRemaining, aTmm, lNodeGroupChildrens);
            }

            // attach the 2 new groups to the splitted nodes
            lNodeSplittedLeft->children_ = lNodeGroupChildrens[0];
            lNodeSplittedRight->children_ = lNodeGroupChildrens[1];


            this->release(aTmm);

            return lNodeGroupNew;

        }

        CsbInnerNodeGroup_t* remove(uint16_t aIdx, TreeMemoryManager_t* aTmm){
            CsbInnerNodeGroup_t* lNodeGroupNew = new (aTmm->getMem(calcMemoryUsage(numNodes_ - 1))) CsbInnerNodeGroup_t(numNodes_ - 1);
            CsbInnerNodeGroup_t* lNodeGroupOld = this;
            memcpy(
                    lNodeGroupNew->members_,
                    lNodeGroupOld->members_,
                    aIdx * kSizeInnerNode
            ); // leading nodes

            memcpy(
                    lNodeGroupNew->members_[aIdx],
                    lNodeGroupOld->members_[aIdx + 1],
                    (numNodes_ - aIdx - 1) * kSizeInnerNode
            ); // trailing nodes

            this = lNodeGroupNew;
            aTmm->release(lNodeGroupOld, lNodeGroupOld->calcMemoryUsage());

            return this;
        }

        inline static uint32_t calcMemoryUsage(uint16_t aNumNodes){
            return 64 + aNumNodes * kSizeInnerNode;
        }

        inline uint32_t calcMemoryUsage(){
            return calcMemoryUsage(this->numNodes_);
        }


    }__attribute__((packed));

    class CsbLeafNodeGroup_t{
    public:
        uint16_t                leaf_;                          //2B
        uint16_t                numNodes_;                      //2B
        CsbLeafNodeGroup_t*     preceding_;                     //8B
        CsbLeafNodeGroup_t*     following;                      //8B
        byte                    padding_    [kSizeCacheLine - kSizePreambleLeafNodeGroup];
        CsbLeafNode_t           members_    [];                 //4B

        CsbLeafNodeGroup_t(uint16_t aNumNodes) {
            leaf_ = 1;
            numNodes_ = aNumNodes;
        }

        void inline release(TreeMemoryManager_t* aTmm){
            aTmm->release((byte*) this, this->calcMemoryUsage());
        }


        /*
         * this function shifts all nodes past the index
         *  the subsequent shift within the parents group has to be done manually
         */
        CsbLeafNodeGroup_t* shift(uint16_t aIdx, TreeMemoryManager_t* aTmm){

            CsbLeafNodeGroup_t* lNodeGroupNew = new (aTmm->getMem(calcMemoryUsage(numNodes_ + 1))) CsbLeafNodeGroup_t(numNodes_ + 1);

            memcpy(
                    lNodeGroupNew->members_,
                    this->members_,
                    (aIdx + 1) * kSizeLeafNode
            ); // leading nodes

            memcpy(
                    &lNodeGroupNew->members_[aIdx + 2],
                    &this->members_[aIdx + 1],
                    (numNodes_ - aIdx - 1) * kSizeLeafNode
            ); // trailing nodes


            this->release(aTmm);

            return lNodeGroupNew;
        }

        CsbLeafNodeGroup_t* splitNode(uint16_t aIdxNode, uint16_t aNumKeysRemaining, TreeMemoryManager_t* aTmm){
            CsbLeafNodeGroup_t* lNodeGroupNew = this->shift(aIdxNode, aTmm);

            CsbLeafNode_t*     lNodeSplittedLeft   = &lNodeGroupNew->members_[aIdxNode];
            CsbLeafNode_t*     lNodeSplittedRight  = &lNodeGroupNew->members_[aIdxNode + 1];
            new (lNodeSplittedRight) CsbLeafNode_t;

            lNodeSplittedLeft->moveKeysAndTids(lNodeSplittedRight, aNumKeysRemaining);



            return lNodeGroupNew;

        }

        void splitNodeGroup(uint16_t aNumNodesRemaining, TreeMemoryManager_t* aTmm, byte* aNewGroups[2]){
            uint16_t            lNumNodesRight  = this->numNodes_ - aNumNodesRemaining;
            CsbLeafNodeGroup_t* lNodeGroupLeft  = nullptr;
            CsbLeafNodeGroup_t* lNodeGroupRight = nullptr;

            // create 2 new children node groups
            lNodeGroupLeft = new (aTmm->getMem(calcMemoryUsage(aNumNodesRemaining)))
                    CsbLeafNodeGroup_t(aNumNodesRemaining);

            lNodeGroupRight = new (aTmm->getMem(calcMemoryUsage(lNumNodesRight)))
                    CsbLeafNodeGroup_t(lNumNodesRight);


            // copy the content of the original node group accordingly
            memcpy(
                    &lNodeGroupLeft->members_,
                    &this->members_[0],
                    aNumNodesRemaining * kSizeLeafNode
            );

            memcpy(
                    &lNodeGroupLeft->members_,
                    &this->members_[aNumNodesRemaining],
                    lNodeGroupRight->numNodes_ * kSizeLeafNode
            );

            this->release(aTmm);

            aNewGroups[0] = (byte*) lNodeGroupLeft;
            aNewGroups[1] = (byte*) lNodeGroupRight;

            return;
        }

        CsbLeafNodeGroup_t * remove(uint16_t aIdx, TreeMemoryManager_t aTmm){
            CsbLeafNodeGroup_t* lNodeGroupNew = new (aTmm->getMem(calcMemoryUsage(numNodes_ - 1))) CsbLeafNodeGroup_t(numNodes_ - 1);
            CsbLeafNodeGroup_t* lNodeGroupOld = this;
            memcpy(
                    lNodeGroupNew->members_,
                    lNodeGroupOld->members_,
                    aIdx * kSizeLeafNode
            ); // leading nodes

            memcpy(
                    lNodeGroupNew->members_[aIdx],
                    lNodeGroupOld->members_[aIdx + 1],
                    (numNodes_ - aIdx - 1) * kSizeLeafNode
            ); // trailing nodes

            this = lNodeGroupNew;
            aTmm->release(lNodeGroupOld, lNodeGroupOld->calcMemoryUsage());

            return this;
        }

        inline static uint32_t calcMemoryUsage(uint16_t aNumNodes){
            return 64 + aNumNodes * kSizeLeafNode;
        }

        inline uint32_t calcMemoryUsage(){
            return calcMemoryUsage(this->numNodes_);
        }

        // TODO update pointers method
        void updatePointers();
    }__attribute__((packed));

    CsbTree_t(){
        tmm_ = new TreeMemoryManager_t();
        root_ = tmm_->getMem(CsbLeafNodeGroup_t::calcMemoryUsage(1));
        new (root_) CsbLeafNodeGroup_t(1);
        new (&((CsbLeafNodeGroup_t*) root_)->members_[0]) CsbLeafNode_t();

        static_assert(kSizeInnerNode == sizeof(CsbInnerNode_t));
        static_assert(kSizeLeafNode == sizeof(CsbLeafNode_t));
        static_assert(64 == sizeof(CsbLeafNodeGroup_t));
        static_assert(64 == sizeof(CsbInnerNodeGroup_t));

    }

    inline byte* getRoot(){
        return (byte*) &((CsbLeafNodeGroup_t*) this->root_ )->members_[0];
    }

    CsbLeafNode_t* findLeafNode(Key_t aKey, std::stack<CsbInnerNode_t*>* aPath=nullptr) {
        CsbInnerNode_t *lNodeCurrent = (CsbInnerNode_t*) getRoot();
        uint16_t lLeafIndicator = isLeaf(root_);
        while (!lLeafIndicator) {
            // TODO remove block
            if (((CsbInnerNodeGroup_t*)lNodeCurrent->children_)->numNodes_ != lNodeCurrent->numKeys_){
                std::cout << "node group corrupt @" << aKey << std::endl;
            }
            uint16_t lIdxToDescend = this->idxToDescend(aKey, lNodeCurrent->keys_, lNodeCurrent->numKeys_);
            if (aPath != nullptr){
                aPath->push(lNodeCurrent);
            }
            lLeafIndicator = isLeaf(lNodeCurrent->children_);
            lNodeCurrent = (CsbInnerNode_t *) getKthNode(lIdxToDescend , lNodeCurrent->children_);
        }

        // TODO remove block
        for (uint16_t i = 1; i<((CsbLeafNode_t *) lNodeCurrent)->numKeys_; i++){
            if (((CsbLeafNode_t *) lNodeCurrent)->keys_[i] != ((CsbLeafNode_t *) lNodeCurrent)->keys_[i-1] + 1) {
                std::cout << "Leaf node not continious " <<  aKey << std::endl;
                break;
            }
        }

        return  (CsbLeafNode_t *) lNodeCurrent;
    }

    Tid_t find(Key_t aKey){
        CsbLeafNode_t* lLeafNode = this->findLeafNode(aKey);
        uint16_t lIdxMatching = this->idxToDescend(aKey, lLeafNode->keys_, lLeafNode->numKeys_);
        if (lLeafNode->keys_[lIdxMatching] == aKey){
            return lLeafNode->tids_[lIdxMatching];
        }
        return NULL;
    }

    void insert(Key_t aKey, Tid_t aTid) {
        // TODO remove block
        if (aKey == 9915){
            int a = 0;
        }
        // find the leaf node to insert the key to and record the path upwards the tree
        std::stack<CsbInnerNode_t*> lPath;
        CsbLeafNode_t* lLeafNodeToInsert = findLeafNode(aKey, &lPath);


        // if there is space, just insert
        if (!isFull((byte*) lLeafNodeToInsert)) {
            lLeafNodeToInsert->insert(aKey, aTid);
        } else {
            // else split the node and see in which of the both nodes that have been split the key fits
            lLeafNodeToInsert = (CsbLeafNode_t*) split((byte*) lLeafNodeToInsert, &lPath);
            if (aKey > lLeafNodeToInsert->keys_[lLeafNodeToInsert->numKeys_- 1]){
                (lLeafNodeToInsert + 1)->insert(aKey, aTid);
            }else {
                lLeafNodeToInsert->insert(aKey, aTid);
            }
        }

        //TODO remove this block
        if (!tmm_->verifyPointers()){
            std::cout << "Memory Manager corrupt @" << aKey<< std::endl;
        }

    }

    void remove(Key_t aKey, Tid_t aTid) {
        std::stack<CsbInnerNode_t *>*   lPath;
        CsbInnerNode_t*                 lNodeParent;
        CsbLeafNode_t*                  lLeafNode;
        uint16_t                        lIdxParent  =    (lLeafNode - lNodeParent->children) / kSizeLeafNode;

        lNodeParent = lPath->top();
        lPath->pop();
        lLeafNode = findLeafNode(aKey, &lPath);

        if (aKey != getLargestKey(lLeafNode)) {
            lLeafNode->remove(aKey, aTid);
        } else {
            lLeafNode->remove(aKey, aTid);
            lNodeParent->keys[lIdxParent] = getLargestKey(lLeafNode);
            // TODO behaviour when no key is remaining
        }
    };

    std::string getTreeAsJson(){
        if (isLeaf(root_)){
            return "{\"nodeStructure\":" + ((CsbLeafNode_t*) root_)->asJson() + "}";
        }
        return "{\"nodeStructure\":" + ((CsbInnerNode_t*)root_)->asJson() + "}";
    }

    void saveTreeAsJson(std::string aPath){ ;
        std::ofstream file(aPath);
        file << getTreeAsJson();
    }

    static inline uint16_t isLeaf(byte* aNodeGroup) {
        return ((CsbInnerNodeGroup_t*) aNodeGroup)->leaf_;
    }

    static inline uint16_t childrenIsLeaf(byte* aNodeGroup) {
        return isLeaf(((CsbInnerNodeGroup_t*) aNodeGroup)->members_[0].children_);
    }

    static uint16_t isFull(byte* aNode) {
        return ((CsbInnerNode_t*) aNode)->numKeys_>= kNumMaxKeys;
    }

    static uint16_t getNumKeys(byte* aNode) {
        return ((CsbInnerNode_t*) aNode)->numKeys_;
    }

    static Key_t getLargestKey(byte* aNode) {
        return ((CsbInnerNode_t*) aNode)->keys_[((CsbInnerNode_t*) aNode)->numKeys_-1];
    }

    static inline std::string kThChildrenAsJson(uint32_t aK, byte* aFirstChild){
        if (isLeaf(aFirstChild)){
            return ((CsbLeafNode_t*) getKthNode(aK, aFirstChild))->asJson();
        } else {
            return ((CsbInnerNode_t*) getKthNode(aK, aFirstChild))->asJson();
        }
    }

    byte* split(byte* aNodeToSplit, std::stack<CsbInnerNode_t*>* aPath) {

        uint16_t                lNumKeysLeftSplit;
        uint16_t                lNumKeysRightSplit;
        uint32_t                lSizeNode;
        uint16_t                lIdxNodeToSplit;
        byte*                   lNodeSplittedLeft       = nullptr;
        CsbInnerNode_t*         lNodeParent             = nullptr;
        byte*                   lNodeGroupNodeToSplit   = nullptr;
        byte*                   lNodeGroupAfterSplit    = nullptr;


        if (aNodeToSplit != getRoot()){
            lNodeParent = aPath->top();
            aPath->pop();
            lNumKeysLeftSplit = ceil(getNumKeys(aNodeToSplit)/2.0);
            lNumKeysRightSplit = getNumKeys(aNodeToSplit) - lNumKeysLeftSplit;
        } else {
            // split is called on root_
            lNodeParent = (CsbInnerNode_t*) getRoot();
            lNumKeysLeftSplit = ceil(lNodeParent->numKeys_/2.0);
            lNumKeysRightSplit = lNodeParent->numKeys_- lNumKeysLeftSplit;
            // create a new root and set it as the parent
            CsbInnerNodeGroup_t* lNodeGroupParent = new (
                    tmm_->getMem(CsbInnerNodeGroup_t::calcMemoryUsage(1))
            ) CsbInnerNodeGroup_t(1);
            lNodeParent = new (&lNodeGroupParent->members_[0]) CsbInnerNode_t();
            lNodeParent->children_ = this->root_;
            lNodeParent->numKeys_= 1;
            lNodeParent->keys_[0] = getLargestKey(aNodeToSplit);
            this->root_ = (byte*) lNodeGroupParent;
        }

        lSizeNode               = isLeaf(lNodeParent->children_) ? kSizeLeafNode : kSizeInnerNode;
        lIdxNodeToSplit         = (aNodeToSplit - getKthNode(0, lNodeParent->children_)) / lSizeNode;


        if (isFull((byte*) lNodeParent)) {
            byte* splitted_nodes = split((byte*) lNodeParent, aPath);
            if (lIdxNodeToSplit < lNumKeysLeftSplit) { // if the largest key of the left parent is smaller than the highest key of the nod to be splitted
                lNodeParent = (CsbInnerNode_t*) splitted_nodes; // mark the left node as the parent for the 2 nodes that will be created by the split
            } else {
                lNodeParent = ((CsbInnerNode_t*) splitted_nodes) + 1; // else mark the right node which is adjacent in memory as the parent
                lIdxNodeToSplit -= lNumKeysLeftSplit;
            }
            aNodeToSplit = getKthNode(lIdxNodeToSplit, lNodeParent->children_);

        }

        // split this node
        lNodeGroupNodeToSplit   = lNodeParent->children_;


        /*
         * split the node itself, let the member function:
         * _ create a new node group for the node to be splitted, one item larger than the old
         * _ create a new node
         * _ move half the keys from to this new node
         * _ create 2 new node groups for the children
         * _ move the nodes to these new node groups so that they are evenly distributed
         */
        if (isLeaf(lNodeGroupNodeToSplit)){

            lNodeGroupAfterSplit = (byte*) ((CsbLeafNodeGroup_t*) lNodeGroupNodeToSplit)
                    ->splitNode(lIdxNodeToSplit, lNumKeysLeftSplit, tmm_);

            lNodeSplittedLeft = (byte*) &((CsbLeafNodeGroup_t*) lNodeGroupAfterSplit)->members_[lIdxNodeToSplit];

        } else {

            lNodeGroupAfterSplit = (byte*) ((CsbInnerNodeGroup_t*) lNodeGroupNodeToSplit)
                    ->splitNode(lIdxNodeToSplit, lNumKeysLeftSplit, tmm_);

            lNodeSplittedLeft = (byte*) &((CsbInnerNodeGroup_t*) lNodeGroupAfterSplit)->members_[lIdxNodeToSplit];
        }

        // update the keys_in the parent
        lNodeParent->children_  = lNodeGroupAfterSplit;
        lNodeParent->shift(lIdxNodeToSplit + 1);
        lNodeParent->keys_[lIdxNodeToSplit] = getLargestKey(lNodeSplittedLeft);
        lNodeParent->keys_[lIdxNodeToSplit +1] = getLargestKey(lNodeSplittedLeft + lSizeNode);

        return (byte*) lNodeSplittedLeft;
    }

    static inline byte* getKthNode(uint16_t aK, byte* aNodeGroup) {
        if (isLeaf(aNodeGroup)){
            return (byte*) &((CsbLeafNodeGroup_t*) aNodeGroup)->members_[aK];
        }
        return (byte*) &((CsbInnerNodeGroup_t*) aNodeGroup)->members_[aK];
    }

    /*
     * returns the idx to descend into for a given key
     */
    static uint16_t idxToDescend(Key_t aKey, Key_t aKeys[], uint16_t aNumKeys){
        if (aNumKeys == 0) return 0;
        for (uint16_t i=0; i<aNumKeys; i++){
            if (aKeys[i] >= aKey){
                return i;
            }
        }
        return aNumKeys -1;
    }



    inline static byte* getNodeGroup(byte* aMemberAddress, uint16_t aMemberIndex){
        if (isLeaf(aMemberAddress)){
            return aMemberAddress - offsetof(CsbLeafNodeGroup_t, members_) - (sizeof(CsbLeafNode_t) * aMemberIndex);
        }
        return aMemberAddress - offsetof(CsbInnerNodeGroup_t, members_) - (sizeof(CsbInnerNode_t) * aMemberIndex);

    }
};


#endif //CSBPLUSTREE_GROUPEDCSBPLUSTREE_H
