#ifndef CSBPLUSTREE_CSBPLUSTREE_H
#define CSBPLUSTREE_CSBPLUSTREE_H

#define CACHE_LINE_SIZE 64
#define FIXED_SIZE 12
#define LEAF_FIXED_SIZE 20
#define NUM_INNER_NODES_TO_ALLOC 1000

#include <stdint.h>
#include <math.h>
#include <memory.h>

struct MemoryManager{
public:
    MemoryManager(uint32_t chunk_size) {
        bytes_allocated = 0;
        bytes_free = 0;
        this->chunk_size = chunk_size;
    }

    char* getMem(uint32_t size){
        if (size > bytes_free){
            allocate_new_chunk();
        }
        bytes_allocated += size;
        bytes_free -= size;
        return ((char*)memptr) + bytes_allocated - size;
    }

private:
    void* memptr;
    uint32_t bytes_allocated;
    uint32_t bytes_free;
    uint32_t chunk_size;

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

    static const uint32_t total_leaf_node_size = ceil((max_keys * sizeof(tKey) + max_keys * sizeof(tTid) + FIXED_SIZE) / CACHE_LINE_SIZE) * CACHE_LINE_SIZE;
    static const uint16_t num_free_bytes_leaf = total_leaf_node_size - max_keys * sizeof(tKey) - max_keys * sizeof(tTid) - FIXED_SIZE;

    struct CsbInnerNode{
        tKey keys[max_keys];
        uint16_t leaf;                    // 2B
        char* children;                   // 8B
        uint16_t num_keys;                // 2B
        uint8_t free[num_free_bytes_inner];  // padding


        CsbInnerNode(){
            num_keys = 0;
            leaf = false;
        }
    };

    struct CsbLeafNode {
        tKey keys[max_keys];
        uint16_t leaf;                    // 2B
        tTid tids[max_keys];
        uint16_t num_keys;                // 2B
        char *preceding_node;             // 8B
        char *following_node;             // 8B
        uint8_t free[num_free_bytes_leaf];   // padding


        CsbLeafNode() {
            leaf = true;
            num_keys = 0;
        }

        /*
         * if rearrangement is necessary, this will create a new LeafNode and copy it
         * to the location of the old node;
         * if the key is the largest of the node, this will just append it
         */
        void insert(tKey key, tTid tid){
            // if key to insert is the largest key in the leaf node, append it
            if(this->keys[this->num_keys-1]<key){
                this->keys[this->num_keys] = key;
                this->tids[this->num_keys] = tid;
                this->num_keys += 1;
                return;
            }

            uint16_t insert_idx = CsbTree::idxToDescend(key, this->keys, this->num_keys);
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
    };

    CsbTree(){
        mm = new MemoryManager(NUM_INNER_NODES_TO_ALLOC * total_inner_node_size);
        root = new CsbInnerNode();
    }

    CsbLeafNode* findLeafNode(tKey key) {
        CsbInnerNode *node = root;
        while (!node->leaf) {
            uint16_t idx_to_descend = this->idxToDescend(key, node->keys, node->num_keys);
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
        // cover case if tree is basically empty
        if (root->num_keys == 0) {
            root->children = mm->getMem(total_leaf_node_size);
            CsbLeafNode *node = new(root->children) CsbLeafNode();
            root->num_keys = 1;
            root->keys[0] = key;
            node->keys[0] = key;
            node->num_keys += 1;
            node->tids[0] = tid;
            return;
        }


        CsbLeafNode* leaf = this->findLeafNode(key);

        // leaf node has enough space for one more key
        if (leaf->num_keys < max_keys) {
            leaf->insert(key, tid);
            return;
        }

        // TODO else case if leaf node is full and splits have to occur
        // TODO recursive splits up the tree


    }

    void remove(tKey key, tTid tid){
        // TODO remove node
    }


private:
    MemoryManager* mm;
    CsbInnerNode* root;

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

    void split(CsbInnerNode* parent_node, uint16_t child_idx){

        // TODO make function generic for leaf and inner nodes

        char* memptr = mm->getMem(total_leaf_node_size * (parent_node->num_keys+1));
        memcpy(memptr,
               parent_node->children,
               total_leaf_node_size*(child_idx)
        );
        memcpy(memptr + total_leaf_node_size*(child_idx+2), // leave space for the 2 new nodes
               parent_node->children + total_leaf_node_size*(child_idx+1),
               total_leaf_node_size * (parent_node->num_keys-child_idx-1) // all the trailing nodes
        );

        CsbLeafNode* leaf_to_split = parent_node->children + child_idx * total_leaf_node_size;
        CsbLeafNode* right_leaf = new (memptr + child_idx*total_leaf_node_size) CsbLeafNode();
        CsbLeafNode* left_leaf = new (memptr + (child_idx+1)*total_leaf_node_size) CsbLeafNode();
        left_leaf->num_keys = ceil(2/leaf_to_split->num_keys);
        right_leaf->num_keys = leaf_to_split->num_keys - left_leaf->num_keys;

        memcpy(&left_leaf->keys, leaf_to_split->keys, sizeof(tKey)*left_leaf->num_keys);
        memcpy(&right_leaf->keys, leaf_to_split->keys[left_leaf->num_keys], sizeof(tKey)*right_leaf->num_keys);

        memcpy(&left_leaf->tids, leaf_to_split->tids, sizeof(tTid)*left_leaf->num_keys);
        memcpy(&right_leaf->tids, leaf_to_split->tids[left_leaf->num_keys], sizeof(tTid)*right_leaf->num_keys);


        // TODO how do I free the memory I allocated with posix_sysalign?

        parent_node->num_keys += 1;
        parent_node->children = memptr;

        updateLeafNodesPointers((CsbLeafNode*)(memptr), parent_node->num_keys);

    }

    static char* getKthNode(uint16_t k, char* first_child){
        uint32_t child_node_size;
        if (((CsbInnerNode*) first_child)->leaf){
            child_node_size = total_leaf_node_size;
        }
        else{
            child_node_size = total_inner_node_size;
        }
        return first_child + k*child_node_size;
    }

    /*
     * returns the idx to descend into for a given key
     */
    static uint16_t idxToDescend(tKey key, tKey key_array[], uint16_t num_array_elems){
        for (uint16_t i=0; i<num_array_elems; i++){
            if (key_array[i] >= key){
                return i;
            }
        }
        return num_array_elems -1;
    }
};


#endif //CSBPLUSTREE_CSBPLUSTREE_H