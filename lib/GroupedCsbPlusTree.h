#ifndef CSBPLUSTREE_CSBPLUSTREE_H
#define CSBPLUSTREE_CSBPLUSTREE_H

#define CACHE_LINE_SIZE 64
#define FIXED_SIZE 12
#define LEAF_FIXED_SIZE 20
#define NUM_INNER_NODES_TO_ALLOC 1000
#define CHUNK_SIZE 100

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stack>
#include <memory.h>
#include <cstddef>
#include "ChunkrefMemoryHandler.h"


typedef std::byte byte;

namespace GroupedCsbPlusTree {

    template<class Key_t, class Tid_t, uint16_t kNumCacheLines, uint16_t kSizeCacheLine>

    static constexpr bool           kBestFit = True;
    static constexpr uint16_t       kSizeChunk = 1000;

    static constexpr uint16_t       kSizeInnerNode_t = kSizeCacheLine * kNumCacheLines;
    static constexpr uint16_t       kSizeFixedInnerNode_t = 10;
    static constexpr uint16_t       kMaxKeysInnerNode_t = (kSizeInnerNode_t - kSizeFixedInnerNode_t) / sizeof(Key_t);
    static constexpr uint16_t       kSizePaddingInnerNode_t = kSizeFixedInnerNode_t - (kMaxKeysInnerNode_t * sizeof(Key_t)) - kSizeFixedInnerNode_t;

    static constexpr uint16_t       kSizeLeafNode_t = kSizeCacheLine * kNumCacheLines;
    static constexpr uint16_t       kSizeFixedLeafNode_t = 2;
    static constexpr uint16_t       kMaxKeysLeafNode_t = (kSizeLeafNode_t - kSizeFixedLeafNode_t) / (sizeof(Key_t) + sizeof(Tid_t));
    static constexpr uint16_t       kSizePaddingLeafNode_t = kSizeFixedLeafNode_t - (kMaxKeysLeafNode_t * (sizeof(Key_t) + sizeof(Tid_t))) - kSizeFixedLeafNode_t;

    struct CsbTree {

        using TreeMemoryManager_t = ChunkRefMemoryHandler::MemoryHandler_t< kSizeChunk, kSizeCacheLine, kBestFit>;

        byte * root;
        TreeMemoryManager_t * tmm_;

        CsbTree() {
            tmm_ = new TreeMemoryManager_t();
            //TODO
            // root =
        }


        class InnerNodeGroup_t {
        public:
            CsbInnerNode members_[];

            InnerNodeGroup_t(uint16_t aNumNodes){
                InnerNodeGroup_t * lNg = (InnerNodeGroup_t *) tmm_->getMem(kSizeInnerNode_t * aNumNodes);
                setLeafIndicator(0, aNumNodes);
            }

            void setLeafIndicator(uint8_t aIsLeaf, uint16_t aNumNodes){
                (uint8_t) members_[aNumNodes + 1] = aIsLeaf;
            }

            uint8_t getLeafIndicator(aNumNodes){
                return (uint8_t) members_[aNumNodes + 1];
            }
        };

        class LeafNodeGroup_t {
            CsbLeafNode* members_;

            LeafNodeGroup_t(uint16_t aNumNodes)  {
                LeafNodeGroup_t lNg = (LeafNodeGroup_t *) tmm_->getMem(kSizeLeafNode_t + kSizeFixedLeafNode_t);
            };

            void setLeafIndicator(uint8_t aIsLeaf, uint16_t aNumNodes){
                (uint8_t) members_[aNumNodes + 1] = aIsLeaf;
            }

            uint8_t getLeafIndicator(uint16_t aNumNodes){
                return (uint8_t) members_[aNumNodes + 1];
            }

            LeafNodeGroup_t* getPrecedingNodeGroup(uint16_t aNumNodes){
                return  (LeafNodeGroup_t *) (((byte*) members_ + aNumNodes + 1) + 1);
            }

