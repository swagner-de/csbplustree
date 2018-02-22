#ifndef CSBPLUSTREE_CSBPLUSTREE_CXX
#define CSBPLUSTREE_CSBPLUSTREE_CXX


#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stack>
#include <memory.h>
#include "ChunkrefMemoryHandler.h"
#include <cstddef>
#include <fstream>
#include <string>
#include <iostream>
#include "csbplustree.h"


template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
CsbTree_t() {
    tmm_ = new TreeMemoryManager_t();
    root_ = tmm_->getMem(kNumMaxKeysInnerNode, true);
    new(root_) CsbLeafNode_t;
    depth_ = 0;
    static_assert(kSizeNode == sizeof(CsbInnerNode_t));
    static_assert(kSizeNode == sizeof(CsbLeafNode_t));
    static_assert(kSizeNode == sizeof(CsbLeafEdgeNode_t));
}

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
CsbInnerNode_t::
CsbInnerNode_t(){
    numKeys_ = 0;
}
template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
void
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
CsbInnerNode_t::
allocateChildNodes(TreeMemoryManager_t* aTmm) {
    this->children_ = aTmm->getMem(kNumMaxKeysInnerNode, true);
}

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
uint16_t
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
CsbInnerNode_t::
getChildNodeIdx(byte *aChildNode) {
    return (aChild - this->children_) / kSizeNode;
}

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
bool
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
CsbInnerNode_t::
childIsEdge(byte* aChildNode, uint32_t aDepth) {
    uint16_t lChildNodeIdx = this->getChildNodeIdx(aChildNode);
    return (
            isLeaf(aDepth) &&
            (lChildNodeIdx == 0 || lChildNodeIdx == this->numKeys_ - 1)
    );
}

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
uint16_t
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
CsbInnerNode_t::
getChildMaxKeys(uint32_t aChildDepth, byte *aChild) {
    if (isLeaf(aChildDepth)) {
        if(childIsEdge(aChild, aChildDepth)) {
            return kNumMaxKeysLeafEdgeNode;
        }
        return kNumMaxKeysLeafNode;
    }
    return kNumMaxKeysInnerNode;
}

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
uint16_t
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
CsbInnerNode_t::
getChildNumKeys(uint32_t aChildDepth, byte *aChild) {
    if (isLeaf(aChildDepth)) {
        if(childIsEdge(aChild, aChildDepth)) {
            return ((CsbLeafEdgeNode_t*) aChild)->numKeys_;
        }
        return ((CsbLeafNode_t*) aChild)->numKeys_;
    }
    return ((CsbLeafInnerNode_t*) aChild)->numKeys_;
}
template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
bool
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
CsbInnerNode_t::
childIsFull(uint16_t aChildDepth, byte *aChildNode) {
    return (this->getChildMaxKeys(aChildDepth, aChildNode) ==
            this->getChildNumKeys(aChildDepth, aChildNode));
}


template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
CsbLeafNode_t::
CsbLeafNode_t() {
    CsbInnerNode_t();
}


template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
CsbLeafEdgeNode_t::
CsbLeafEdgeNode_t() {
    CsbInnerNode_t();
}

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
void
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
CsbInnerNode_t::
shift(uint16_t aIdx) {


    /*
     *   0   1   2   3
     * [ x | x | x |   ]
     * [   | x | x | x ]
     *   args: idx = 0
     */

    // shift keys_
    memmove(
            &this->keys_[aIdxToInsert + 1],
            &this->keys_[aIdxToInsert],
            sizeof(Key_t) * (this->numKeys_ - aIdxToInsert)
    );

    // shift the child nodes
    memmove(
            &this->children_[aIdxToInsert + 1],
            &this->children_[aIdxToInsert],
            kSizeNode * (this->numKeys_ - aIdxToInsert)
    );

    this->numKeys_ += 1;
}


