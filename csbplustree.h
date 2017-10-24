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

    static const uint16_t num_cachelines_leaf_node =  ceil((max_keys * sizeof(tKey) + max_keys * sizeof(tTid) + LEAF_FIXED_SIZE)/64.0);
    static const uint32_t total_leaf_node_size = num_cachelines_leaf_node * CACHE_LINE_SIZE;
    static const uint16_t num_free_bytes_leaf = total_leaf_node_size - (max_keys * sizeof(tKey) + max_keys * sizeof(tTid) + LEAF_FIXED_SIZE);

    struct CsbInnerNode{
        tKey keys[max_keys];
        uint16_t leaf;                    // 2B
        uint16_t num_keys;                // 2B
        char* children;                   // 8B
        uint8_t free[num_free_bytes_inner];  // padding


        CsbInnerNode(){
            num_keys = 0;
            leaf = false;
        }
    };

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
        }

        /*
         * if rearrangement is necessary, this will create a new LeafNode and copy it
         * to the location of the old node;
         * if the key is the largest of the node, this will just append it
         */
        void insert(tKey key, tTid tid){
            // if key to insert is the largest key in the leaf node, append it
            if(this->keys[this->num_keys-1]<key || this->num_keys == 0){
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
        root = (char*) new (mm->getMem(total_leaf_node_size)) CsbLeafNode();
    }

    CsbLeafNode* findLeafNode(tKey key) {
        CsbInnerNode *node = (CsbInnerNode*) root;
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
        // cover case if root node is a leaf and the key can be appended right to it
        if (isLeaf(root) && !isFull(root)){
            CsbLeafNode* node = (CsbLeafNode*) root;
            node->insert(key, tid);
            return;
        }
        // root is leaf but is full
        if (isLeaf(root) && isFull(root)){
            // create 2 new leaf nodes, one new root inner node with pointers to the leaf nodes
            CsbLeafNode* to_split = (CsbLeafNode*) root;
            CsbInnerNode* new_root = new (mm->getMem(total_inner_node_size)) CsbInnerNode();
            new_root->num_keys = 1;
            new_root->keys[0] = getLargestKey(root);
            new_root->children = root;
            root = (char*) new_root;
            split(new_root, 0);
            return;
        }

/*        if (root->num_keys == 0) {
            root->children = mm->getMem(total_leaf_node_size);
            CsbLeafNode *node = new(root->children) CsbLeafNode();
            root->num_keys = 1;
            root->keys[0] = key;
            node->keys[0] = key;
            node->num_keys += 1;
            node->tids[0] = tid;
            return;
        }*/


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
    char* root;

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

    void split(CsbInnerNode* parent_node, uint16_t child_idx){
        uint16_t children_leaf = childrenIsLeaf((char*) parent_node);

        uint32_t child_node_size;
        if (children_leaf){
            child_node_size = total_leaf_node_size;
        } else{
            child_node_size = total_inner_node_size;
        }

        char* memptr = mm->getMem(child_node_size * (parent_node->num_keys+1));
        memcpy(memptr,
               parent_node->children,
               child_node_size*(child_idx)
        ); // copy leading nodes of node group

        memcpy(memptr + child_node_size*(child_idx+2), // leave space for the 2 new nodes
               parent_node->children + child_node_size*(child_idx+1),
               child_node_size * (parent_node->num_keys-child_idx-1) // all the trailing nodes
        ); // copy trailing nodes of node group



        char* node_to_split = parent_node->children + child_idx * child_node_size;
        parent_node->children = memptr;

        CsbInnerNode* right_child;
        CsbInnerNode* left_child;
        // create the new nodes
        if (children_leaf){
            right_child = (CsbInnerNode*) new (memptr + child_idx*child_node_size) CsbLeafNode();
            left_child = (CsbInnerNode*) new (memptr + (child_idx+1)*child_node_size) CsbLeafNode();

        }else{
            right_child = new (memptr + child_idx*child_node_size) CsbInnerNode();
            left_child =  new (memptr + (child_idx+1)*child_node_size) CsbInnerNode();
        }

        left_child->num_keys = ceil(getNumKeys(node_to_split)/2.0);
        right_child->num_keys = getNumKeys(node_to_split)- left_child->num_keys;

        // copy the keys
        memcpy(&left_child->keys, &((CsbInnerNode*)node_to_split)->keys, sizeof(tKey)*left_child->num_keys);
        memcpy(&right_child->keys, &((CsbInnerNode*)node_to_split)->keys[left_child->num_keys], sizeof(tKey)*right_child->num_keys);

        if (children_leaf){
            // copy the tids and update the chain pointers
            memcpy(&((CsbLeafNode*)left_child)->tids, &((CsbLeafNode*)node_to_split)->tids, sizeof(tTid)*left_child->num_keys);
            memcpy(&((CsbLeafNode*)right_child)->tids, &((CsbLeafNode*)node_to_split)->tids[left_child->num_keys], sizeof(tTid)*right_child->num_keys);
            updateLeafNodesPointers((CsbLeafNode*)(memptr), parent_node->num_keys+1);
        }

        // update the keys in the parent node
        parent_node->num_keys += 1;
        memmove(&parent_node->keys[child_idx+1], // shift everything behind the index where the new key is inserted
               &parent_node->keys[child_idx], // start at the child index
               sizeof(tKey)*parent_node->num_keys-child_idx // all the trailing nodes
        );
        parent_node->keys[child_idx] = left_child->keys[left_child->num_keys-1];



        // TODO how do I free the memory I allocated with posix_sysalign?

    }

    static char* getKthNode(uint16_t k, char* first_child){
        uint32_t child_node_size;
        if (isLeaf(first_child)){
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