            uint8_t getLeafIndicator(aNumNodes){
                return (uint8_t) members_[aNumNodes + 1];
            }





        };

        struct CsbInnerNode {
            tKey keys[max_keys];
            uint16_t num_keys;                // 2B
            byte *children;                   // 8B
            uint8_t free[kSizePaddingInnerNode_t];  // padding


            CsbInnerNode() {
                num_keys = 0;
                leaf = false;
            };

            /*
             * this function shifts all nodes past the insert index within the parents node group
             */

            byte *insert(uint16_t idx_to_insert, TreeMemoryManager *aTmm) {

                uint32_t node_size;
                byte *memptr;
                byte *node_to_insert;

                node_size = (isLeaf(this->children)) ? total_leaf_node_size : total_inner_node_size;

                // allocate new node group, 1 node larger than the old one
                memptr = aTmm->getMem(this->num_keys + 1, isLeaf(this->children));
                // copy values to new group
                memcpy(memptr, this->children, idx_to_insert * node_size); // leading nodes
                memcpy(memptr + node_size * (idx_to_insert + 1), this->children + node_size * (idx_to_insert),
                       (this->num_keys - idx_to_insert) * node_size); // trailing nodes
                // shift keys in parent
                memmove(&this->keys[idx_to_insert + 1], &this->keys[idx_to_insert],
                        sizeof(tKey) * (this->num_keys - idx_to_insert));

                aTmm->release(this->children, this->num_keys, isLeaf(this->children));
                // update the child ptr
                this->children = memptr;

                if (isLeaf(this->children)) {
                    node_to_insert = (byte *) new(memptr + node_size * idx_to_insert) CsbLeafNode();
                } else {
                    node_to_insert = (byte *) new(memptr + node_size * idx_to_insert) CsbInnerNode();
                }

                return node_to_insert -
                       node_size; // returns a pointer to the left node, where the node group's containing child nodes have been copied from
            }

            void splitKeys(CsbInnerNode *split_node, uint16_t num_keys_remaining) {
                memmove(&split_node->keys, &this->keys[num_keys_remaining],
                        sizeof(tKey) * (this->num_keys - num_keys_remaining)); // keys
                split_node->num_keys = this->num_keys - num_keys_remaining;
                split_node->children = getKthNode(num_keys_remaining, this->children);
                this->num_keys = num_keys_remaining;
            }

            void remove(uint16_t remove_idx) {
                uint32_t child_node_size = (childrenIsLeaf(this)) ? total_leaf_node_size : total_inner_node_size;
                byte *memptr = _tmm->getMem(this->num_keys, isLeaf(this->children));

                memcpy(memptr, this->children, remove_idx * child_node_size);
                memcpy(memptr + (remove_idx * child_node_size), this->children + ((remove_idx + 1) * child_node_size),
                       (this->num_keys - 1 - remove_idx) * child_node_size);
                _tmm->release(this->children, this->num_keys, isLeaf(this->children));

                this->children = memptr;
                memmove(&this->keys[remove_idx], &this->keys[remove_idx + 1],
                        (this->num_keys - 1 - remove_idx) * sizeof(tKey));
                this->num_keys -= 1;
            }

        } __attribute__((packed));


        struct InnerNodeGroup {

        };

        struct CsbLeafNode {
            tKey keys[max_keys];
            uint16_t num_keys;                // 2B
            tTid tids[max_keys];
            uint8_t free[num_free_bytes_leaf];   // padding



            CsbLeafNode() {
                leaf = true;
                num_keys = 0;
            };

            void updatePointers(CsbLeafNode *preceding, CsbLeafNode *following) {
                this->preceding_node = preceding;
                this->following_node = following;
                preceding->following_node = &this;
                following->preceding_node = &this;
            }

