#include <stdint.h>
#include <iostream>
#include <iterator>
#include <vector>
#include <map>
#include <cmath> 
#include <string>
using namespace std;

#define BUCKETSIZE 4


class Client; 
class Server; 
class Block;
class Bucket;
class Node; 
// class Tree; 

class Client{
    public: 
        map<int,int> position_map; // client will store position map
        int largest_uid;           // also needs to keep track of largest assinged unique id
    
    Client(){ // constructor for Node
        position_map.clear(); 
        largest_uid = 0; // can assign initiall uid to dummy
    }

    void prinClient(){
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

    Block(int uidP, int leafP, string dataP){
        uid = uidP;
        leaf =leafP; 
        data = dataP; 
    }

    void printBlock(){
        cout << "BLOCK ID: " << uid << " LEAF: " << leaf << " DATA: " << data << endl; 

    }

};

class Bucket{
    public: 
        int size; 
        Block blocks[BUCKETSIZE];
    
     Bucket(){
        size = BUCKETSIZE;

        for (int i = 0; i < size; i++){
            Block newBlock; 
            blocks[i] = newBlock; 
        }
    }

    void printBucket(){

        cout<< "\n--- BUCKET w/ size " << size << " --- " << endl; 
        for (int i = 0; i < size; i++){
            blocks[i].printBlock();

        }

        cout << "--------------------------" << endl;
    }

};

class Node{
    public:
        Node* left; 
        Node* right; 
        Bucket bucket;
        bool isLeaf; 

    Node(){
        isLeaf = true;
    }

    void printNode(){

        if (isLeaf){
            cout << "\n --- NODE --- " << endl; 
            bucket.printBucket();
            cout << " ^^^ LEAF ^^^ " <<endl;
        }
            
        else{
            cout << "\n --- NODE --- " << endl; 
            bucket.printBucket();
            left->printNode(); 
            right->printNode();
        }
        
    }
};

class Tree{
    public:
        int size; 
        int levels; 
        int leafNums;
        Node* root;
        int bucketSize;

    
    Tree(int N) {
        size = N;
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
        cout << "\n --- TREE of size " << size << " --- " << endl;
        root->printNode();
    }
};

int main(){
    cout << "hello world" << endl;
    Tree tree(4); 
    tree.printTree(); 

//     Node root; 
//     Node left; 
//     Node right; 

//     root.isLeaf = false; 

//     root.left = &left; 
//     root.right = &right; 

//     root.printNode(); 
//    // left.printNode(); 
//     //right.printNode(); 

    // Node n1; 
    // n1.printNode();

    // Node n2; 
    // Node n3; 

    // n1.isLeaf = false; 
    // n1.left = &n2;
    // n1.right = &n3;

    // n1.printNode();

}   
