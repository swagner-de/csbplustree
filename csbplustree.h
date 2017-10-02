#ifndef CSBPLUSTREE_CSBPLUSTREE_H
#define CSBPLUSTREE_CSBPLUSTREE_H

#include <math.h>
#include <map>
#include <list>


typedef typename std::list<unsigned int> TidList;

// superclass for inner and leaf nodes
template <class T>
struct CsbPlusTreeNode{
public:
    bool isLeaf;
protected:
    CsbPlusTreeNode<T>* first_child;
    unsigned int no_keys;
    
    
public:
    virtual ~CsbPlusTreeNode() = 0;
    virtual T getRightMostKey() = 0;
};


template <class T>
struct CsbPlusTreeLeafNode: public CsbPlusTreeNode<T>{
public:
    CsbPlusTreeLeafNode* preceding_sibling;
    CsbPlusTreeLeafNode* following_sibling;
private:
    std::map<T ,TidList >* values;
    
    
public:
    CsbPlusTreeLeafNode(std::map<T ,TidList >* values, CsbPlusTreeLeafNode* preceding_sibling, CsbPlusTreeLeafNode* following_sibling){
        this->following_sibling = following_sibling;
        this->preceding_sibling = preceding_sibling;
        this->no_keys = values->size();
        this->values = values;
        this->isLeaf = true;
    }

    TidList::iterator* getTidsByKey(T k){
        this->no_keys = this->no_keys;
    }

    T getRightMostKey(){
        return this->values->end()->first; // obtain the last key from the map, assuming the map is ordered
    }
};


template <class T>
struct CsbPlusTreeInnerNode: public CsbPlusTreeNode<T>{
private:
    T keys =  new T[this->no_keys];
    CsbPlusTreeNode<T>* first_child;

public:
    T getRightMostKey(){
        return this->keys[this->no_keys - 1]; // obtain the right-most key
    }

    CsbPlusTreeInnerNode(CsbPlusTreeNode<T>* first_child, unsigned int no_keys){
        this->first_child = first_child;
        this->isLeaf=false;
        this->no_keys = no_keys;
        // iterate over all children and get their highest keys
        for (unsigned int i = 0; i<no_keys; ++i) {
            CsbPlusTreeNode<T> *child = first_child + i; // obtain the i-th child
            this->keys[i] = child->getRightMostKey();
        }
    };

    ~CsbPlusTreeInnerNode(){
        delete[] keys;
    }


    CsbPlusTreeNode<T>* find(T k){
        unsigned int i = 0;
        while (i < this->no_keys){
            if (k <= this->keys[i]){
                return this->first_leaf_child + i-1;;
            }
            else i++;
        }
        return this->first_leaf_child + k-1;
    }
};


template <class T>
struct CsbPlusTree{
private:
    unsigned int leaf_key_size;
    unsigned int node_split_size;
    CsbPlusTreeInnerNode<T>* root;
    unsigned int no_leaf_nodes;

public:
    CsbPlusTree(unsigned int leaf_key_size, unsigned int node_split_size){
        this->node_split_size = node_split_size;
        this->leaf_key_size = leaf_key_size;
    }
    ~CsbPlusTree(){}

    void bulk_insert(std::map< T, TidList* >* values){
        
        CsbPlusTreeNode<T> nodes = this->createLeafNodes(values);
        unsigned int no_lower_level_nodes = this->no_leaf_nodes;
        unsigned int no_new_nodes = ceil(no_lower_level_nodes/this->node_split_size);
        do{
            nodes = this->createInnerNodes(nodes, no_lower_level_nodes, no_new_nodes);
            no_lower_level_nodes = no_new_nodes;
            no_new_nodes = ceil(no_lower_level_nodes/this->node_split_size);
        }while(no_new_nodes>0);
        this->root = nodes;

    };
    
    TidList::iterator find(T key){
        CsbPlusTreeNode<T>* current_node;
        current_node = this->root;
        do{
            // need to cast here to run the .find member?
            current_node = (dynamic_cast<CsbPlusTreeInnerNode<T>*>(current_node))->find(key);
        }while(!current_node->isLeaf());

        // is this the way to do it?
        return (dynamic_cast<CsbPlusTreeLeafNode<T>*>(current_node))->getTidsByKey(key);


    }
    void insert(T Key, TidList){}

private:


    CsbPlusTreeLeafNode<T>* createLeafNodes(std::map< T, TidList* >* values) {

        this->no_leaf_nodes = ceil(values->size() / leaf_key_size);

        // allocate consecutive space for leaf nodes
        CsbPlusTreeLeafNode<T>* leaf_nodes = new CsbPlusTreeLeafNode<T>[this->no_leaf_nodes];


        // why do i need typename here?
        typename std::map<T, TidList>::iterator iter = values->begin();
        unsigned int i = 0;

        // create leaf nodes here
        while (iter != values->end()) {

            // determine forward and backward pointers of leaf node
            CsbPlusTreeLeafNode<T>* preceding;
            CsbPlusTreeLeafNode<T>* following;

            if (i == 0) preceding = nullptr;
            else preceding = leaf_nodes[i - 1];
            if (i == values->size() - 1) following = nullptr;
            else following = leaf_nodes[i + 1];


            // create the map of the leaf node with multiple keys
            std::map<T, TidList>* node_values;
            for (unsigned int j = 0; j < this->leaf_key_size && iter != values.end; ++j) {
                node_values = std::map<T, TidList*>();
                node_values[iter->first] = iter->second;
                iter++;
                leaf_nodes[i] = CsbPlusTreeLeafNode<T>(node_values, preceding, following);
            }
            i++;
        }
        return leaf_nodes;
    }

    CsbPlusTreeInnerNode<T>* createInnerNodes(CsbPlusTreeNode<T>* lower_level_nodes, unsigned int no_lower_level_nodes, unsigned int no_nodes){
        // allocate consecutive memory for the new higher level of nodes
        CsbPlusTreeInnerNode<T>* nodes = new CsbPlusTreeInnerNode<T>[no_nodes];


        // fill the created array
        for(unsigned int i=0; i< no_nodes; ++i){

            // calculate the number of keys the inner node will contain
            unsigned int no_keys;
            unsigned int remaining_keys = no_lower_level_nodes - (i*this->node_split_size);
            if (remaining_keys < this->node_split_size) no_keys = remaining_keys;
            else no_keys = this->node_split_size;

            CsbPlusTreeInnerNode<T>* node = CsbPlusTreeInnerNode<T>(lower_level_nodes[i*this->node_split_size], no_keys);

            nodes[i]=node;
        }
        return nodes;
    }
};





#endif //CSBPLUSTREE_CSBPLUSTREE_H