            void insert(tKey key, tTid tid) {

                uint16_t insert_idx = idxToDescend(key, this->keys, this->num_keys);
                memmove(&this->keys[insert_idx + 1],
                        &this->keys[insert_idx],
                        (this->num_keys - insert_idx) * sizeof(tKey)
                );
                memmove(&this->tids[insert_idx + 1],
                        &this->tids[insert_idx],
                        (this->num_keys - insert_idx) * sizeof(tTid)
                );
                this->keys[insert_idx] = key;
                this->tids[insert_idx] = tid;
                this->num_keys += 1;
            }

            void splitKeysAndTids(CsbLeafNode *split_node, uint16_t num_keys_remaining) {
                memmove(&split_node->keys, &this->keys[num_keys_remaining],
                        sizeof(tKey) * (this->num_keys - num_keys_remaining)); // keys
                memmove(&split_node->tids, &this->tids[num_keys_remaining],
                        sizeof(tTid) * (this->num_keys - num_keys_remaining)); // tids
                split_node->num_keys = this->num_keys - num_keys_remaining;
                this->num_keys = num_keys_remaining;
            }

            void remove(tKey key, tTid tid) {
                uint16_t remove_idx = idxToDescend(key, this->keys, this->num_keys);
                while (this->tids[remove_idx] != tid && remove_idx < this->num_keys && this->keys[remove_idx] == key)
                    remove_idx++;
                if (this->tids[remove_idx] != tid) {
                    return;
                }
                memmove(this->keys[remove_idx], this->keys[remove_idx + 1],
                        (this->num_keys - 1 - remove_idx) * sizeof(tKey));
                memmove(this->tids[remove_idx], this->tids[remove_idx + 1],
                        (this->num_keys - 1 - remove_idx) * sizeof(tTid));
                this->num_keys -= 1;
            }
        } __attribute__((packed));


        CsbLeafNode *findLeafNode(tKey key, std::stack<CsbInnerNode *> *path = nullptr) {
            CsbInnerNode *node = (CsbInnerNode *) root;
            while (!node->leaf) {
                uint16_t idx_to_descend = this->idxToDescend(key, node->keys, node->num_keys);
                if (path != nullptr) path->push(node);
                node = (CsbInnerNode *) getKthNode(idx_to_descend, node->children);
            }
            return (CsbLeafNode *) node;
        }

        tTid find(tKey key) {
            CsbLeafNode *leaf = this->findLeafNode(key);
            uint16_t match_idx = this->idxToDescend(key, leaf->keys, leaf->num_keys);
            if (leaf->keys[match_idx] == key) return leaf->tids[match_idx];
            return NULL;
        }

        void insert(tKey key, tTid tid) {

            // find the leaf node to insert the key to and record the path upwards the tree
            std::stack < CsbInnerNode * > path;
            CsbLeafNode *leaf_to_insert = findLeafNode(key, &path);

            // if there is space, just insert
            if (!isFull((byte *) leaf_to_insert)) {
                leaf_to_insert->insert(key, tid);
                return;
            }
            // else split the node and see in which of the both nodes that have been split the key fits

            leaf_to_insert = (CsbLeafNode *) split((byte *) leaf_to_insert, &path);
            if (key > leaf_to_insert->keys[leaf_to_insert->num_keys - 1]) {
                (leaf_to_insert + 1)->insert(key, tid);
            } else {
                leaf_to_insert->insert(key, tid);
            }
        }

        void remove(tKey key, tTid tid) {
            std::stack < CsbInnerNode * > *path;
            CsbInnerNode *parent;
            CsbLeafNode *leaf;
            uint16_t parent_idx = (leaf - parent->children) / total_leaf_node_size;

            parent = path->top();
            path->pop();
            leaf = findLeafNode(key, &path);

            if (key != getLargestKey(leaf)) {
                leaf->remove(key, tid);
            } else {
                leaf->remove(key, tid);
                parent->keys[parent_idx] = getLargestKey(leaf);
            }
        }

        static uint16_t isLeaf(byte *node) {
            return ((CsbInnerNode *) node)->leaf;
        }

