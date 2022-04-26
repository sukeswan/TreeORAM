#include <stdint.h>
#include <iostream>
#include <iterator>
#include <vector>
#include <map>
#include <cmath> 
#include <string>
#include <bitset>
#include <thread>
#include <chrono>
using namespace std;
using namespace std::chrono;

// -------------------------- CHECK THESE PARAMETERS ------------------------

const int N = 4096; // size of tree
const int numNodes = N*2-1; // number of nodes in tree ~2N 
const int BUCKETSIZE = 12; // Size of bucket is logN        *** MANUAL FILL ***
const string DUMMY = "Dummy"; // Dummy data stored in dummy blocks 
const int PATHSIZE  = (log2(N)+1); // length of the path from root to leaf 
const int THREADS = 6; // # of threads to use (hardware concurrency for this laptop is 12)
const int MAX_DATA = 10000; // # of random data strings to create
const int DATA_SIZE = 10; // length of random data strings 

// -------------------------- CHECK THESE PARAMETERS ------------------------

class Block; 
class Bucket; 
class Node; 
class Tree; 
class Server; 
class Client; 

void printArray(int* a){
    cout << "["; 
    for(int i = 0; i < PATHSIZE; i++){
        cout << a[i] << ", "; 
    }
    cout << "]" << endl; 
}

