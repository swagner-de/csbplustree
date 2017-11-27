#ifndef CSBPLUSTREE_CSBPLUSTREE_H
#define CSBPLUSTREE_CSBPLUSTREE_H

#define CACHE_LINE_SIZE 64
#define FIXED_SIZE 12
#define LEAF_FIXED_SIZE 20
#define NUM_INNER_NODES_TO_ALLOC 1000

#include <stdint.h>
#include <math.h>
#include <stack>
#include <memory.h>

struct MemoryManager{
public:
    static MemoryManager& getInstance(){
        static MemoryManager instance;
        return instance;
    }

    char* getMem(uint32_t size){
        if (size > bytes_free){
            allocate_new_chunk();
        }
        bytes_allocated += size;
        bytes_free -= size;
        return ((char*)memptr) + bytes_allocated - size;
    }

    void release(char* begin, uint32_t num_bytes){
        // TODO
    }

private:
    void* memptr;
    uint32_t bytes_allocated;
    uint32_t bytes_free;
    const uint32_t chunk_size = 10000;

    MemoryManager() {
        bytes_allocated = 0;
        bytes_free = 0;
    }

    void allocate_new_chunk(){
        bytes_free = chunk_size;
        bytes_allocated = 0;
        if (posix_memalign(&this->memptr, 64, chunk_size)) {
            std::cout << "Error allocating memory";
        }
    }
};

template <class tKey, class tTid, uint16_t num_cachelines>
struct CsbTree{
    static const uint32_t total_inner_node_size = CACHE_LINE_SIZE * num_cachelines;
    static const uint16_t max_keys = (total_inner_node_size - FIXED_SIZE) / sizeof(tKey);
    static const uint16_t num_free_bytes_inner = total_inner_node_size - (max_keys * sizeof(tKey) + FIXED_SIZE);

    static const uint16_t num_cachelines_leaf_node =  ceil((max_keys * sizeof(tKey) + max_keys * sizeof(tTid) + LEAF_FIXED_SIZE)/64.0);
    static const uint32_t total_leaf_node_size = num_cachelines_leaf_node * CACHE_LINE_SIZE;
    static const uint16_t num_free_bytes_leaf = total_leaf_node_size - (max_keys * sizeof(tKey) + max_keys * sizeof(tTid) + LEAF_FIXED_SIZE);

    static MemoryManager* mm; // made it static as embedded class does not belong to the instance of the outer class
    char* root;

    CsbTree(){
        root = (char*) new (MemoryManager::getInstance().getMem((total_leaf_node_size))) CsbLeafNode();
    }

    struct CsbInnerNode{
        tKey keys[max_keys];
        uint16_t leaf;                    // 2B
        uint16_t num_keys;                // 2B
        char* children;                   // 8B
        uint8_t free[num_free_bytes_inner];  // padding


        CsbInnerNode(){
            num_keys = 0;
            leaf = false;
        };


        /*
         * this function shifts all nodes past the insert index within the parents node group
         */

        char* insert(uint16_t idx_to_insert){

            uint32_t node_size;
            char* memptr;
            char* node_to_insert;

            node_size = (isLeaf(this->children)) ? total_leaf_node_size : total_inner_node_size;

            // allocate new node group, 1 node larger than the old one
            memptr = MemoryManager::getInstance().getMem((this->num_keys+1) * node_size);
            // copy values to new group
            memcpy(memptr, this->children, idx_to_insert*node_size); // leading nodes
            memcpy(memptr + node_size*(idx_to_insert+1), this->children + node_size*(idx_to_insert), (this->num_keys-idx_to_insert)*node_size); // trailing nodes
            // shift keys in parent
            memmove(&this->keys[idx_to_insert+1], &this->keys[idx_to_insert], sizeof(tKey) * (this->num_keys-idx_to_insert));

            MemoryManager::getInstance().release(this->children, this->num_keys * node_size);

            // update the child ptr
            this->children = memptr;

            if (isLeaf(this->children)){
                node_to_insert =  (char*) new (memptr + node_size*idx_to_insert) CsbLeafNode();
            }else{
                node_to_insert =  (char*) new (memptr + node_size*idx_to_insert) CsbInnerNode();
            }

            return  node_to_insert - node_size; // returns a pointer to the left node, where the node group's containing child nodes have been copied from
        }

        void splitKeys(CsbInnerNode* split_node, uint16_t num_keys_remaining){
            memmove(&split_node->keys, &this->keys[num_keys_remaining], sizeof(tKey)*(this->num_keys-num_keys_remaining)); // keys
            split_node->num_keys = this->num_keys-num_keys_remaining;
            split_node->children = getKthNode(num_keys_remaining, this->children);
            this->num_keys = num_keys_remaining;
        }