template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
void
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
CsbLeafNode_t::
insert(Key_t aKey, Tid_t aTid) {

    uint16_t lIdxToInsert = idxToDescend(aKey, this->keys_, this->numKeys_);

    memmove(&this->keys_[lIdxToInsert + 1],
            &this->keys_[lIdxToInsert],
            (this->numKeys_ - lIdxToInsert) * sizeof(Key_t)
    );
    memmove(&this->tids_[lIdxToInsert + 1],
            &this->tids_[lIdxToInsert],
            (this->numKeys_ - lIdxToInsert) * sizeof(Tid_t)
    );
    this->keys_[lIdxToInsert] = aKey;
    this->tids_[lIdxToInsert] = aTid;
    this->numKeys_ += 1;
}

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
void
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
insert(Key_t aKey, Tid_t aTid) {

    // find the leaf node to insert the key to and record the path upwards the tree
    std::stack<CsbInnerNode_t*>         lPath;
    CsbInnerNode_t*                     lNodeParent;
    byte*                               lPtrNodeLeaf;
    bool                                lIsLeafEdge;


    lPtrNodeLeaf    = findLeafNode(aKey, lPath);
    lNodeParent     = lPath.top();
    lIsLeafEdge     = lNodeParent->childIsEdge(lPtrNodeLeaf, this->depth_);

    // check if space is available and split if nescessarry
    if (lNodeParent->childrenIsFull(lPtrNodeLeaf, this->depth_)) {

        SplitResult_tt*                 lSplitResult;
        split(lPtrNodeLeaf, this->depth_, &lPath, &lSplitResult);

        // determine which of resulting nodes is the node to insert an look up if it is a edge or not
        if (lPathLeaf._parentIdx >= lSplitResult->_splitIdx) {
            lPtrNodeLeaf = lSplitResult->_left + kSizeNode;
            lIsLeafEdge  = lSplitResult->_edgeIndicator[1];
        } else {
            lPtrNodeLeaf = lSplitResult->_left;
            lIsLeafEdge  = lSplitResult->_edgeIndicator[0];
        }
    }

    if (lIsLeafEdge) {
        ((CsbLeafEdgeNode_t*) lPtrNodeLeaf)->insert(aKey, aTid);
    } else {
        ((CsbLeafNode_t*) lPtrNodeLeaf)->insert(aKey, aTid);
    }
    return;

}

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
void
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
CsbInnerNode_t::
moveKeysAndChildren(CsbInnerNode_t* aNodeTarget, uint16_t aNumKeysRemaining) {
    memmove(
            aNodeTarget->keys_,
            &this->keys_[aNumKeysRemaining],
            sizeof(Key_t) * (this->numKeys_ - aNumKeysRemaining)
    ); // keys
    memcpy(
            aNodeTarget->children_,
            &this->children_[aNumKeysRemaining],
            kSizeNode * (this->numKeys_ - aNumKeysRemaining)
    ); // children


    aNodeTarget->numKeys_ = this->numKeys_ - aNumKeysRemaining;
    this->numKeys_ = aNumKeysRemaining;
}

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
void
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
CsbLeafNode_t::
moveKeysAndTids(CsbLeafNode_t* aNodeTarget, uint16_t aNumKeysRemaining){
    memmove(
            aNodeTarget->keys_,
            &this->keys_[aNumKeysRemaining],
            sizeof(Key_t) * (this->numKeys_ - aNumKeysRemaining)
    ); // keys_
    memmove(
            aNodeTarget->tids_,
            &this->tids_[aNumKeysRemaining],
            sizeof(Tid_t) * (this->numKeys_ - aNumKeysRemaining)
    ); // tids_

    aNodeTarget->numKeys_ = this->numKeys_ - aNumKeysRemaining;
    this->numKeys_ = aNumKeysRemaining;
}

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
void
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
CsbLeafEdgeNode_t::
moveKeysAndTids(CsbLeafNode_t* aNodeTarget, uint16_t aNumKeysRemaining){
    memmove(
            aNodeTarget->keys_,
            &this->keys_[aNumKeysRemaining],
            sizeof(Key_t) * (this->numKeys_ - aNumKeysRemaining)
    ); // keys_
    memmove(
            aNodeTarget->tids_,
            &this->tids_[aNumKeysRemaining],
            sizeof(Tid_t) * (this->numKeys_ - aNumKeysRemaining)
    ); // tids_

    aNodeTarget->numKeys_ = this->numKeys_ - aNumKeysRemaining;
    this->numKeys_ = aNumKeysRemaining;
}

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
void
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
CsbLeafEdgeNode_t::
moveKeysAndTids(CsbLeafEdgeNode_t* aNodeTarget, uint16_t aNumKeysRemaining){
    memmove(
            aNodeTarget->keys_,
            &this->keys_[aNumKeysRemaining],
            sizeof(Key_t) * (this->numKeys_ - aNumKeysRemaining)
    ); // keys_
    memmove(
            aNodeTarget->tids_,
            &this->tids_[aNumKeysRemaining],
            sizeof(Tid_t) * (this->numKeys_ - aNumKeysRemaining)
    ); // tids_

    aNodeTarget->numKeys_ = this->numKeys_ - aNumKeysRemaining;
    this->numKeys_ = aNumKeysRemaining;
}

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
void
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::CsbLeafEdgeNode_t::
convertToLeafNode() {
    uint16_t lNumKeys = this->numKeys_;
    memmove(
            ((CsbLeafNode_t*) this)->tids_,
            this->tids_,
            sizeof(Tid_t) * lNumKeys
    );
    ((CsbLeafNode_t*) this)->numKeys_ = lNumKeys;
}