void printArraySize(int* a, int size){
    cout << "["; 
    for(int i = 0; i < size; i++){
        cout << a[i] << ", "; 
    }
    cout << "]" << endl; 
}

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

    void writeToBucket(int uid, int leaf, string data){

        for(int i = 0; i < BUCKETSIZE; i++){
            if (blocks[i]->uid == -1){
                blocks[i] = new Block(uid,leaf,data);
                break; 
            }
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

    void printTree(){
        cout << "\n --- TREE of size " << numNodes << " --- " << endl;
        
        for(int i = 0; i < numNodes; i++){
            nodes[i]->printNode(i);
        }

    }
};

class Server{ 
    public:
        Tree* tree;

    Server(){
        tree = new Tree(); 
    }

    void printServer(){
        
        cout << " ----- SERVER STATE START ----- " << endl; 
        tree->printTree(); 
        cout << " ------ SERVER STATE END ------ " << endl;
    }
    
};

Node* globalFetch(Server* s,int nid){ // fetch a node from the server and replace with dummy node

    Node* dummy = new Node(); 
    Node* wanted  = s->tree->nodes[nid]; 

    s->tree->nodes[nid] = dummy; 

    return wanted; 

}

void globalMultiFetch(Server* s, int start, int end, int* fetchingIndices, Node** store){
    for(int i=start; i <end; i++){  // fetch all the nodes from server
        store[i] = globalFetch(s, fetchingIndices[i]); // parallelize 
    }
}

void checkBucket(Node** nodes, int index,int targetUID, string* solution){

    Block* dummy = new Block(); // dummy block to swap out 

    Node* currentNode = nodes[index]; 
    Bucket* currentBucket = currentNode->bucket; 

        for (int b = 0; b < BUCKETSIZE; b++){ // iterate through blocks in bucket

            Block* currentBlock = currentBucket->blocks[b]; 
            if(currentBlock->uid == targetUID){ // if ids are same
                *solution = currentBlock->data; //retreive data
                nodes[index]->bucket->blocks[b] = dummy; //replace with dummy block
                b = BUCKETSIZE; 

            }
        }
}

void multiBucketCheck(int start, int end, Node** nodes,int targetUID, string* solution){

    for(int i=start; i <end; i++){  // fetch all the nodes from server
        checkBucket(nodes, i, targetUID, solution);
    }

}

void nodeToServer(Server* s, int i, int* path, Node** nodes){
    int nid = path[i]; 
    Node* n = nodes[i]; 
    s->tree->nodes[nid] = n; 
}

void multiNodesToServer(int start, int end, Server* s, int* path, Node** nodes){
    for(int i = start; i < end; i++){
        
        nodeToServer(s,i,path,nodes); 
    }
}

class Client{
    public: 
        map<int,int> position_map; // client will store position map

    Client(){ // constructor for Node
        position_map.clear(); 

    }

    Node* fetch(Server* s,int nid){ // fetch a node from the server and replace with dummy node

        Node* dummy = new Node(); 
        Node* wanted  = s->tree->nodes[nid]; 

        s->tree->nodes[nid] = dummy; 

        return wanted; 

    }

    void multiFetch(Server* s, int start, int end, int* fetchingIndices, Node** store){
        for(int i=start; i <end; i++){  // fetch all the nodes from server
            store[i] = fetch(s, fetchingIndices[i]); // parallelize 
        }
    }

    string readAndRemoveParallel(Server* s,int uid, int leaf){
        
        int* path = getPath(leaf); // get the indexes of the path
        
        Node* nodes[PATHSIZE];
        Node** nP = nodes; 

        int step = PATHSIZE / THREADS; // how much to split up array 
        std::vector<std::thread> threads; 

        for (int i = 0; i < THREADS; i++) { // parallel fetching the buckets from the server
            int start = i * step; 
            int end = (i+1) * step; 
            threads.push_back(std::thread(globalMultiFetch, s, start, end, path, nP));
        }

        for (std::thread &t : threads) {
            if (t.joinable()) {
                t.join();
            }
        }

        string data = DUMMY;
        string* dP = &data;

        for (int i = 0; i < THREADS; i++) { // parallel checking buckets for target data 
            int start = i * step; 
            int end = (i+1) * step; 
            threads.push_back(std::thread(multiBucketCheck, start, end, nP, uid, dP));
        }

        for (std::thread &t : threads) {
            if (t.joinable()) {
                t.join();
            }
        }

        for (int i = 0; i < THREADS; i++) { // write path back to server
            int start = i * step; 
            int end = (i+1) * step; 
            threads.push_back(std::thread(multiNodesToServer, start, end, s, path, nP));
        }

        for (std::thread &t : threads) {
            if (t.joinable()) {
                t.join();
            }
        }      
        return data; // return dummy data if block DNE 
        
    }

    string read(Server* s, int uid){

        int oldLeaf = position_map[uid]; // create random new leaf for block
        int newLeaf = randomLeaf();
        position_map[uid] = newLeaf;

        string data = readAndRemoveParallel(s,uid,oldLeaf); 

        Node* root = fetch(s, 0); // fetch node and write back to it 

        if (data.compare(DUMMY) != 0){ // if block is not dummy, write it to tree
            root->bucket->writeToBucket(uid,newLeaf,data);  
        }

        s->tree->nodes[0] = root; 

        return data; 
    }

     void write(Server* s, int uid, string data){

        if(position_map.find(uid) == position_map.end()){ // if uid not in position map (first write to tree)
            position_map[uid] = randomLeaf(); 
        }
        
        int leaf = position_map[uid]; 

        readAndRemoveParallel(s,uid,leaf); //remove the value from tree if it exists 

        Node* root = fetch(s, 0); // fetch node and write back to it 

        if (data.compare(DUMMY) != 0){ // if block is not dummy, write it to tree
            root->bucket->writeToBucket(uid, leaf, data);  
        }

        s->tree->nodes[0] = root;

        evict(s);  
    }

    void evict(Server* s){
            
        int* evicting = blocksToEvicit(); 
        int numOfEvictions = 2*PATHSIZE-3;

        for(int i = 0; i < numOfEvictions; i++){
                
            int evictingIndex = evicting[i];

            Node* currentEvict = fetch(s,evictingIndex);  

            int leftIndex = left(evictingIndex); // get children that are getting new block
            int rightIndex = right(evictingIndex);
            Node* leftChild = fetch(s,leftIndex); 
            Node* rightChild = fetch(s, rightIndex); 

            Block* real = currentEvict->bucket->pop();

            if(real->uid != -1){ // real block so evict it

                int possible = real->leaf; 

                while (possible != leftIndex && possible !=rightIndex){
                        possible = parent(possible); 
                }

                if(possible == leftIndex){
                    leftChild->bucket->writeToBucket(real->uid, real->leaf, real->data); 
                }
                else{
                    rightChild->bucket->writeToBucket(real->uid, real->leaf, real->data); 
                }
            }

            s->tree->nodes[evictingIndex] = currentEvict; // put the eviction node and children back 
            s->tree->nodes[leftIndex] = leftChild; 
            s->tree->nodes[rightIndex] = rightChild; 
        }
    }

    void printClient(){ // print postion map at client 
        cout << "\n --- Client Position Map ---" << endl;
        map<int, int>::iterator itr;
        cout << "\tUID\tLEAF\n";
        for (itr = position_map.begin(); itr != position_map.end(); ++itr) {
            cout << '\t' << itr->first << '\t' << itr->second
                << '\n';
        }

        cout << " --------------------------" << endl;
    }
};

string randomString(const int len) {
    static const char alpha[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    string tmp_s;
    tmp_s.reserve(len);

    for (int i = 0; i < len; ++i) {
        tmp_s += alpha[rand() % (sizeof(alpha) - 1)];
    }
    
    return tmp_s;
}

int testWrites(Client* c1, Server* s){

    auto start = high_resolution_clock::now();

    for(int i = 0; i < MAX_DATA; i++){
        c1->write(s,i,randomString(DATA_SIZE)); // initially fill tree with data
    }

    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);

    int writes = duration.count();

    return writes;
}

int testReads(Client* c1, Server* s){
    auto start = high_resolution_clock::now();

    for(int i = 0; i < MAX_DATA; i++){
        string data =  c1->read(s,i); // initially fill tree with data
    }

    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);
    int reads = duration.count();

    return reads; 
}

void singleTest(int* reading, int* writing, int index){

    Client* c1 = new Client(); 
    Server* s = new Server(); 

    writing[index] = testWrites(c1,s);
    reading[index] = testReads(c1,s);  

}

void multipleTests(int iterations){
    int* reads = new int[iterations]; 
    int* writes = new int[iterations]; 

    for(int i = 0; i < iterations; i++){
        singleTest(reads, writes, i); 
    }

    cout << "___ Reads in microseconds ___" << endl; 
    printArraySize(reads, iterations); 

    cout << "___ Writes in microseconds ___" << endl; 
    printArraySize(writes, iterations); 

}

int main(){
    cout << "hello world" << endl;

    multipleTests(10); 

    // c1->write(s, 3,"Hello"); 
    // c1->write(s, 8,"Working"); 
    // c1->write(s, 1000,"yay!");
    
    // c1->printClient(); 
    // s->printServer();

    // string hello = c1->read(s,3); 
    // string working = c1->read(s,8);
    // string yay = c1->read(s,1000); 

    // c1->printClient(); 
    // s->printServer(); 

    // cout << hello << " " << working << " " << yay << endl; 

}   