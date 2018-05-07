#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <cstring>
#include <fstream>
#include <string>
#include <iostream>


template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
CsbTree_t() : depth_(0) {
    tmm_ = new TreeMemoryManager_t;
    root_ = tmm_->getMem(kNumMaxKeysInnerNode *kSizeNode);
    new(root_) CsbLeafEdgeNode_t;
    ((CsbLeafNode_t*) getKthNode(1, root_))->numKeys_ = 0; // stop marker
    static_assert(kSizeNode == sizeof(CsbInnerNode_t));
    static_assert(kSizeNode == sizeof(CsbLeafNode_t));
    static_assert(kSizeNode == sizeof(CsbLeafEdgeNode_t));
    static_assert(kNumMaxKeysLeafEdgeNode > 0);
    static_assert(kNumMaxKeysLeafNode >= (kNumMaxKeysLeafEdgeNode / 2));
    // TODO assert that a InnerNode has space for 2 more nodes after a split
}
template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
~CsbTree_t() {
    delete tmm_;
}



template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
CsbInnerNode_t::
CsbInnerNode_t() : numKeys_(0) {}

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
void
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
CsbInnerNode_t::
allocateChildNodes(TreeMemoryManager_t* aTmm) {
    this->children_ = aTmm->getMem(kNumMaxKeysInnerNode * kSizeNode);
}

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
uint16_t
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
CsbInnerNode_t::
getChildNodeIdx(byte *aChildNode) {
    return (aChildNode - this->children_) / kSizeNode;
}

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
bool
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
CsbInnerNode_t::
childIsEdge(byte* aChildNode) {
    return (this->getChildNodeIdx(aChildNode) == 0);
}

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
uint16_t
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
CsbInnerNode_t::
getChildMaxKeys(uint32_t aChildDepth, uint32_t aTreeDepth, byte *aChild) {
    if (aTreeDepth == aChildDepth) {
        if(childIsEdge(aChild)) {
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
getChildNumKeys(uint32_t aChildDepth, uint32_t aTreeDepth, byte *aChild) {
    return this->getChildNumKeys((aChildDepth == aTreeDepth), aChild);
}

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
uint16_t
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
CsbInnerNode_t::
getChildNumKeys(bool aIsLeaf, byte *aChild) {
    if (aIsLeaf) {
        if(childIsEdge(aChild)) {
            return ((CsbLeafEdgeNode_t*) aChild)->numKeys_;
        }
        return ((CsbLeafNode_t*) aChild)->numKeys_;
    }
    return ((CsbInnerNode_t*) aChild)->numKeys_;
}

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
Key_t
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
CsbInnerNode_t::
getLargestKey() {
    return this->keys_[this->numKeys_ -1 ];
}

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
Key_t
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
CsbLeafNode_t::
getLargestKey() {
    return this->keys_[this->numKeys_ - 1];
}

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
Key_t
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
CsbLeafEdgeNode_t::
getLargestKey() {
    return this->keys_[this->numKeys_ - 1 ];
}

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
bool
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
CsbInnerNode_t::
childIsFull(uint32_t aChildDepth, uint32_t aTreeDepth, byte *aChildNode) {
    return (this->getChildMaxKeys(aChildDepth, aTreeDepth, aChildNode) ==
            this->getChildNumKeys(aChildDepth, aTreeDepth, aChildNode));
}




template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
CsbLeafNode_t::
CsbLeafNode_t() : numKeys_(0) {}


template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
CsbLeafEdgeNode_t::
CsbLeafEdgeNode_t() : numKeys_(0), previous_(NULL), following_(NULL) {}

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
void
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
CsbInnerNode_t::
shift(uint16_t aIdx) {
    if (this->numKeys_ + 1 > kNumMaxKeysInnerNode) {
        throw TooManyKeysException();
    }
    if (aIdx <= this->numKeys_) {



        /*
         *   0   1   2   3
         * [ x | x | x |   ]
         * [   | x | x | x ]
         *   args: idx = 0
         */

        // shift keys_
        memmove(
                &this->keys_[aIdx + 1],
                &this->keys_[aIdx],
                sizeof(Key_t) * (this->numKeys_ - aIdx)
        );

        // shift the child nodes
        memmove(
                getKthNode(aIdx + 1, this->children_),
                getKthNode(aIdx, this->children_),
                kSizeNode * (this->numKeys_ - aIdx)
        );
    }
    this->numKeys_ += 1;
}

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
void
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
CsbInnerNode_t::
setStopMarker() {
    if (this->numKeys_ != kNumMaxKeysInnerNode){
        ((CsbLeafNode_t*) getKthNode(this->numKeys_, this->children_))->numKeys_ = 0;
    }
}



template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
void
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
CsbLeafNode_t::
insert(Key_t aKey, Tid_t aTid) {

    uint16_t lIdxToInsert = idxToInsert(aKey, this->keys_, this->numKeys_);

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
CsbLeafEdgeNode_t::
insert(Key_t aKey, Tid_t aTid) {

    uint16_t lIdxToInsert = idxToInsert(aKey, this->keys_, this->numKeys_);

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
const uint16_t
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
getCacheLinesPerNode() {
    return kNumCacheLinesPerInnerNode;
}

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
void
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
insert(std::pair<Key_t, Tid_t> aKeyVal) {
    insert(aKeyVal.first, aKeyVal.second);
}

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
void
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
insert(Key_t aKey, Tid_t aTid) {

    // find the leaf node to insert the key to and record the path upwards the tree
    std::stack<CsbInnerNode_t*>         lPath;
    byte*                               lPtrNodeLeaf;
    SearchResult_tt                     lSearchResult;
    bool                                lIsLeafEdge;
    bool                                lIsFull;
    bool                                lRevisitRoot;


    findLeafForInsert(aKey, &lSearchResult, &lPath);
    lPtrNodeLeaf    = lSearchResult._node;
    lIsLeafEdge     = lSearchResult._idx == 0;
    lIsFull         = (lIsLeafEdge && ((CsbLeafEdgeNode_t*) lSearchResult._node)->numKeys_ == kNumMaxKeysLeafEdgeNode) ||
            (!lIsLeafEdge &&  ((CsbLeafNode_t*) lSearchResult._node)->numKeys_ == kNumMaxKeysLeafNode);
    lRevisitRoot    = lIsFull && this->depth_ == 0;

    // check if space is available and split if necessary
    if (lIsFull) {

        SplitResult_tt                 lSplitResult = {};
        split(lPtrNodeLeaf, this->depth_, &lPath, &lSplitResult);

        // determine which of resulting nodes is the node to insert an look up if it is a edge or not
        if (getLargestKey(lSplitResult._left, true, lSplitResult._edgeIndicator) < aKey) {
            /*
             * if root has been splitted, is now an InnerNode and as the aKey could be larger
             * than the largest key seen so far, we update it
             */
            if (lRevisitRoot) {
                findLeafForInsert(aKey, &lSearchResult, &lPath);
            }
            lPtrNodeLeaf = lSplitResult._left + kSizeNode;
            lIsLeafEdge = false;
        } else {
            lPtrNodeLeaf = lSplitResult._left;
            lIsLeafEdge = lSplitResult._edgeIndicator;
        }
    }

    if (lIsLeafEdge) {
        ((CsbLeafEdgeNode_t*) lPtrNodeLeaf)->insert(aKey, aTid);
    } else {
        ((CsbLeafNode_t*) lPtrNodeLeaf)->insert(aKey, aTid);
    }
}

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
void
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
CsbInnerNode_t::
moveKeysAndChildren(CsbInnerNode_t* aNodeTarget, uint16_t aNumKeysRemaining) {
    memcpy(
            aNodeTarget->keys_,
            &this->keys_[aNumKeysRemaining],
            sizeof(Key_t) * (this->numKeys_ - aNumKeysRemaining)
    ); // keys
    memcpy(
            aNodeTarget->children_,
            getKthNode(aNumKeysRemaining, this->children_),
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
CsbLeafNode_t::
leftInsertKeysAndTids(CsbLeafNode_t *aNodeSource, uint16_t aNumKeys) {

    /*
     * _ move existing keys
     * _ copy keys then tids from source node
     * _ set numKeys at source and this
     */

    if (this->numKeys_ + aNumKeys > kNumMaxKeysLeafNode)
        throw TooManyKeysException();

    // move existing keys and tids
    memmove(
            &this->keys_[aNumKeys],
            this->keys_,
            this->numKeys_ * sizeof(Key_t)
    ); // keys_

    memmove(
            &this->tids_[aNumKeys],
            this->tids_,
            this->numKeys_ * sizeof(Tid_t)
    ); // tids_

    this->numKeys_ += aNumKeys;

    // copy the keys and tids from the source node to this node
    memmove(
            this->keys_,
            &aNodeSource->keys_[aNodeSource->numKeys_ - aNumKeys],
            aNumKeys * sizeof(Key_t)
    );
    memmove(
            this->tids_,
            &aNodeSource->tids_[aNodeSource->numKeys_ - aNumKeys],
            aNumKeys * sizeof(Tid_t)
    );

    aNodeSource->numKeys_ -= aNumKeys;
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
CsbLeafNode_t::
toLeafEdge() {
    if (this->numKeys_ > kNumMaxKeysLeafEdgeNode){
        throw TooManyKeysException();
    }
    memmove(
            ((CsbLeafEdgeNode_t*) this)->tids_,
            this->tids_,
            sizeof(Tid_t) * this->numKeys_
    );
    ((CsbLeafEdgeNode_t*) this)->numKeys_ = this->numKeys_;
}


template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
void
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
split(byte* aNodeToSplit, uint32_t aDepth,  std::stack<CsbInnerNode_t*>* aPath, SplitResult_tt* aResult){


    CsbInnerNode_t* lNodeParent;
    byte*           lNodeSplittedRight;
    uint16_t        lNumKeysNodeToSplit;
    uint16_t        lNumKeysLeftSplit;
    uint16_t        lIdxNodeToSplit;
    const bool      lIsLeaf = isLeaf(aDepth);



    if (this->root_ == aNodeToSplit) {
        // split is called on root_
        lNodeParent = new(tmm_->getMem(kNumMaxKeysInnerNode * kSizeNode)) CsbInnerNode_t();
        lNodeParent->children_ = this->root_;
        lNodeParent->numKeys_ = 1;
        lNodeParent->keys_[0] = getLargestKey(this->root_, this->depth_ == 0, true);
        depth_ += 1;
        aDepth += 1;
        this->root_ = (byte *) lNodeParent;
    } else {
        lNodeParent = aPath->top();
        aPath->pop();
    }

    lIdxNodeToSplit = lNodeParent->getChildNodeIdx(aNodeToSplit);
    lNumKeysNodeToSplit = lNodeParent->getChildNumKeys(lIsLeaf, aNodeToSplit);
    lNumKeysLeftSplit = (lNumKeysNodeToSplit + 1 )/ 2;

    // case if parent node is full
    if (lNodeParent->numKeys_ == kNumMaxKeysInnerNode) {

        SplitResult_tt lParentSplit;

        split((byte *) lNodeParent, aDepth - 1, aPath, &lParentSplit);

        if (lIsLeaf) {
            /*
             *
             * if the split occurs at level aDepth = tree->depth -1 the following level
             * will contain LeafNodes. Due to the split, a new node group was created and
             * the nodes have been moved to this node group.
             * Subsequently the node at index = 0 at the right node group has to be
             * converted to an EdgeNode and the overflowing keys need to be moved to
             * either the following node or a new node if the following does not have
             * the space
             *
             *                 lNodeParentRight
             *                                v
             *                          [ o | o | o | ... ]
             *                                 \
             *                                  \
             * [ e | o | o | o |   | ... ]     [ o | o | o |   |   | ... ]
             *                                   ^   ^
             *                    lNodeRightGroup0   |
             *                        lNodeRightGroup1
             */

            CsbInnerNode_t* lNodeParentRight    = ((CsbInnerNode_t*) lParentSplit._left) + 1;
            CsbLeafNode_t*  lNodeRightGroup0    = (CsbLeafNode_t*) getKthNode(0, lNodeParentRight->children_);
            CsbLeafNode_t*  lNodeRightGroup1    = (CsbLeafNode_t*) getKthNode(1, lNodeParentRight->children_);
            CsbLeafEdgeNode_t *lLeafEdgePrevious, *lLeafEdgeMiddle, *lLeafEdgeFollowing;

            if (lNodeRightGroup0->numKeys_ > kNumMaxKeysLeafEdgeNode) {
                // basically we do a simplified split on the node. We can assume to have space in the parent as it has just been splitted

                lNodeParentRight->shift(1);
                // [ o | o | o |  |  |  ] -> [ o |   | o | o |  |  ]
                new(lNodeRightGroup1) CsbLeafNode_t;
                lNodeParentRight->keys_[1] = lNodeParentRight->keys_[0];
                lNodeRightGroup0->moveKeysAndTids(lNodeRightGroup1, (kNumMaxKeysLeafEdgeNode + 1) / 2);
                lNodeParentRight->keys_[0] = lNodeRightGroup0->getLargestKey();

                if (lIdxNodeToSplit > lParentSplit._splitIdx) {
                    lIdxNodeToSplit += 1;
                }
            }
            // now enough keys from the edge have been moved to safely convert the node to an edge
            lNodeRightGroup0->toLeafEdge();

            // now we have to adjust the pointers
            lLeafEdgeMiddle     = (CsbLeafEdgeNode_t*) lNodeRightGroup0;
            lLeafEdgePrevious   = (CsbLeafEdgeNode_t*) (lNodeParentRight - 1)->children_;
            lLeafEdgeFollowing  = lLeafEdgePrevious->following_;

            lLeafEdgePrevious   ->following_    = lLeafEdgeMiddle;
            lLeafEdgeMiddle     ->previous_     = lLeafEdgePrevious;
            lLeafEdgeMiddle     ->following_    = lLeafEdgeFollowing;
            if (lLeafEdgeFollowing != nullptr){
                lLeafEdgeFollowing  ->previous_     = lLeafEdgeMiddle;
            }


            ((CsbInnerNode_t*) lParentSplit._left)->setStopMarker();
            lNodeParentRight->setStopMarker();
            if (lIdxNodeToSplit == lParentSplit._splitIdx ){
                aResult->_left = (byte*) lNodeRightGroup0;
                aResult->_splitIdx = 0;
                aResult->_edgeIndicator = true;
                return;
            }
        }

        // select the matching parent and reassign the pointer to the node as due to the split the location has changed
        if (lIdxNodeToSplit < lParentSplit._splitIdx) {
            lNodeParent = (CsbInnerNode_t*) lParentSplit._left;

        } else {
            lNodeParent = ((CsbInnerNode_t*) lParentSplit._left) + 1;
            lIdxNodeToSplit = lIdxNodeToSplit - lParentSplit._splitIdx;
        }
        aNodeToSplit = getKthNode(lIdxNodeToSplit, lNodeParent->children_);
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
    lNodeParent->keys_[lIdxNodeToSplit + 1] = lNodeParent->keys_[lIdxNodeToSplit];

    if (lIsLeaf) {
        if (lNodeParent->childIsEdge(aNodeToSplit)) {
        // LeafEdgeNode
            /*
             *   v
             * [ e |   | o | ....]
             *  edge:
             *  _ create new LeafNode
             *  _ node remains an edge, simply move the keys_ and tids_
             *
             */
            aResult->_edgeIndicator = true;
            new(lNodeSplittedRight) CsbLeafNode_t();
            lNumKeysLeftSplit = ((CsbLeafEdgeNode_t *) aNodeToSplit)->numKeys_ / 2;
            ((CsbLeafEdgeNode_t *) aNodeToSplit)->moveKeysAndTids((CsbLeafNode_t*) lNodeSplittedRight, lNumKeysLeftSplit);
            lNodeParent->keys_[lIdxNodeToSplit] = ((CsbLeafEdgeNode_t *) aNodeToSplit)->getLargestKey();
        } else {
            // LeafNode
            new (lNodeSplittedRight) CsbLeafNode_t();
            ((CsbLeafNode_t*) aNodeToSplit)->moveKeysAndTids((CsbLeafNode_t*) lNodeSplittedRight, lNumKeysLeftSplit);
            lNodeParent->keys_[lIdxNodeToSplit] = ((CsbLeafNode_t*) aNodeToSplit)->getLargestKey();
        }
        lNodeParent->setStopMarker();

    } else {
        // InnerNode
        new (lNodeSplittedRight) CsbInnerNode_t();
        ((CsbInnerNode_t*) lNodeSplittedRight)->allocateChildNodes(tmm_);
        ((CsbInnerNode_t*) aNodeToSplit)->moveKeysAndChildren((CsbInnerNode_t*) lNodeSplittedRight, lNumKeysLeftSplit);
        lNodeParent->keys_[lIdxNodeToSplit] = ((CsbInnerNode_t*) aNodeToSplit)->getLargestKey();

    }

    aResult->_left              = aNodeToSplit;
    aResult->_splitIdx          = lNumKeysLeftSplit;
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
    // TODO remove method
}


template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
std::string
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
CsbInnerNode_t::
asJson(uint32_t aDepth, uint32_t aTreeDepth) {
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
        lStringChildren.append(kThChildrenAsJson(i, children_, aDepth + 1, aTreeDepth));
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
    lJsonObj.append("keys: " + std::to_string(keys_[0]) + " - " + std::to_string(keys_[numKeys_ - 1]) + "; #" + std::to_string(this->numKeys_));
    return lJsonObj.append("\"}}");
}

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
std::string
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
CsbLeafEdgeNode_t::
asJson() {
    /*
     * "text": {"name": "Parent node"},
     *  "children": [....]
     *
     */
    std::string lJsonObj = "{\"text\":{\"name\":\"";
    lJsonObj.append("keys: " + std::to_string(keys_[0]) + " - " + std::to_string(keys_[numKeys_ - 1]) + "; #" + std::to_string(this->numKeys_));
    return lJsonObj.append("\"}}");
}

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
std::string
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
getTreeAsJson() {
    if (this->depth_ == 0) {
        return "{\"nodeStructure\":" + ((CsbLeafEdgeNode_t *) root_)->asJson() + "}";
    }
    return "{\"nodeStructure\":" + ((CsbInnerNode_t*) this->root_)->asJson(0, this->depth_) + "}";
}

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
std::string
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
kThChildrenAsJson(uint32_t aK, byte *aFirstChild, uint32_t aNodeDepth, uint32_t aTreeDepth) {
    if (aTreeDepth == aNodeDepth) {
        if (aK == 0){
            return ((CsbLeafEdgeNode_t*) getKthNode(aK, aFirstChild))->asJson();
        }
        return ((CsbLeafNode_t *) getKthNode(aK, aFirstChild))->asJson();
    }
    return ((CsbInnerNode_t*) getKthNode(aK, aFirstChild))->asJson(aNodeDepth, aTreeDepth);
}

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
void
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
saveTreeAsJson(std::string aPath) {
    std::ofstream file(aPath);
    file << getTreeAsJson();
}
template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
void
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
getMemoryUsage() {
    this->tmm_->printUsage();
}

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
void
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
findLeafNode(Key_t aKey, SearchResult_tt* aResult) {

    if (this->depth_ == 0) {
        aResult->_idx = 0;
        aResult->_node = this->root_;
        return;
    }

    uint16_t            lIdxToDescend;
    uint32_t            lLevel       = 0;
    CsbInnerNode_t*     lNodeCurrent = (CsbInnerNode_t *) root_;


    while (lLevel < this->depth_) {

        lIdxToDescend= idxToDescend(aKey, lNodeCurrent->keys_, lNodeCurrent->numKeys_);
        if (lIdxToDescend == UINT16_MAX) {
            aResult->_node = nullptr;
            return;
        }
        lNodeCurrent = (CsbInnerNode_t *) getKthNode(lIdxToDescend, lNodeCurrent->children_);
        lLevel++;
    }

    aResult->_node = (byte*) lNodeCurrent;
    aResult->_idx  = lIdxToDescend;

 }


template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
void
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
findLeafForInsert(Key_t aKey, SearchResult_tt* aResult, std::stack<CsbInnerNode_t*>* aPath) {


    uint16_t            lIdxToDescend;
    uint32_t            lLevel       = 0;
    CsbInnerNode_t*     lNodeCurrent = (CsbInnerNode_t*) this->root_;

    while (lLevel < this->depth_) {
        aPath->push(lNodeCurrent);
        lIdxToDescend= idxToDescend(aKey, lNodeCurrent->keys_, lNodeCurrent->numKeys_);
        if (lIdxToDescend == UINT16_MAX) {
            lIdxToDescend = lNodeCurrent->numKeys_ - 1;
            lNodeCurrent->keys_[lIdxToDescend] = aKey;
        }

        lNodeCurrent = (CsbInnerNode_t *) getKthNode(lIdxToDescend, lNodeCurrent->children_);
        lLevel++;
    }

    aResult->_node = (byte*) lNodeCurrent;
    aResult->_idx = lIdxToDescend;

};

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
Tid_t*
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
findRef(Key_t aKey) {

    SearchResult_tt     lSearchResult;
    Key_t *             lKeys;
    uint16_t            lNumKeys;
    uint16_t            lIdxTid;



    this->findLeafNode(aKey, &lSearchResult);
    if (lSearchResult._node == nullptr) {
        return nullptr;
    }


    if (lSearchResult._idx == 0) {
        lKeys = ((CsbLeafEdgeNode_t*) lSearchResult._node)->keys_;
        lNumKeys = ((CsbLeafEdgeNode_t*) lSearchResult._node)->numKeys_;
    } else {
        lKeys = ((CsbLeafNode_t*) lSearchResult._node)->keys_;
        lNumKeys = ((CsbLeafNode_t*) lSearchResult._node)->numKeys_;
    }


    lIdxTid = idxToDescend(aKey, lKeys, lNumKeys);
    if (lIdxTid == UINT16_MAX){
        return nullptr;
    }

    if (lSearchResult._idx == 0) {
        if (((CsbLeafEdgeNode_t*) lSearchResult._node)->keys_[lIdxTid] == aKey){
            return &((CsbLeafEdgeNode_t*) lSearchResult._node)->tids_[lIdxTid];
        }
    } else {
        if (((CsbLeafNode_t*) lSearchResult._node)->keys_[lIdxTid] == aKey) {
            return &((CsbLeafNode_t *) lSearchResult._node)->tids_[lIdxTid];
        }
    }
    return nullptr;
}


template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
typename CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::iterator
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
find(Key_t aKey) {
    return (iterator) findRef(aKey);
}

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
int32_t
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
find(Key_t aKey, Tid_t* aResult) {
    *aResult = *findRef(aKey);
    if (aResult == nullptr) return -1;
    return 0;

}

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
Tid_t
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
operator[](Key_t aKey) {
    return *findRef(aKey);
}


template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
bool
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
isLeaf(uint32_t aDepth) {
    return (aDepth == this->depth_);
}

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
Key_t
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
getLargestKey(byte* aNode, bool aChild, bool aEdge) {
    if (aChild){
        if (aEdge) {
            return ((CsbLeafEdgeNode_t*) aNode)->getLargestKey();
        }
        return ((CsbLeafNode_t*) aNode)->getLargestKey();
    }
    return ((CsbInnerNode_t*) aNode)->getLargestKey();
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
    return -1;
}

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
uint16_t
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
idxToInsert(Key_t aKey, Key_t aKeys[], uint16_t aNumKeys) {
    if (aNumKeys == 0) return 0;
    for (uint16_t i = 0; i < aNumKeys; i++) {
        if (aKeys[i] >= aKey) {
            return i;
        }
    }
    return aNumKeys;
}

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
uint64_t
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
getNumKeys() {
    if (isLeaf(0)){
        return ((CsbLeafEdgeNode_t*) this->root_)->numKeys_;
    }

    CsbInnerNode_t*     lNodeCurrent        = (CsbInnerNode_t *) this->root_;
    CsbLeafEdgeNode_t*  lLeafEdgeCurrent;
    CsbLeafEdgeNode_t*  lLeafEdgeFollowing;
    uint64_t            lNumKeysTotal       = 0;

    for (uint64_t iDepth = 0; iDepth < this->depth_; iDepth++){
        lNodeCurrent = (CsbInnerNode_t*) lNodeCurrent->children_;
    }

    lLeafEdgeCurrent = (CsbLeafEdgeNode_t*) lNodeCurrent;

    do {
        lLeafEdgeFollowing = lLeafEdgeCurrent->following_;
        lNumKeysTotal += lLeafEdgeCurrent->numKeys_;
        for (uint16_t n = 1; n < kNumMaxKeysInnerNode; n++){
            CsbLeafNode_t* lLeafCurrent = (CsbLeafNode_t*) lLeafEdgeCurrent + n;
            uint16_t lNumKeysCurrent = lLeafCurrent->numKeys_;
            if (lNumKeysCurrent == 0) {
                break;
            }
            else {
                lNumKeysTotal += lNumKeysCurrent;
            }
        }
        lLeafEdgeCurrent = lLeafEdgeFollowing;
    } while (lLeafEdgeCurrent != nullptr);
    return lNumKeysTotal;
}

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
uint64_t
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
getNumKeysBackwards() {
    if (isLeaf(0)){
        return ((CsbLeafEdgeNode_t*) this->root_)->numKeys_;
    }

    CsbInnerNode_t*     lNodeCurrent        = (CsbInnerNode_t *) this->root_;
    CsbLeafEdgeNode_t*  lLeafEdgeCurrent;
    CsbLeafEdgeNode_t*  lLeafEdgePrevious;
    uint64_t            lNumKeysTotal       = 0;

    for (uint64_t iDepth = 1; iDepth < this->depth_; iDepth++){
        lNodeCurrent = (CsbInnerNode_t*) getKthNode(lNodeCurrent->numKeys_ - 1, lNodeCurrent->children_);
    }

    lLeafEdgeCurrent = (CsbLeafEdgeNode_t*) lNodeCurrent->children_;

    do {
        lLeafEdgePrevious = lLeafEdgeCurrent->previous_;
        lNumKeysTotal += lLeafEdgeCurrent->numKeys_;
        for (uint16_t n = 1; n < kNumMaxKeysInnerNode; n++){
            CsbLeafNode_t* lLeafCurrent = (CsbLeafNode_t*) lLeafEdgeCurrent + n;
            uint16_t lNumKeysCurrent = lLeafCurrent->numKeys_;
            if (lNumKeysCurrent == 0) {
                break;
            }
            else {
                lNumKeysTotal += lNumKeysCurrent;
            }
        }
        lLeafEdgeCurrent = lLeafEdgePrevious;
    } while (lLeafEdgeCurrent != nullptr);
    return lNumKeysTotal;
}


template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
bool
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
verifyOrder() {

    CsbInnerNode_t*      lNodeCurrent        = (CsbInnerNode_t*) this->root_;
    CsbLeafEdgeNode_t*  lLeafEdgeCurrent;
    CsbLeafNode_t*      lLeafCurrent;
    CsbLeafEdgeNode_t*  lLeafEdgeFollowing;
    Key_t               lPreviousKey;
    bool                lFirstKey           = true;


    // walk to the leftmost LeafEdgeNode
    for (uint64_t iDepth = 0; iDepth < this->depth_; iDepth++){
        lNodeCurrent = (CsbInnerNode_t*) lNodeCurrent->children_;
    }

    lLeafEdgeCurrent = (CsbLeafEdgeNode_t*) lNodeCurrent;



    // iterate over the leaf node groups
    do {
        // remember the following LeafEdge
        lLeafEdgeFollowing = lLeafEdgeCurrent->following_;
        // iterate over the LeafEdge's keys
        for (uint16_t i = 0; i<lLeafEdgeCurrent->numKeys_; i++){
            if (lLeafEdgeCurrent->keys_[i] <= lPreviousKey && !lFirstKey){
                return false;
            } else{
                lFirstKey = false;
                lPreviousKey = lLeafEdgeCurrent->keys_[i];
            }
        }
        // iterate over the LeafNodes
        uint16_t n = 1;
        CsbLeafNode_t* lLeafCurrent = (CsbLeafNode_t*) getKthNode(n, (byte*) lLeafEdgeCurrent);
        while (n < kNumMaxKeysInnerNode && lLeafCurrent->numKeys_ != 0){
            for (uint16_t i = 0; i<lLeafCurrent->numKeys_; i++){
                if (lLeafCurrent->keys_[i] <= lPreviousKey && !lFirstKey)
                    return false;
                lPreviousKey = lLeafCurrent->keys_[i];
            }
            n++;
            lLeafCurrent++;
        }
        lLeafEdgeCurrent = lLeafEdgeFollowing;
    } while (lLeafEdgeCurrent != nullptr);
    return true;
}