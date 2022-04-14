#include <stdint.h>
#include <iostream>
#include <iterator>
#include <vector>
#include <map>
#include <cmath> 
#include <string>
#include <bitset>
using namespace std;

#define N 8
#define BUCKETSIZE 3 // logN


class Client; 
class Server; 
class Block;
class Bucket;
class Node; 
class Tree; 

class Client{
    public: 
        map<int,int> position_map; // client will store position map
        int largest_uid;           // also needs to keep track of largest assinged unique id
    
    Client(){ // constructor for Node
        position_map.clear(); 
        largest_uid = 0; // can assign initiall uid to dummy
    }

    void prinClient(){ // print postion map at client 
        cout << "\n --- Client Position Map ---" << endl;
        map<int, int>::iterator itr;
        cout << "\tKEY\tELEMENT\n";
        for (itr = position_map.begin(); itr != position_map.end(); ++itr) {
            cout << '\t' << itr->first << '\t' << itr->second
                << '\n';
        }

        cout << " --------------------------" << endl;
    }
};

class Server{ 
    // binary tree
    // leaves with unique ids 
    // buckets with blocks 

};

class Block{
    public:
        int uid; // unique identifier
        int leaf; // leaf each block is pointing to 
        string data;

    Block(){ // need default constructor for Bucket
        uid = 0;
        leaf = 0; 
        data = "Dummy"; 
    }

    Block(int uidP, int leafP, string dataP){ // constructor for non dummy blocks
        uid = uidP;
        leaf =leafP; 
        data = dataP; 
    }

    void printBlock(){ // print block info 
        cout << "BLOCK ID: " << uid << " LEAF: " << leaf << " DATA: " << data << endl; 

    }

};

class Bucket{
    public: 
        Block* blocks[BUCKETSIZE]; // store pointers to blocks 
    
     Bucket(){

        for (int i = 0; i < BUCKETSIZE; i++){
            blocks[i] = new Block();  // intialize dummy blocks for bucket
        }
    }

    void printBucket(){ // print bucket + blocks 

        cout<< "\n--- BUCKET w/ size " << BUCKETSIZE << " --- " << endl; 
        for (int i = 0; i < BUCKETSIZE; i++){
            blocks[i]->printBlock();

        }

        cout << "--------------------------" << endl;
    }

};

class Node{
    public:
        Node* left; // children nodes 
        Node* right; 
        Bucket* bucket;
        bool isLeaf; // keep track of leafs 

    Node(){
        isLeaf = true; // assume node is leaf 
        bucket = new Bucket(); 
    }

    void printNode(){ // print all nodes 

        if (isLeaf){
            cout << "\n --- NODE --- " << endl; 
            bucket->printBucket();
            cout << " ^^^ LEAF ^^^ " <<endl;
        }
            
        else{
            cout << "\n --- NODE --- " << endl; 
            bucket->printBucket();
            left->printNode(); 
            right->printNode();
        }
        
    }
};

class Tree{
    public:
        int levels; 
        int leafNums;
        Node* root;
        int bucketSize;

    
    Tree() {
        levels = log2(N);
        leafNums = N;
        bucketSize = log2(N) + 1;
        root = buildRec(levels);
    }

    Node* buildRec(int levelsP){
        Node* root = new Node(); 
        if (levelsP ==0){
            root->isLeaf = true; 
            return root;
        }
        else{
            root->isLeaf = false; 
            root->left = buildRec(levelsP-1);
            root->right = buildRec(levelsP-1);
            return root; 
        }
    }

    void printTree(){
        cout << "\n --- TREE of size " << N << " --- " << endl;
        root->printNode();
    }
};

int main(){
    cout << "hello world" << endl;
    Tree tree;
    tree.printTree();
}   