        static uint16_t childrenIsLeaf(byte *node) {
            return ((CsbInnerNode *) (((CsbInnerNode *) node)->children))->leaf;
        }

        static uint16_t isFull(byte *node) {
            return ((CsbInnerNode *) node)->num_keys >= max_keys;
        }

        static uint16_t getNumKeys(byte *node) {
            return ((CsbInnerNode *) node)->num_keys;
        }

        static tKey getLargestKey(byte *node) {
            return ((CsbInnerNode *) node)->keys[((CsbInnerNode *) node)->num_keys - 1];
        }

        byte *split(byte *node_to_split, std::stack<CsbInnerNode *> *path) {

            uint16_t node_to_split_index;
            byte *splitted_right;
            byte *splitted_left;
            CsbInnerNode *parent_node;
            uint16_t num_keys_left_split;
            uint16_t num_keys_right_split;
            uint32_t node_size = (isLeaf(node_to_split)) ? total_leaf_node_size : total_inner_node_size;

            if (this->root != node_to_split) {
                parent_node = path->top();
                path->pop();
                num_keys_left_split = ceil(getNumKeys(node_to_split) / 2.0);
                num_keys_right_split = getNumKeys(node_to_split) - num_keys_left_split;
            } else {
                // split is called on root
                parent_node = (CsbInnerNode *) this->root;
                num_keys_left_split = ceil(parent_node->num_keys / 2.0);
                num_keys_right_split = parent_node->num_keys - num_keys_left_split;

                parent_node = new(_tmm->getMem(1, false)) CsbInnerNode();
                _tmm->release(this->root, 1, isLeaf(this->root));
                parent_node->children = this->root;
                parent_node->num_keys = 1;
                parent_node->keys[0] = getLargestKey(this->root);
                this->root = (byte *) parent_node;
            }

            if (isFull((byte *) parent_node)) {
                byte *splitted_nodes = split((byte *) parent_node, path);
                if (getLargestKey(splitted_nodes) > getLargestKey(
                        node_to_split)) { // if the largest key of the left parent is smaller than the highest key of the nod to be splitted
                    parent_node = (CsbInnerNode *) splitted_nodes; // mark the left node as the parent for the 2 nodes that will be created by the split
                } else {
                    parent_node = ((CsbInnerNode *) splitted_nodes) +
                                  1; // else mark the right node which is adjacent in memory as the parent
                }

            }

            // split this node
            node_to_split_index = (node_to_split - parent_node->children) / node_size;
            splitted_left = parent_node->insert(node_to_split_index + 1, _tmm);
            splitted_right = splitted_left + node_size;

            if (isLeaf(splitted_left)) {
                // first move the keys from the original node to the newly allocated
                ((CsbLeafNode *) splitted_left)->splitKeysAndTids((CsbLeafNode *) splitted_right, num_keys_left_split);
            } else {
                // move the keys from orig to new node
                ((CsbInnerNode *) splitted_left)->splitKeys(
                        (CsbInnerNode *) splitted_right, num_keys_left_split);
            }

            // update the keys in the parent
            parent_node->keys[node_to_split_index] = getLargestKey(splitted_left);
            parent_node->keys[node_to_split_index + 1] = getLargestKey(splitted_right);
            parent_node->num_keys += 1;

            return (byte *) splitted_left;
        }

        static byte *getKthNode(uint16_t k, byte *first_child) {
            return first_child + k * (isLeaf(first_child) ? total_leaf_node_size : total_inner_node_size);
        }

        /*
         * returns the idx to descend into for a given key
         */
        static uint16_t idxToDescend(tKey key, tKey key_array[], uint16_t num_array_elems) {
            if (num_array_elems == 0) return 0;
            for (uint16_t i = 0; i < num_array_elems; i++) {
                if (key_array[i] >= key) {
                    return i;
                }
            }
            return num_array_elems - 1;
        }
    };
}


#endif //CSBPLUSTREE_CSBPLUSTREE_H