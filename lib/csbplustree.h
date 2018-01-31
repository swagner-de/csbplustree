#ifndef CSBPLUSTREE_CSBPLUSTREE_H
#define CSBPLUSTREE_CSBPLUSTREE_H

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
#include <string>
#include <iostream>


typedef std::byte byte;

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



    //using TreeMemoryManager_t_T = BitSetMemoryHandler::MemoryManager_t<kSizeInnerNode, kSizeLeafNode, kSizeMemoryChunks>;
    using TreeMemoryManager_t = ChunkRefMemoryHandler::NodeMemoryManager_t<kSizeMemoryChunk, kSizeCacheLine, 1, kSizeInnerNode, kSizeLeafNode>;

    byte* root_;
    TreeMemoryManager_t* tmm_;

public:

    CsbTree_t(){
        tmm_ = new TreeMemoryManager_t();
        root_ = tmm_->getMem(1, true);
        new (root_) CsbLeafNode_t;
        static_assert(kSizeInnerNode == sizeof(CsbInnerNode_t));
        static_assert(kSizeLeafNode == sizeof(CsbLeafNode_t));

    }


    class CsbInnerNode_t{
    public:
        Key_t       keys_       [kNumMaxKeys];
        uint16_t    leaf_       :1;
        uint16_t    numKeys_    :15;
        byte*       children_;
        uint8_t     free_       [kSizePaddingInnerNode];  // padding




        CsbInnerNode_t(){
            numKeys_ = 0;
            leaf_ = false;
        };

        /*
         * this function shifts all nodes past the insert index within the parents node group
         */
        byte* insert(uint16_t aIdxToInsert, TreeMemoryManager_t* aTmm){

            uint32_t lSizeNode;
            byte* lMemNewChunk;
            byte* lNodeToInsert;

            lSizeNode = (isLeaf(this->children_)) ? kSizeLeafNode : kSizeInnerNode;

            // allocate new node group, 1 node larger than the old one
            lMemNewChunk = aTmm->getMem(this->numKeys_ + 1, isLeaf(this->children_));
            // copy values to new group
            memcpy(
                    lMemNewChunk,
                    this->children_,
                    aIdxToInsert*lSizeNode
            ); // leading nodes
            memcpy(
                    lMemNewChunk + lSizeNode * (aIdxToInsert + 1),
                    this->children_ + lSizeNode * (aIdxToInsert),
                    (this->numKeys_ - aIdxToInsert) * lSizeNode
            ); // trailing nodes

            // shift keys_in parent
            memmove(
                    &this->keys_[aIdxToInsert + 1],
                    &this->keys_[aIdxToInsert],
                    sizeof(Key_t) * (this->numKeys_-aIdxToInsert)
            );

            aTmm->release(this->children_, this->numKeys_, isLeaf(this->children_));
            // update the child ptr
            this->children_ = lMemNewChunk;

            if (isLeaf(this->children_)){
                lNodeToInsert =  (byte*) new (lMemNewChunk + lSizeNode*aIdxToInsert) CsbLeafNode_t();
            }else{
                lNodeToInsert =  (byte*) new (lMemNewChunk + lSizeNode*aIdxToInsert) CsbInnerNode_t();
            }

            return  lNodeToInsert - lSizeNode; // returns a pointer to the left node, where the node group's containing child nodes have been copied from
        }

        void splitKeys(CsbInnerNode_t* aNodeToSplit, uint16_t lNumKeysRemaining){
            memmove(
                    &aNodeToSplit->keys_,
                    &this->keys_[lNumKeysRemaining],
                    sizeof(Key_t)*(this->numKeys_ - lNumKeysRemaining)
            ); // keys
            aNodeToSplit->numKeys_= this->numKeys_ - lNumKeysRemaining;
            aNodeToSplit->children_ = getKthNode(lNumKeysRemaining, this->children_);
            this->numKeys_= lNumKeysRemaining;
        }

        void remove(uint16_t aIdxToRemove){
            uint32_t lSizeNode = (childrenIsLeaf(this)) ? kSizeLeafNode : kSizeInnerNode;
            byte* lMemSmallerChunk = tmm_->getMem(this->numKeys_ - 1, isLeaf(this->children_));

            memcpy(
                    lMemSmallerChunk,
                    this->children,
                    aIdxToRemove*lSizeNode
            );
            memcpy(
                    lMemSmallerChunk + (aIdxToRemove*lSizeNode),
                    this->children + ((aIdxToRemove+1) * lSizeNode),
                    (this->numKeys_ - 1 - aIdxToRemove) * lSizeNode
            );
            tmm_->release(this->children, this->num_keys, isLeaf(this->children));

            this->children = lMemSmallerChunk;
            memmove(
                    &this->keys[aIdxToRemove],
                    &this->keys[aIdxToRemove+1],
                    (this->num_keys-1-aIdxToRemove)*sizeof(Key_t)
            );
            this->numKeys_-= 1;
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
        uint16_t        leaf_       :1;
        uint16_t        numKeys_    :15;
        Tid_t           tids_       [kNumMaxKeys];
        CsbLeafNode_t*  preceding_node_;
        CsbLeafNode_t*  following_node_;
        uint8_t         free_       [kSizePaddingLeafNode];   // padding



        CsbLeafNode_t() {
            leaf_ = true;
            numKeys_= 0;
        };

        void updatePointers(CsbLeafNode_t* preceding, CsbLeafNode_t* following){
            this->preceding_node = preceding;
            this->following_node = following;
            preceding->following_node = &this;
            following->preceding_node = &this;
        }

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

        void splitKeysAndTids(CsbLeafNode_t* aNodeToSplit, uint16_t aNumKeysRemaining){
            memmove(
                    &aNodeToSplit->keys_,
                    &this->keys_[aNumKeysRemaining],
                    sizeof(Key_t)*(this->numKeys_ - aNumKeysRemaining)
            ); // keys
            memmove(
                    &aNodeToSplit->tids_,
                    &this->tids_[aNumKeysRemaining],
                    sizeof(Tid_t)*(this->numKeys_ - aNumKeysRemaining)
            ); // tids
            aNodeToSplit->numKeys_= this->numKeys_- aNumKeysRemaining;
            this->numKeys_= aNumKeysRemaining;
        }

        void remove(Key_t key, Tid_t tid) {
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



    CsbLeafNode_t* findLeafNode(Key_t aKey, std::stack<CsbInnerNode_t*>* aPath=nullptr) {
        CsbInnerNode_t *lNodeCurrent = (CsbInnerNode_t*) root_;
        while (!lNodeCurrent->leaf_) {
            uint16_t lIdxToDescend = this->idxToDescend(aKey, lNodeCurrent->keys_, lNodeCurrent->numKeys_);
            if (aPath != nullptr){
                aPath->push(lNodeCurrent);
            }
            lNodeCurrent = (CsbInnerNode_t *) getKthNode(lIdxToDescend , lNodeCurrent->children_);
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

        // find the leaf node to insert the key to and record the path upwards the tree
        std::stack<CsbInnerNode_t*> lPath;
        CsbLeafNode_t* lLeafNodeToInsert = findLeafNode(aKey, &lPath);

        if (aKey == 9981){
            std::cout<< aKey<< std::endl;
        }

        // if there is space, just insert
        if (!isFull((byte*) lLeafNodeToInsert)){
            lLeafNodeToInsert->insert(aKey, aTid);
            return;
        }
        // else split the node and see in which of the both nodes that have been split the key fits

        lLeafNodeToInsert = (CsbLeafNode_t*) split((byte*) lLeafNodeToInsert, &lPath);
        if (aKey > lLeafNodeToInsert->keys_[lLeafNodeToInsert->numKeys_- 1]){
            (lLeafNodeToInsert + 1)->insert(aKey, aTid);
        }else {
            lLeafNodeToInsert->insert(aKey, aTid);
        }
    }

    void remove(Key_t aKey, Tid_t aTid) {
        std::stack<CsbInnerNode_t*>* lPath;
        CsbInnerNode_t* lNodeParent;
        CsbLeafNode_t* lLeafNode;
        uint16_t lIdxParent = (lLeafNode-lNodeParent->children)/kSizeLeafNode;

        lNodeParent = lPath->top();
        lPath->pop();
        lLeafNode = findLeafNode(aKey, &lPath);

        if (aKey != getLargesKey_t(lLeafNode)) {
            lLeafNode->remove(aKey, aTid);
        }
        else{
            lLeafNode->remove(aKey, aTid);
            lNodeParent->keys[lIdxParent] = getLargesKey_t(lLeafNode);
        }
    }

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

    static inline uint16_t isLeaf(CsbInnerNode_t* aNode){
        return aNode->leaf_;
    }

    static inline uint16_t isLeaf(CsbLeafNode_t* aNode){
        return aNode->leaf_;
    }

    static inline uint16_t isLeaf(byte* aNode) {
        return isLeaf((CsbInnerNode_t*) aNode);
    }

    static inline uint16_t childrenIsLeaf(byte* aNode) {
        return ((CsbInnerNode_t*)(((CsbInnerNode_t*) aNode)->children))->leaf_;
    }

    static uint16_t isFull(byte* aNode) {
        return ((CsbInnerNode_t*) aNode)->numKeys_>= kNumMaxKeys;
    }

    static uint16_t getNumKeys(byte* aNode) {
        return ((CsbInnerNode_t*) aNode)->numKeys_;
    }

    static Key_t getLargesKey_t(byte* aNode) {
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

        uint16_t  lIdxNodeToSplit;
        byte* lNodeSplittedRight;
        byte* lNodeSplittedLeft;
        CsbInnerNode_t* lNodeParent;
        uint16_t lNumKeysLeftSplit;
        uint16_t lNumKeysRightSplit;
        uint32_t lSizeNode = (isLeaf(aNodeToSplit)) ? kSizeLeafNode : kSizeInnerNode;

        if (this->root_ != aNodeToSplit){
            lNodeParent = aPath->top();
            aPath->pop();
            lNumKeysLeftSplit = ceil(getNumKeys(aNodeToSplit)/2.0);
            lNumKeysRightSplit = getNumKeys(aNodeToSplit) - lNumKeysLeftSplit;
        }else{
            // split is called on root_
            lNodeParent = (CsbInnerNode_t*) this->root_;
            lNumKeysLeftSplit = ceil(lNodeParent->numKeys_/2.0);
            lNumKeysRightSplit = lNodeParent->numKeys_- lNumKeysLeftSplit;

            lNodeParent = new (tmm_->getMem(1, false)) CsbInnerNode_t();
            lNodeParent->children_ = this->root_;
            lNodeParent->numKeys_= 1;
            lNodeParent->keys_[0] = getLargesKey_t(this->root_);
            this->root_ = (byte*) lNodeParent;

        }

        if (isFull((byte*) lNodeParent)) {
            byte* splitted_nodes = split((byte*) lNodeParent, aPath);
            if (getLargesKey_t(splitted_nodes) > getLargesKey_t(aNodeToSplit) ){ // if the largest key of the left parent is smaller than the highest key of the nod to be splitted
                lNodeParent = (CsbInnerNode_t*) splitted_nodes; // mark the left node as the parent for the 2 nodes that will be created by the split
            }else{
                lNodeParent = ((CsbInnerNode_t*) splitted_nodes) + 1; // else mark the right node which is adjacent in memory as the parent
            }

        }

        // split this node
        lIdxNodeToSplit = (aNodeToSplit - lNodeParent->children_) / lSizeNode;
        lNodeSplittedLeft = lNodeParent->insert(lIdxNodeToSplit + 1, tmm_);
        lNodeSplittedRight = lNodeSplittedLeft + lSizeNode;

        if (isLeaf(lNodeSplittedLeft)){
            // first move the keys_from the original node to the newly allocated
            ((CsbLeafNode_t*)lNodeSplittedLeft)->splitKeysAndTids((CsbLeafNode_t*) lNodeSplittedRight, lNumKeysLeftSplit);
        }else{
            // move the keys_from orig to new node
            ((CsbInnerNode_t*)lNodeSplittedLeft)->splitKeys(
                    (CsbInnerNode_t*) lNodeSplittedRight, lNumKeysLeftSplit);
        }

        // update the keys_in the parent
        lNodeParent->keys_[lIdxNodeToSplit] = getLargesKey_t(lNodeSplittedLeft);
        lNodeParent->keys_[lIdxNodeToSplit +1] = getLargesKey_t(lNodeSplittedRight);
        lNodeParent->numKeys_+=1;

        return (byte*) lNodeSplittedLeft;
    }

    static byte* getKthNode(uint16_t aK, byte* aNodeFirstChild) {
        return aNodeFirstChild + aK*(isLeaf(aNodeFirstChild) ? kSizeLeafNode : kSizeInnerNode);
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
};


#endif //CSBPLUSTREE_CSBPLUSTREE_H