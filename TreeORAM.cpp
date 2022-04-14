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
#define numNodes N*2 -1
#define BUCKETSIZE 3 // logN

class Block;
class Bucket;
class Node; 
class Tree; 

class Client; 
class Server; 

class Block{
    public:
        int uid; // unique identifier
        int leaf; // leaf each block is pointing to 
        string data;

    Block(){ // need default constructor for Bucket
        uid = -1;
        leaf = -1; 
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

        Bucket* bucket;
        bool isLeaf; // keep track of leafs 

    Node(int index){ // pas index of node

        isLeaf = checkLeaf(index); // leaf boolean
        bucket = new Bucket(); 
    }

    int smallLeaf(){ // returns smallest leaf value
        return N-1; 
    }

    int bigLeaf(){
        return 2*N -1; // returns biggest leaf value +1
    }

    bool checkLeaf(int index){ // check if node is leaf

        if ((index >= smallLeaf()) && (index < bigLeaf())){
            return true;
        }
        return false; 
    }

    int left(int index){ // calculate index of left child
        if (isLeaf){     // leaf has no right child
            return -1; 
        }
        return ((2*index) + 1);
    }

    int right(int index){ // calculate index of right child 
        if (isLeaf){      // leaf has no right child 
            return -1; 
        }
        return ((2*index) +2);
    }

    int parent(int index){ // get index of parent node

        if (index == 0){  // parent of root does not exist 
            return -1; 
        }

        return(floor((index-1)/2));
    }


    void printNode(int index){ // print node info + bucket

        cout << "\n --- NODE " << index << " --- " << endl; 
        cout << "Left Index: " << left(index) << " Right Index: " << right(index) << " Parent Index: " << parent(index) << endl; 
        bucket->printBucket();
        
    }
};

class Tree{
    public:
        int levels; 
        Node* nodes[numNodes];

    Tree(){
        levels = log2(N);
        for (int i = 0; i < numNodes; i++){
            nodes[i] = new Node(i);  // intialize dummy blocks for bucket
        }
    }

    void printTree(){
        cout << "\n --- TREE of size " << numNodes << " --- " << endl;
        
        for(int i = 0; i < numNodes; i++){
            nodes[i]->printNode(i);
        }

    }

};

class Client{
    public: 
        map<int,int> position_map; // client will store position map
        int new_uid;               // also needs to keep track of largest assinged unique id
        Tree* tree;

    Client(){ // constructor for Node
        position_map.clear(); 
        new_uid = 0; // generate new uids for blocks

        tree = new Tree(); 

    }

    void prinClient(){ // print postion map at client 
        cout << "\n --- Client Position Map ---" << endl;
        map<int, int>::iterator itr;
        cout << "\tKEY\tELEMENT\n";
        for (itr = position_map.begin(); itr != position_map.end(); ++itr) {
            cout << '\t' << itr->first << '\t' << itr->second
                << '\n';
        }

        tree->printTree(); 

        cout << " --------------------------" << endl;
    }
};

class Server{ 
    public:
        Tree* tree;

    Server(){
        tree = new Tree(); 
    }

    void update(Tree* newTree){
        tree = newTree; 
    }

    void printServer(){
        
        cout << " ----- SERVER STATE START ----- " << endl; 
        tree->printTree(); 
        cout << " ------ SERVER STATE END ------ " << endl;
    }
    
};

int main(){
    cout << "hello world" << endl;
    
    Client c1; 
    c1.prinClient(); 

    c1.tree->nodes[0]->bucket->blocks[0]->data = "Testing 123456"; 

    Server s1; 
    s1.update(c1.tree); 

    s1.printServer(); 

}   
