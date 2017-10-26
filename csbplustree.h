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

        void splitKeys(CsbInnerNode* split_node){
            uint16_t split_idx= ceil(this->num_keys/2.0);
            memmove(&split_node->keys, &this->keys[split_idx], sizeof(tKey)*this->num_keys-split_idx); // keys
            split_node->num_keys = this->num_keys-split_idx;
            this->num_keys = split_idx;
            split_node->children = getKthNode(split_idx, this->children);
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
        void insert(tKey key, tTid tid, CsbInnerNode* parent){
            // if key to insert is the largest key in the leaf node, append it
            if(this->keys[this->num_keys-1]<key || this->num_keys == 0){
                this->keys[this->num_keys] = key;
                this->tids[this->num_keys] = tid;
                this->num_keys += 1;
                return;
            }

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

        void splitKeysAndTids(CsbLeafNode* split_node){
            uint16_t split_idx= ceil(this->num_keys/2.0);
            memmove(&split_node->keys, &this->keys[split_idx], sizeof(tKey)*this->num_keys-split_idx); // keys
            memmove(&split_node->tids, &this->tids[split_idx], sizeof(tTid)*this->num_keys-split_idx); // tids
            split_node->num_keys = this->num_keys-split_idx;
            this->num_keys = split_idx;
        }

    };

    CsbTree(){
        mm = new MemoryManager(NUM_INNER_NODES_TO_ALLOC * total_inner_node_size);
        root = (char*) new (mm->getMem(total_leaf_node_size)) CsbLeafNode();
    }

    CsbLeafNode* findLeafNode(tKey key, std::stack<std::pair<CsbLeafNode*, uint16_t >>* path=nullptr) {
        CsbInnerNode *node = (CsbInnerNode*) root;
        while (!node->leaf) {
            uint16_t idx_to_descend = this->idxToDescend(key, node->keys, node->num_keys);
            if (path != nullptr) path->push(std::make_pair(node, idx_to_descend));
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

        std::stack<std::pair<CsbLeafNode*, uint16_t >> path;
        CsbLeafNode* leaf_to_insert = findLeafNode(key, path);

        if (!isFull((char*) leaf_to_insert)){
            leaf_to_insert->insert(key, tid, path);
            updatePath(path);
            return;
        }


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

    static void updatePath(tKey key, std::stack<std::pair<CsbInnerNode*, uint16_t >> path){
        while (!path.empty()){
            std::pair<CsbInnerNode*, uint16_t> p = path.top();
            CsbInnerNode* node = p.first;
            uint16_t idx = p.second;
            p = NULL;

            if (node->keys[idx] >= key) return; // nothing to do
            node->keys[idx] = key;
        }
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


    void split(char* node_to_split, std::stack<std::pair<CsbInnerNode*, uint16_t>>* path){
        std::pair<CsbInnerNode*, uint16_t> parent = path->top();
        CsbInnerNode * parent_node = parent->first;
        uint16_t node_to_split_idx = parent->second;
        uint32_t node_size = total_inner_node_size;
        if (isLeaf(node_to_split)) node_size = total_leaf_node_size;


        if (isFull((char*) parent_node)){
            uint16_t split_idx = ceil(parent_node->num_keys/2.0);
            // create a new node group g'
            char* memptr = mm->getMem((parent_node->num_keys - split_idx) * node_size);
            // distribute the nodes evenly
            memcpy(memptr, &parent_node->children[split_idx+1], node_size * (parent_node->num_keys - split_idx));
            parent_node->num_keys -= (split_idx-1);

            split(((char*) parent_node), path);


        }

        if(!isFull((char*) parent_node)) {

            char *memptr = mm->getMem((parent_node->num_keys + 1) * node_size); //create node group g'

            // copy all nodes from old node group to g' leaving space for an additional node
            memcpy(
                    memptr,
                   &parent_node->children,
                   node_size * (node_to_split_idx + 1)
            );
            memcpy(
                    memptr + node_size * (node_to_split_idx + 2), // leave space for one additional node
                    &parent_node->children[node_to_split_idx + 1],
                    node_size *
                    (parent_node->keys - node_to_split_idx - 1)
            ); // copy the node to the freshly allocated node group

            parent_node->children = memptr; // update the child pointer f

            //create the new node in the new node group
            if (isLeaf(node_to_split)){
                CsbLeafNode* split_node = new(memptr + node_size * (node_to_split_idx +1)) CsbLeafNode();
                (CsbLeafNode*)node_to_split)->splitKeysAndTids(split_node); // evenly distribute the keys
            }else{
                CsbInnerNode* split_node = new(memptr + node_size * (node_to_split_idx +1)) CsbInnerNode();
                ((CsbInnerNode*)node_to_split)->splitKeys(split_node); // evenly distribute the keys

            }

            memmove(&parent_node->keys[node_to_split_idx+2], &parent_node->keys[node_to_split_idx+1], sizeof(tKey) * parent_node->num_keys-1); // shift the keys in the parent accordingly
            // update the keys
            parent_node->keys[node_to_split_idx] = getLargestKey(node_to_split);
            parent_node->keys[node_to_split_idx+1] =  getLargestKey((char*) split_node);
        }
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