        void remove(uint16_t remove_idx){
            uint32_t child_node_size = (childrenIsLeaf(this)) ? total_leaf_node_size : total_inner_node_size;
            char* memptr = mm->getInstance().getMem((this->num_keys-1) * child_node_size);

            memcpy(memptr, this->children, remove_idx*child_node_size);
            memcpy(memptr + (remove_idx*child_node_size), this->children + ((remove_idx+1) * child_node_size), (this->num_keys-1-remove_idx) * child_node_size);
            mm->getInstance().release(this->children, child_node_size*this->num_keys);

            this->children = memptr;
            memmove(&this->keys[remove_idx], &this->keys[remove_idx+1], (this->num_keys-1-remove_idx)*sizeof(tKey));
            this->num_keys -= 1;
        }

    } __attribute__((packed));

    struct CsbLeafNode {
        tKey keys[max_keys];
        uint16_t leaf;                    // 2B
        uint16_t num_keys;                // 2B
        tTid tids[max_keys];
        CsbLeafNode* preceding_node;             // 8B
        CsbLeafNode* following_node;             // 8B
        uint8_t free[num_free_bytes_leaf];   // padding



        CsbLeafNode() {
            leaf = true;
            num_keys = 0;
        };

        void insert(tKey key, tTid tid){

            uint16_t insert_idx = idxToDescend(key, this->keys, this->num_keys);
            memmove(&this->keys[insert_idx+1],
                    &this->keys[insert_idx],
                    (this->num_keys - insert_idx) * sizeof(tKey)
            );
            memmove(&this->tids[insert_idx+1],
                    &this->tids[insert_idx],
                    (this->num_keys - insert_idx) * sizeof(tTid)
            );
            this->keys[insert_idx] = key;
            this->tids[insert_idx] = tid;
            this->num_keys += 1;
        }

        void splitKeysAndTids(CsbLeafNode* split_node, uint16_t num_keys_remaining){
            memmove(&split_node->keys, &this->keys[num_keys_remaining], sizeof(tKey)*(this->num_keys-num_keys_remaining)); // keys
            memmove(&split_node->tids, &this->tids[num_keys_remaining], sizeof(tTid)*(this->num_keys-num_keys_remaining)); // tids
            split_node->num_keys = this->num_keys - num_keys_remaining;
            this->num_keys = num_keys_remaining;
        }

        void remove(tKey key, tTid tid) {
            uint16_t remove_idx = idxToDescend(key, this->keys, this->num_keys);
            while (this->tids[remove_idx] != tid && remove_idx < this->num_keys && this->keys[remove_idx] == key)
                remove_idx++;
            if (this->tids[remove_idx] != tid){
                return;
            }
            memmove(this->keys[remove_idx], this->keys[remove_idx+1], (this->num_keys-1-remove_idx)*sizeof(tKey));
            memmove(this->tids[remove_idx], this->tids[remove_idx+1], (this->num_keys-1-remove_idx)*sizeof(tTid));
            this->num_keys -=1;
        }
    } __attribute__((packed));



    CsbLeafNode* findLeafNode(tKey key, std::stack<CsbInnerNode*>* path=nullptr) {
        CsbInnerNode *node = (CsbInnerNode*) root;
        while (!node->leaf) {
            uint16_t idx_to_descend = this->idxToDescend(key, node->keys, node->num_keys);
            if (path != nullptr) path->push(node);
            node = (CsbInnerNode *) getKthNode(idx_to_descend, node->children);
        }
        return  (CsbLeafNode *) node;
    }

    tTid find(tKey key){
        CsbLeafNode* leaf = this->findLeafNode(key);
        uint16_t match_idx = this->idxToDescend(key, leaf->keys, leaf->num_keys);
        if (leaf->keys[match_idx] == key) return leaf->tids[match_idx];
        return NULL;
    }

    void insert(tKey key, tTid tid) {

        // find the leaf node to insert the key to and record the path upwards the tree
        std::stack<CsbInnerNode*> path;
        CsbLeafNode* leaf_to_insert = findLeafNode(key, &path);

        // if there is space, just insert
        if (!isFull((char*) leaf_to_insert)){
            leaf_to_insert->insert(key, tid);
            return;
        }
        // else split the node and see in which of the both nodes that have been split the key fits

        leaf_to_insert = (CsbLeafNode*) split((char*) leaf_to_insert, &path);
        if (key > leaf_to_insert->keys[leaf_to_insert->num_keys - 1]){
            (leaf_to_insert + 1)->insert(key, tid);
        }else{
            leaf_to_insert->insert(key, tid);
        }
    }