template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
void
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
split(byte *aNodeToSplit, uint32_t aDepth, std::stack<TreePath_tt> *aPath, SplitResult_tt* aResult) {

    CsbInnerNode_t* lNodeParent;
    byte*           lNodeSplittedRight;
    uint16_t        lNumKeysNodeToSplit;
    uint16_t        lNumKeysLeftSplit;
    uint16_t        lIdxNodeToSplit;


    if (this->root_ = aNodeToSplit) {
        // split is called on root_
        lNodeParent = new(tmm_->getMem(kNumMaxKeysInnerNode, true)) CsbInnerNode_t();
        lNodeParent->children_ = this->root_;
        lNodeParent->numKeys_ = 1;
        depth_ += 1;
        aDepth += 1;
        lNodeParent->keys_[0] = getLargesKey_t(this->root_);
        this->root_ = (byte *) lNodeParent;
    } else {
        lNodeParent = aPath->top();
        aPath->pop();
    }

    lIdxNodeToSplit = lNodeParent->getChildNodeIdx(aNodeToSplit );
    lNumKeysNodeToSplit = lNodeParent->getChildNumKeys(aDepth, aNodeToSplit);
    lNumKeysLeftSplit = ceil(lNumKeysNodeToSplit / 2.0);

    // case if parent node is full
    if (lNodeParent->numKeys_ == kNumMaxKeysInnerNode) {

        SplitResult_tt lParentSplit;

        split((byte *) lNodeParent, aDepth - 1, aPath, &lParentSplit);

        // select the matching parent and reassign the pointer to the node as due to the split the location has changed
        if (lIdxNodeToSplit <= lParentSplit._splitIdx) {
            lNodeParent = (CsbInnerNode_t*) lParentSplit._left;
            aNodeToSplit = getKthNode(lIdxNodeToSplit, lNodeParent->children_);

        } else {
            lNodeParent = ((CsbInnerNode_t*) lParentSplit._left) + 1;
            aNodeToSplit = getKthNode(lIdxNodeToSplit - lParentSplit._splitIdx, lNodeParent->children_);
        }
    }



    /*
     * _ shift the nodes in the parent and the parents' node group
     * _ create a new node at the right place in the parent
     * _ move keys in the parent to the newly created node
     * _ create a new node group for the child nodes
     * _ move part of the child nodes from aNodeToSplit to the new node group
     * _ update counters
     */

    lNodeParent->shift(lIdxNodeToSplit + 1);
    lNodeSplittedRight = getKthNode(lIdxNodeToSplit + 1, lNodeParent->children_);

    if (isLeaf(aDepth)) {
        if (lNodeParent->childIsEdge(aNodeToSplit, aDepth)) {
            // LeafEdgeNode

            if (lIdxNodeToSplit == 0) {
                /*
                 *   v
                 * [ e |   | o | ....]
                 *  right edge:
                 *  _ create new LeafNode
                 *  _ node remains an edge, simply move the keys_ and tids_
                 *
                 */
                aResult->_edgeIndicator[0]
                new(lNodeSplittedRight) CsbLeafNode_t();
                ((CsbLeafEdgeNode_t *) aNodeToSplit)->moveKeysAndTids((CsbLeafNode_t*) lNodeSplittedRight, lNumKeysLeftSplit);
            }
            if (lIdxNodeToSplit = lNodeParent->numKeys_ - 1){
                /*
                 *               v
                 * [ e | o | o | e |   ]
                 *  left edge:
                 *  _ create LeafEdge right of current
                 *  _ move keys_ and tids_
                 *  _ set adjacent pointer
                 *  _ convert the former LeafEdgeNode to a LeafNode
                 */
                lNumKeysLeftSplit = lNumKeysNodeToSplit - (ceil(kNumMaxKeysLeafEdgeNode/2.0));
                new (lNodeSplittedRight) CsbLeafEdgeNode_t();
                ((CsbLeafEdgeNode_t *) aNodeToSplit)->moveKeysAndTids((CsbLeafEdgeNode_t*) lNodeSplittedRight, lNumKeysLeftSplit);
                ((CsbLeafEdgeNode_t *) lNodeSplittedRight)->adjacent_ = ((CsbLeafEdgeNode_t *) aNodeToSplit)->adjacent_;
                ((CsbLeafEdgeNode_t *) aNodeToSplit)->convertToLeafNode();

            }

        } else {
            // LeafNode
            new (lNodeSplittedRight) CsbLeafNode_t();
            ((CsbLeafNode_t*) aNodeToSplit)->splitKeysAndTids(lNodeSplittedRight, lNumKeysLeftSplit);
        }

    } else {
        // InnerNode
        new (lNodeSplittedRight) CsbInnerNode_t();
        ((CsbInnerNode_t*) lNodeSplittedRight)->allocateChildNodes(tmm_);
        ((CsbInnerNode_t*) aNodeToSplit)->moveKeysAndChildren(lNodeSplittedRight, lNumKeysLeftSplit);
    }

    lNodeParent->keys_[lIdxNodeToSplit] = getLargestKey(aNodeToSplit, lIdxNodeToSplit, aDepth);
    lNodeParent->keys_[lIdxNodeToSplit + 1] = getLargestKey(aNodeToSplit, lIdxNodeToSplit + 1, aDepth);

    aResult->_left              = aNodeToSplit;
    aResult->_splitIdx          = lNumKeysLeftSplit;
}

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
void
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
CsbInnerNode_t::
remove(uint16_t aIdxToRemove) {
    uint32_t lSizeNode = (childrenIsLeaf(this)) ? kSizeLeafNode : kSizeInnerNode;
    byte *lMemSmallerChunk = tmm_->getMem(this->numKeys_ - 1, isLeaf(this->children_));

    memcpy(
            lMemSmallerChunk,
            this->children,
            aIdxToRemove * lSizeNode
    );
    memcpy(
            lMemSmallerChunk + (aIdxToRemove * lSizeNode),
            this->children + ((aIdxToRemove + 1) * lSizeNode),
            (this->numKeys_ - 1 - aIdxToRemove) * lSizeNode
    );
    tmm_->release(this->children, this->num_keys, isLeaf(this->children));

    this->children = lMemSmallerChunk;
    memmove(
            &this->keys[aIdxToRemove],
            &this->keys[aIdxToRemove + 1],
            (this->num_keys - 1 - aIdxToRemove) * sizeof(Key_t)
    );
    this->numKeys_ -= 1;
}

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
void
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
CsbLeafNode_t::
remove(Key_t key, Tid_t tid) {
    uint16_t lIdxToRemove = idxToDescend(key, this->keys_, this->numKeys_);
    while (this->tids_[lIdxToRemove] != tid && lIdxToRemove < this->numKeys_ && this->keys_[lIdxToRemove] == key)
        lIdxToRemove++;
    if (this->tids_[lIdxToRemove] != tid) {
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
    this->numKeys_ -= 1;
}

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
void
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
remove(Key_t aKey, Tid_t aTid) {
    std::stack<CsbInnerNode_t *> *lPath;
    CsbInnerNode_t *lNodeParent;
    CsbLeafNode_t *lLeafNode;
    uint16_t lIdxParent = (lLeafNode - lNodeParent->children) / kSizeLeafNode;

    lNodeParent = lPath->top();
    lPath->pop();
    lLeafNode = findLeafNode(aKey, &lPath);

    if (aKey != getLargesKey_t(lLeafNode)) {
        lLeafNode->remove(aKey, aTid);
    } else {
        lLeafNode->remove(aKey, aTid);
        lNodeParent->keys[lIdxParent] = getLargesKey_t(lLeafNode);
    }
}


template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
std::string
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
CsbInnerNode_t::
asJson() {
    /*
     * "text": {"name": "Parent node"},
     *  "children": [....]
     *
     */

    std::string lStringKeys = "";
    std::string lStringChildren = "\"children\": [";
    // attach keys and children
    for (int32_t i = 0; i < numKeys_; i++) {
        if (i != 0) {
            lStringKeys.append(",");
        }
        lStringKeys.append(std::to_string(keys_[i]));
        lStringChildren.append(kThChildrenAsJson(i, children_));
        if (i + 1 < numKeys_) {
            lStringChildren.append(",");
        }
    }
    lStringChildren.append("]");


    return "{\"stackChildren\":true, \"text\": {\"name\": \"" + lStringKeys + "\"}," + lStringChildren + "}";;
}



template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
std::string
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
CsbLeafNode_t::
asJson() {
    /*
     * "text": {"name": "Parent node"},
     *  "children": [....]
     *
     */
    std::string lJsonObj = "{\"text\":{\"name\":\"";
    lJsonObj.append("keys: " + std::to_string(keys_[0]) + " - " + std::to_string(keys_[numKeys_ - 1]));
    return lJsonObj.append("\"}}");
}

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
std::string
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
getTreeAsJson() {
    if (isLeaf(root_)) {
        return "{\"nodeStructure\":" + ((CsbLeafNode_t *) root_)->asJson() + "}";
    }
    return "{\"nodeStructure\":" + ((CsbInnerNode_t *) root_)->asJson() + "}";
}

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
void
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
saveTreeAsJson(std::string aPath) {
    ;
    std::ofstream file(aPath);
    file << getTreeAsJson();
}

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
void
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
CsbLeafNode_t::
updatePointers(CsbLeafNode_t *preceding, CsbLeafNode_t *following) {
    this->preceding_node = preceding;
    this->following_node = following;
    preceding->following_node = &this;
    following->preceding_node = &this;
}

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
byte*
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
findLeafNode(Key_t aKey, std::stack<CsbInnerNode_t*> *aPath) {

    uint16_t            lIdxToDescend;
    uint32_t            lLevel       = 0;
    CsbInnerNode_t*     lNodeCurrent = (CsbInnerNode_t *) root_;

    // add root to the stack

    while (lLevel < this->depth_) {
        if (aPath != nullptr) {
            aPath->push(lNodeCurrent);
        }
        lIdxToDescend= idxToDescend(aKey, lNodeCurrent->keys_, lNodeCurrent->numKeys_);
        lNodeCurrent = (CsbInnerNode_t *) getKthNode(aResultIdx, lNodeCurrent->children_);
        lLevel++;
    }

    lIdxToDescend= idxToDescend(aKey, lNodeCurrent->keys_, lNodeCurrent->numKeys_);
    lNodeCurrent = (CsbInnerNode_t *) getKthNode(aResultIdx, lNodeCurrent->children_);

    return (byte*) lNodeCurrent;
}

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
Tid_t
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
find(Key_t aKey) {
    CsbLeafNode_t *lLeafNode = this->findLeafNode(aKey);
    uint16_t lIdxMatching = this->idxToDescend(aKey, lLeafNode->keys_, lLeafNode->numKeys_);
    if (lLeafNode->keys_[lIdxMatching] == aKey) {
        return lLeafNode->tids_[lIdxMatching];
    }
    return NULL;
}


template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
bool
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
isLeaf(uint32_t aDepth) {
    return (aDepth == depth_);
}

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
std::string
CsbTree_t<Key_t, Tid_t, kNumCac  ++
        heLinesPerInnerNode>::
kThChildrenAsJson(uint32_t aK, byte *aFirstChild) {
    if (isLeaf(aFirstChild)) {
        return ((CsbLeafNode_t *) getKthNode(aK, aFirstChild))->asJson();
    } else {
        return ((CsbInnerNode_t *) getKthNode(aK, aFirstChild))->asJson();
    }
}

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
byte *
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
getKthNode(uint16_t aK, byte *aNodeFirstChild) {
    return aNodeFirstChild + aK * kSizeNode;
}

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
uint16_t
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
idxToDescend(Key_t aKey, Key_t aKeys[], uint16_t aNumKeys) {
    if (aNumKeys == 0) return 0;
    for (uint16_t i = 0; i < aNumKeys; i++) {
        if (aKeys[i] >= aKey) {
            return i;
        }
    }
    return aNumKeys - 1;
}
#endif //CSBPLUSTREE_CSBPLUSTREE_CXX