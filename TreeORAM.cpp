#include <stdint.h>
#include <iostream>
#include <iterator>
#include <vector>
#include <map>
#include <cmath> 
#include <string>
#include <bitset>
using namespace std;

#define N 16
#define numNodes N*2-1
#define PATHSIZE int(log2(N)+1)
#define BUCKETSIZE 3 // log N
#define DUMMY "Dummy"

int smallLeaf(){ // returns smallest leaf value
        return N-1; 
}

int bigLeaf(){ // returns biggest leaf value 
        return 2*N -2; 
}

int randomLeaf(){ // TODO not crypto secure
     return smallLeaf() + rand() % ((bigLeaf() - smallLeaf() + 1)); // [smallLeaf, bigLeaf]
}

int randomRange(int small, int big){ // pick random value from range
    return small + rand() % ((big - small + 1));
}

int* levelRange(int level){ // get node index at level 

    int* range = new int[2];
    int low = pow(2,level) - 1;
    int high = pow(2,level+1) -2; 

    range[0] = low; 
    range[1] = high; 

    return range; 
}

int* pick2(int small, int big){ // pick 2 random values from range

    int* random = new int[2]; 

    random[0] = randomRange(small, big); 
    random[1] = randomRange(small, big);
    while(random[0] == random[1]){
        random[1] = randomRange(small, big);
    } 

    return random; 
}

int* blocksToEvicit(){

    int* toEvict = new int[2*PATHSIZE-3]; // number of blocks to evict
    toEvict[0] = 0; //root will always be evicited 
    int next = 1; // where to store randomly selected buckets 
    for(int i =1; i < PATHSIZE-1; i++){ // choose eviction blocks from level 1 - leave level non inclusive 

        int* level = levelRange(i); // get picking range for the level 
        int* evictBuckets = pick2(level[0], level[1]); // pick 2 random blocks to evict

        toEvict[next] = evictBuckets[0]; //keep track of blocks to evict in array 
        toEvict[next+1] = evictBuckets[1]; 

        next = next + 2; 

    }

    return toEvict; 

}

bool checkLeaf(int index){ // check if node is leaf

    if ((index >= smallLeaf()) && (index <= bigLeaf())){
            return true;
    }
    return false; 
}

int left(int index){ // calculate index of left child
    if (checkLeaf(index)){     // leaf has no right child
        return -1; 
    }
        return ((2*index) + 1);
}

int right(int index){ // calculate index of right child 
    if (checkLeaf(index)){      // leaf has no right child 
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

int* getPath(int leafid){ // get the indexes for path to leaf 

    int* path = new int[PATHSIZE]; 
    int backwards = PATHSIZE - 1; 
    int currentNode = leafid;

    while (backwards >= 0){
        path[backwards] = currentNode; 
        backwards--; 
        currentNode = parent(currentNode); 
    }

    return path; 

}

class Block{
    public:
        int uid; // unique identifier
        int leaf; // leaf each block is pointing to 
        string data;

    Block(){ // need default constructor for Bucket
        uid = -1;
        leaf = -1; 
        data = DUMMY; 
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

    Block* pop(){ // pop off block of data if it exisits. otherwise return dummy

        for(int i = 0; i < BUCKETSIZE; i++){

            if(blocks[i]->uid != -1){ // if data exists
                Block* temp = blocks[i]; 
                blocks[i] = new Block(); 
                return temp; 
            }

        }

        Block* dummy = new Block();
        return dummy; 

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

    Node(){ // dummy node for when removing things from server 

        isLeaf = false;
        bucket = new Bucket; 

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

    string readAndRemove(int uid, int leaf){
        
        int* path = getPath(leaf); // get the indexes of the path
        Block* dummy = new Block(); // dummy block to swap out 

        for(int n = 0; n < PATHSIZE; n++){ //iterate through nodes on path

            Node* currentNode = nodes[n]; 
            Bucket* currentBucket = currentNode->bucket; 

            for (int b = 0; b < BUCKETSIZE; b++){ // iterate through blocks in bucket

                Block* currentBlock = currentBucket->blocks[b]; 
                if(currentBlock->uid == uid){ // if ids are same
                    string data = currentBlock->data; //retreive data
                    nodes[n]->bucket->blocks[b] = dummy; //replace with dummy block
                    return data; 
                }
            }
        }
        return DUMMY; // return dummy data if block DNE 
    }

    void write(int uid, int leaf, string data){

        Node* root = nodes[0]; 

        for (int i = 0; i < BUCKETSIZE; i++){

            Block* current = root->bucket->blocks[i]; // current block to look at
            Block* newBlock = new Block(uid,leaf,data); // new block to write o tree

            if((current->data.compare(DUMMY)) == 0){ // if block is dummy in root
                root->bucket->blocks[i] = newBlock;  // replace dummy with new block 
                break; 
            }


        }

        // TODO: CALL EVICT FUNCTION
    }

    // void evict(){
        
    //     int* evicting = blocksToEvicit(); 
    //     int numOfEvictions = 2*PATHSIZE-3;

    //     for(int i = 0; i < numOfEvictions; i++){
            
    //         int evictingIndex = evicting[i];

    //         Node* currentEvict = nodes[evictingIndex]; 

    //         int leftIndex = left(evictingIndex); // get children that are getting new block
    //         int rightIndex = right(evictingIndex);
    //         Node* leftChild = nodes[leftIndex]; 
    //         Node* rightChild = nodes[rightIndex]; 

    //         Block* real = currentEvict->bucket->pop();

    //         if(real->uid == -1){ // no real blocks then nothing to evict
    //             return; 
    //         }

    //         else{
    //             int possible = real->leaf; 

    //             while (possible != leftIndex|| possible !=rightIndex){
    //                 possible = parent(possible); 
    //             }

    //             if(possible == leftIndex){
    //                 // write to block 
    //             }
    //             else{

    //             }

    //         }

    //     }

    // }

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
        Tree* tree;

    Client(){ // constructor for Node
        position_map.clear(); 
        tree = new Tree(); 

    }

    void write(int uid, string data){

        if(position_map.find(uid) == position_map.end()){ // if uid not in position map (first write to tree)
            position_map[uid] = randomLeaf(); 
        }
        
        int leaf = position_map[uid]; 
        tree->write(uid,leaf,data); 
    }

    string readAndRemove(int uid){

        int oldLeaf = position_map[uid]; // create random new leaf for block
        int newLeaf = randomLeaf();
        position_map[uid] = newLeaf;

        string data = tree->readAndRemove(uid,oldLeaf); 
        return data; 
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

    // Client c1; 

    // c1.write(3,"Hello"); 
    // c1.write(8,"Working"); 
    // c1.write(1000,"yay!");
    
    // c1.prinClient();

    // string hello = c1.readAndRemove(3); 
    // string working = c1.readAndRemove(8);
    // string yay = c1.readAndRemove(1000); 

    // c1.prinClient(); 

    // cout << hello << " " << working << " " << yay << endl; 

}   