    void remove(tKey key, tTid tid){
        std::stack<CsbInnerNode*>* path;
        CsbInnerNode* parent;
        CsbLeafNode* leaf;
        uint16_t parent_idx = (leaf-parent->children)/total_leaf_node_size;

        parent = path->top();
        path->pop();
        leaf = findLeafNode(key, &path);

        if (key != getLargestKey(leaf)) {
            leaf->remove(key, tid);
        }
        else{
            leaf->remove(key, tid);
            parent->keys[parent_idx] = getLargestKey(leaf);
        }
    }

    static uint16_t isLeaf(char* node){
        return ((CsbInnerNode*) node)->leaf;
    }

    static uint16_t childrenIsLeaf(char* node){
        return ((CsbInnerNode*)(((CsbInnerNode*) node)->children))->leaf;
    }

    static uint16_t isFull(char* node){
        return ((CsbInnerNode*) node)->num_keys >= max_keys;
    }

    static uint16_t getNumKeys(char* node){
        return ((CsbInnerNode*) node)->num_keys;
    }

    static tKey getLargestKey(char* node){
        return ((CsbInnerNode*) node)->keys[((CsbInnerNode*) node)->num_keys -1];
    }

    static void updateLeafNodesPointers(CsbLeafNode* first_leaf, uint16_t num_leaf_nodes){
        for (uint16_t i=0; i<num_leaf_nodes; i++){
            if (i!=0){
                (first_leaf + i)->preceding_node = first_leaf + i-1;
            }
            if (i!=num_leaf_nodes-1){
                (first_leaf + i)->following_node = first_leaf + i +1;
            }
        }
    }


    char* split(char* node_to_split, std::stack<CsbInnerNode*>* path){

        uint16_t  node_to_split_index;
        char* splitted_right;
        char* splitted_left;
        CsbInnerNode* parent_node;
        uint16_t num_keys_left_split;
        uint16_t num_keys_right_split;
        uint32_t node_size = (isLeaf(node_to_split)) ? total_leaf_node_size : total_inner_node_size;

        if (this->root != node_to_split){
            parent_node = path->top();
            path->pop();
            num_keys_left_split = ceil(getNumKeys(node_to_split)/2.0);
            num_keys_right_split = getNumKeys(node_to_split) - num_keys_left_split;
        }else{
            // split is called on root
            parent_node = (CsbInnerNode*) this->root;
            num_keys_left_split = ceil(parent_node->num_keys/2.0);
            num_keys_right_split = parent_node->num_keys - num_keys_left_split;

            parent_node = new (MemoryManager::getInstance().getMem(total_inner_node_size)) CsbInnerNode();
            MemoryManager::getInstance().release(this->root, (isLeaf(this->root)) ? total_leaf_node_size : total_inner_node_size);
            parent_node->children = this->root;
            parent_node->num_keys = 1;
            parent_node->keys[0] = getLargestKey(this->root);
            this->root = (char*) parent_node;
        }

        if (isFull((char*) parent_node)) {
            char* splitted_nodes = split((char *) parent_node, path);
            if (getLargestKey(splitted_nodes) > getLargestKey(node_to_split) ){ // if the largest key of the left parent is smaller than the highest key of the nod to be splitted
                parent_node = (CsbInnerNode*) splitted_nodes; // mark the left node as the parent for the 2 nodes that will be created by the split
            }else{
                parent_node = ((CsbInnerNode*) splitted_nodes) + 1; // else mark the right node which is adjacent in memory as the parent
            }

        }

        // split this node
        node_to_split_index = (node_to_split - parent_node->children) / node_size;
        splitted_left = parent_node->insert(node_to_split_index + 1);
        splitted_right = splitted_left + node_size;

        if (isLeaf(splitted_left)){
            // first move the keys from the original node to the newly allocated
            ((CsbLeafNode*)splitted_left)->splitKeysAndTids((CsbLeafNode*) splitted_right, num_keys_left_split);
        }else{
            // move the keys from orig to new node
            ((CsbInnerNode*)splitted_left)->splitKeys(
                    (CsbInnerNode*) splitted_right, num_keys_left_split);
        }

        // update the keys in the parent
        parent_node->keys[node_to_split_index] = getLargestKey(splitted_left);
        parent_node->keys[node_to_split_index +1] = getLargestKey(splitted_right);
        parent_node->num_keys +=1;

        return (char*) splitted_left;
    }

    static char* getKthNode(uint16_t k, char* first_child){
        return first_child + k*(isLeaf(first_child) ? total_leaf_node_size : total_inner_node_size);
    }

    /*
     * returns the idx to descend into for a given key
     */
    static uint16_t idxToDescend(tKey key, tKey key_array[], uint16_t num_array_elems){
        if (num_array_elems == 0) return 0;
        for (uint16_t i=0; i<num_array_elems; i++){
            if (key_array[i] >= key){
                return i;
            }
        }
        return num_array_elems -1;
    }
};


#endif //CSBPLUSTREE_CSBPLUSTREE_H