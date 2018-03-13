#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <memory.h>
#include <fstream>
#include <string>
#include <iostream>


template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
CsbTree_t() {
    tmm_ = new TreeMemoryManager_t();
    root_ = tmm_->getMem(kNumMaxKeysInnerNode *kSizeNode);
    new(root_) CsbLeafEdgeNode_t;
    depth_ = 0;
    static_assert(kSizeNode == sizeof(CsbInnerNode_t));
    static_assert(kSizeNode == sizeof(CsbLeafNode_t));
    static_assert(kSizeNode == sizeof(CsbLeafEdgeNode_t));
    static_assert(kNumMaxKeysLeafEdgeNode > 0);
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
CsbLeafNode_t() {
    numKeys_ = 0;
}


template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
CsbLeafEdgeNode_t::
CsbLeafEdgeNode_t() {
    numKeys_ = 0;
}

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
void
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
CsbInnerNode_t::
shift(uint16_t aIdx) {
    if (this->numKeys_ + 1 > kNumMaxKeysInnerNode) {
        throw TooManyKeysException();
    }


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

    this->numKeys_ += 1;
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
void
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
insert(Key_t aKey, Tid_t aTid) {

    // find the leaf node to insert the key to and record the path upwards the tree
    std::stack<CsbInnerNode_t*>         lPath;
    byte*                               lPtrNodeLeaf;
    SearchResult_tt                     lSearchResult;
    bool                                lIsLeafEdge;
    bool                                lIsFull;


    findLeafNode(aKey, &lSearchResult, false, &lPath);
    lPtrNodeLeaf    = lSearchResult._node;
    lIsLeafEdge     = lSearchResult._idx == 0;
    lIsFull         = (lIsLeafEdge && ((CsbLeafEdgeNode_t*) lSearchResult._node)->numKeys_ == kNumMaxKeysLeafEdgeNode) ||
            (!lIsLeafEdge &&  ((CsbLeafNode_t*) lSearchResult._node)->numKeys_ == kNumMaxKeysLeafNode);


    // check if space is available and split if necessary
    if (lIsFull) {

        SplitResult_tt                 lSplitResult = {};
        split(lPtrNodeLeaf, this->depth_, &lPath, &lSplitResult);

        // determine which of resulting nodes is the node to insert an look up if it is a edge or not
        if (getLargestKey(lSplitResult._left, true, lSplitResult._edgeIndicator) < aKey) {
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
    lNumKeysLeftSplit = ceil(lNumKeysNodeToSplit / 2.0);

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

            if (lNodeRightGroup0->numKeys_ > kNumMaxKeysLeafEdgeNode) {
                uint16_t lNumKeysToMove = kNumMaxKeysLeafEdgeNode - lNodeRightGroup0->numKeys_;
                if (lNumKeysToMove + lNodeRightGroup1->numKeys_ > kNumMaxKeysLeafNode) {
                    // next node cannot catch all the keys

                    lNodeParentRight->shift(1);
                    // [ o | o | o |  |  |  ] -> [ o |   | o | o |  |  ]
                }
                lNodeRightGroup0->moveKeysAndTids(lNodeRightGroup1, lNodeRightGroup0->numKeys_ - lNumKeysToMove);
                lNodeParentRight->keys_[1] = lNodeRightGroup1->getLargestKey();
                lNodeParentRight->keys_[0] = lNodeRightGroup0->getLargestKey();
            }
            // now enough keys from the edge have been moved to safely convert the node to an edge
            lNodeRightGroup0->toLeafEdge();
            ((CsbLeafEdgeNode_t*) lNodeRightGroup0)->previous_ = (CsbLeafEdgeNode_t*) (lNodeParentRight - 1)->children_;
            (((CsbLeafEdgeNode_t*) getKthNode(0, (lNodeParentRight - 1)->children_)))->following_ = (CsbLeafEdgeNode_t*) lNodeRightGroup0;
        }

        // select the matching parent and reassign the pointer to the node as due to the split the location has changed
        if (lIdxNodeToSplit <= lParentSplit._splitIdx) {
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
            ((CsbLeafEdgeNode_t *) aNodeToSplit)->moveKeysAndTids((CsbLeafNode_t*) lNodeSplittedRight, lNumKeysLeftSplit);
            lNodeParent->keys_[lIdxNodeToSplit] = ((CsbLeafEdgeNode_t *) aNodeToSplit)->getLargestKey();
        } else {
            // LeafNode
            new (lNodeSplittedRight) CsbLeafNode_t();
            ((CsbLeafNode_t*) aNodeToSplit)->moveKeysAndTids((CsbLeafNode_t*) lNodeSplittedRight, lNumKeysLeftSplit);
            lNodeParent->keys_[lIdxNodeToSplit] = ((CsbLeafNode_t*) aNodeToSplit)->getLargestKey();
        }

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
CsbInnerNode_t::
remove(uint16_t aIdxToRemove) {
    // TODO remove method
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
asJson(uint16_t aDepth) {
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
        //lStringChildren.append(kThChildrenAsJson(i, children_));
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
    if (this->depth_ == 0) {
        return "{\"nodeStructure\":" + ((CsbLeafNode_t *) root_)->asJson() + "}";
    }
    return "{\"nodeStructure\":" + ((CsbInnerNode_t *) root_)->asJson(this->depth_) + "}";
}

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
std::string
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
kThChildrenAsJson(uint32_t aK, byte *aFirstChild, uint32_t aTreeDepth, uint32_t aNodeDepth) {
    if (aTreeDepth == aNodeDepth) {
        return ((CsbLeafNode_t *) getKthNode(aK, aFirstChild))->asJson();
    } else {
        return ((CsbInnerNode_t *) getKthNode(aK, aFirstChild))->asJson();
    }
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
findLeafNode(Key_t aKey, SearchResult_tt* aResult, bool aAbortEarly, std::stack<CsbInnerNode_t*>* aPath) {

    if (this->depth_ == 0) {
        aResult->_idx = 0;
        aResult->_node = this->root_;
        return;
    }

    uint16_t            lIdxToDescend;
    uint32_t            lLevel       = 0;
    CsbInnerNode_t*     lNodeCurrent = (CsbInnerNode_t *) root_;


    while (lLevel < this->depth_) {

        if (aPath != nullptr) {
            aPath->push(lNodeCurrent);
        }
        lIdxToDescend= idxToDescend(aKey, lNodeCurrent->keys_, lNodeCurrent->numKeys_);
        if (lIdxToDescend == UINT16_MAX) {
            if (aAbortEarly){
                aResult->_node == nullptr;
                return;
            } else {
                lIdxToDescend = lNodeCurrent->numKeys_ - 1;
            }
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


    aResult->_idx = 0;

    uint16_t            lIdxToDescend;
    uint32_t            lLevel       = 0;
    CsbInnerNode_t*     lNodeCurrent = (CsbInnerNode_t*) aResult->_node;

    while (lLevel < this->depth_) {
        aPath->push(lNodeCurrent);
        lIdxToDescend= idxToDescend(aKey, lNodeCurrent->keys_, lNodeCurrent->numKeys_);
        if (lIdxToDescend == (uint16_t) -1){
            lIdxToDescend = lNodeCurrent->numKeys_ - 1;
        }
        lNodeCurrent = (CsbInnerNode_t *) getKthNode(lIdxToDescend, lNodeCurrent->children_);
        lLevel++;
    }

    aResult->_node = (byte*) lNodeCurrent;
    aResult->_idx = lIdxToDescend;



    };

template<class Key_t, class Tid_t, uint16_t kNumCacheLinesPerInnerNode>
int32_t
CsbTree_t<Key_t, Tid_t, kNumCacheLinesPerInnerNode>::
find(Key_t aKey, Tid_t* aResult) {

    SearchResult_tt     lSearchResult;
    Key_t *             lKeys;
    uint16_t            lNumKeys;
    uint16_t            lIdxTid;



    this->findLeafNode(aKey, &lSearchResult, true);
    if (lSearchResult._node == nullptr) {
        return -1;
    }


    if (lSearchResult._idx == 0) {
        lKeys = ((CsbLeafEdgeNode_t*) lSearchResult._node)->keys_;
        lNumKeys = ((CsbLeafEdgeNode_t*) lSearchResult._node)->numKeys_;
    } else {
        lKeys = ((CsbLeafNode_t*) lSearchResult._node)->keys_;
        lNumKeys = ((CsbLeafNode_t*) lSearchResult._node)->numKeys_;
    }


    lIdxTid = idxToDescend(aKey, lKeys, lNumKeys);
    if (lIdxTid == (uint16_t) -1){
        return -1;
    }

    if (lSearchResult._idx == 0) {
        if (((CsbLeafEdgeNode_t*) lSearchResult._node)->keys_[lIdxTid] == aKey){
            *aResult =  ((CsbLeafEdgeNode_t*) lSearchResult._node)->tids_[lIdxTid];
            return 0;
        }
    } else {
        if (((CsbLeafNode_t*) lSearchResult._node)->keys_[lIdxTid] == aKey) {
             *aResult = ((CsbLeafNode_t *) lSearchResult._node)->tids_[lIdxTid];
            return 0;
        }
    }
    return -1;
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
    for (uint16_t i = 0; i <= aNumKeys; i++) {
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