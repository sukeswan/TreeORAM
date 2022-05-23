#include <stdint.h>
#include <iostream>
#include <iterator>
#include <vector>
#include <map>
#include <cmath> 
#include <string>
#include <cstring>
#include <bitset>
#include <chrono>

#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <gperftools/profiler.h>


using namespace std;
using namespace std::chrono;

class Block; 
class Bucket; 
class Node; 
class Tree; 
class Server; 
class Client; 

// -------------------------- CHECK THESE PARAMETERS ------------------------

const int N = 1024; // size of tree
const int numNodes = N*2-1; // number of nodes in tree ~2N 
const int BUCKETSIZE = 4; // Size of bucket is set to 4
const string DUMMY = "Dummy"; // Dummy data stored in dummy blocks 
const int PATHSIZE  = (log2(N)+1); // length of the path from root to leaf 
const int THREADS = 1; // # of threads to use (hardware concurrency for this laptop is 12)
const float TEST_CAPACITY = 0.75; // percentage you want to fill the tree when testing
const int MAX_DATA = int(N*TEST_CAPACITY); // # of random data strings to create
const int DATA_SIZE = 1000; // length of random data strings 

const int KEY_SIZE = 256; // for AES
const int IV_SIZE = 128; 
const int BYTE_SIZE = 8; 
const int BUFFER_SIZE = 1024; 
const bool ENCRYPTION=true;  // turn encryption on/off, helpful for debugging

// -------------------------- CHECK THESE PARAMETERS ------------------------

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

int randomLeafLeft(){ // get random leaf on left side of tree for eviction 
    int maxLeafLeft = smallLeaf() + (N/2) - 1; 
    return smallLeaf() + rand() % ((maxLeafLeft - smallLeaf() + 1)); // [smallLeaf, maxLeafLeft]
}

int randomLeafRight(){ // get random leaf on right side of tree for eviction 
    int minLeafRight = smallLeaf() + (N/2); 
    return minLeafRight + rand() % ((bigLeaf() - minLeafRight + 1)); // [minLeafRight, bigLeaf]
}

int randomRange(int small, int big){ // pick random value from range
    return small + rand() % ((big - small + 1));
}

void levelRange(int level, int range[]){ // get node index at level 

    int low = pow(2,level) - 1;
    int high = pow(2,level+1) -2; 

    range[0] = low; 
    range[1] = high; 

}

int getLevelOff1(int nodeID){ // level is offset by 1 as stash is counted as level 0
    int level = int(log2(nodeID+1));
    return level+1;
}

void pick2(int small, int big, int random[]){ // pick 2 random values from range

    random[0] = randomRange(small, big); 
    random[1] = randomRange(small, big);
    while(random[0] == random[1]){
        random[1] = randomRange(small, big);
    } 

}

void blocksToEvict(int toEvict[]){

    toEvict[0] = 0; //root will always be evicited 
    int next = 1; // where to store randomly selected buckets 
    for(int i =1; i < PATHSIZE-1; i++){ // choose eviction blocks from level 1 - leave level non inclusive 

        int level[2];
        levelRange(i, level); // get picking range for the level 

        int evictBuckets[2];

        pick2(level[0], level[1], evictBuckets); // pick 2 random blocks to evict

        toEvict[next] = evictBuckets[0]; //keep track of blocks to evict in array 
        toEvict[next+1] = evictBuckets[1]; 

        next = next + 2; 
    }

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

 void getPath(int leafid, int path[]){ // get the indexes for path to leaf 

    int backwards = PATHSIZE - 1; 
    int currentNode = leafid;

    while (backwards >= 0){
        path[backwards] = currentNode; 
        backwards--; 
        currentNode = parent(currentNode); 
    }
}

int commonAncestor(int leaf1, int leaf2){

    if (leaf1 == leaf2){
        return leaf1; 
    }
    else{
        return commonAncestor(parent(leaf1), parent(leaf2)); 
    }
}

void generateKey(unsigned char key[]){
    RAND_bytes(key, sizeof(KEY_SIZE/BYTE_SIZE));
}


string unsignedChar2String(unsigned char* input){
    string output = (reinterpret_cast<char*>(input));
    return output;
}

void handleErrors(void){
    ERR_print_errors_fp(stderr);
    abort();
}

int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key,
            unsigned char *iv, unsigned char *ciphertext){
    EVP_CIPHER_CTX *ctx;

    int len;

    int ciphertext_len;

    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new()))
        handleErrors();

    /*
     * Initialise the encryption operation. IMPORTANT - ensure you use a key
     * and IV size appropriate for your cipher
     * In this example we are using 256 bit AES (i.e. a 256 bit key). The
     * IV size for *most* modes is the same as the block size. For AES this
     * is 128 bits
     */
    if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
        handleErrors();

    /*
     * Provide the message to be encrypted, and obtain the encrypted output.
     * EVP_EncryptUpdate can be called multiple times if necessary
     */
    if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
        handleErrors();
    ciphertext_len = len;

    /*
     * Finalise the encryption. Further ciphertext bytes may be written at
     * this stage.
     */
    if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len))
        handleErrors();
    ciphertext_len += len;

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return ciphertext_len;
}

int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
            unsigned char *iv, unsigned char *plaintext){
    EVP_CIPHER_CTX *ctx;

    int len;

    int plaintext_len;

    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new()))

        handleErrors();

    /*
     * Initialise the decryption operation. IMPORTANT - ensure you use a key
     * and IV size appropriate for your cipher
     * In this example we are using 256 bit AES (i.e. a 256 bit key). The
     * IV size for *most* modes is the same as the block size. For AES this
     * is 128 bits
     */
    if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
        handleErrors();

    /*
     * Provide the message to be decrypted, and obtain the plaintext output.
     * EVP_DecryptUpdate can be called multiple times if necessary.
     */
    if(1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
        handleErrors();
    plaintext_len = len;

    /*
     * Finalise the decryption. Further plaintext bytes may be written at
     * this stage.
     */
    if(1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len))
        handleErrors();
    plaintext_len += len;

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return plaintext_len;
}

class Block{
    public:
        int uid; // unique identifier
        int leaf; // leaf each block is pointing to 
        string data;
        
        bool encrypted; 
        unsigned char iv[IV_SIZE/BYTE_SIZE];
        unsigned char ciphertext[BUFFER_SIZE]; 
        int cipher_len; 

    Block(){ // need default constructor for Bucket
        uid = -1;
        leaf = -1; 
        data = DUMMY; 
        
        emptyArray(iv, IV_SIZE/BYTE_SIZE); 
        encrypted = false;
        emptyArray(ciphertext, BUFFER_SIZE);  
        cipher_len = 0; 

    }

    Block(int uidP, int leafP, string dataP){ //plaintext constructor  for non dummy blocks
        uid = uidP;
        leaf = leafP; 
        data = dataP;

        emptyArray(iv, IV_SIZE/BYTE_SIZE);   
        encrypted = false; 
        emptyArray(ciphertext, BUFFER_SIZE);   
        cipher_len = 0; 

    }

    void generateIV(){
        RAND_bytes(iv, sizeof(IV_SIZE/BYTE_SIZE)); 
    }

    void emptyArray(unsigned char emptying[], int size){
        std::fill_n(emptying, BUCKETSIZE, 0x00); // using standard fill instead of for loop reduced latency by 1/2
        
        // for(int i = 0; i < size; i++){ 
        //     emptying[i] = 0; 
        // }
    }

    void makePayload(){ // create payload for encrypting data
        string uid_string = to_string(uid); 
        string leaf_string = to_string(leaf); 
        string payload = uid_string + "," + leaf_string + "," + data; 

        data = payload; // get ready for encryption!
    }

    void breakPayload(){ // remove uid and leaf from payload

        string delimiter = ",";

        size_t pos = 0;                   
        pos = data.find(delimiter); 
        string uid_string = data.substr(0, pos); 
        data.erase(0, pos + delimiter.length());
        uid = stoi(uid_string); 

        pos = 0;
        pos = data.find(delimiter); 
        string leaf_string = data.substr(0, pos); 
        data.erase(0, pos + delimiter.length());
        leaf= stoi(leaf_string); 

        emptyArray(iv, IV_SIZE/BYTE_SIZE);  
        encrypted = false; 
        emptyArray(ciphertext, BUFFER_SIZE);   
        cipher_len = 0;  

    }

    void string2UnsignedChar(unsigned char store[]){ 
        strcpy( (char*) store, data.c_str());

    }

    void easy_encrypt(unsigned char *key){

        if(ENCRYPTION){

            makePayload();
            generateIV(); 

            unsigned char plaintext[data.length()];
            string2UnsignedChar(plaintext);
            
            /*
            * Buffer for ciphertext. Ensure the buffer is long enough for the
            * ciphertext which may be longer than the plaintext, depending on the
            * algorithm and mode.
            */

            /* Encrypt the plaintext */
            cipher_len = encrypt (plaintext, strlen((char *)plaintext), key, iv, ciphertext);

            uid = -2; 
            leaf = -2; 
            data = ""; 
            encrypted = true; 
        }

    }

    void easy_decrypt(unsigned char *key){

        if(ENCRYPTION){
        
            unsigned char decryptedtext[BUFFER_SIZE];
            int decryptedtext_len;

            /* Decrypt the ciphertext */
            decryptedtext_len = decrypt(ciphertext, cipher_len, key, iv, decryptedtext);
            decryptedtext[decryptedtext_len] = '\0';
            data = unsignedChar2String(decryptedtext); 

            breakPayload();

            emptyArray(iv, IV_SIZE/BYTE_SIZE);  
            encrypted = false;

            emptyArray(ciphertext, BUFFER_SIZE);  
            cipher_len = 0; 

        }

    }

    void printCipher(){
        /* Do something useful with the ciphertext here */
        printf("Ciphertext for Encrypted Block: ");
        BIO_dump_fp (stdout, (const char *)ciphertext, cipher_len);
    }

    void printBlock(){ // print block info 
        if(encrypted){
            printCipher(); 
        }
        else{
            cout << "BLOCK ID: " << uid << " LEAF: " << leaf << " DATA: " << data << endl;
        }
        
    }
};

class Bucket{
    public: 
        Block* blocks[BUCKETSIZE]; // store pointers to blocks 
    
     Bucket(){

        for (int i = 0; i < BUCKETSIZE; i++){
            blocks[i] = new Block();  // intialize dummy blocks for bucket deleted in ~
        }
    }

    int findDummyIndex(){
        for (int i = 0; i < BUCKETSIZE; i++){
            if (blocks[i]->uid == -1){
                return i; 
            }
        }

        return -1; 
    }

    bool isFull(){
        return (findDummyIndex() == -1); 
    }

    void encryptBucket(unsigned char* key){
        for(int i = 0; i < BUCKETSIZE; i++){
            blocks[i]->easy_encrypt(key);
        }
    }

    void decryptBucket(unsigned char* key){
        for(int i = 0; i < BUCKETSIZE; i++){
            blocks[i]->easy_decrypt(key);
        }
    }

    void printBucket(){ // print bucket + blocks 

        cout<< "\n--- BUCKET w/ size " << BUCKETSIZE << " --- " << endl; 
        for (int i = 0; i < BUCKETSIZE; i++){
            blocks[i]->printBlock();

        }

        cout << "--------------------------" << endl;
    }

    ~Bucket(){

        for(int i = 0; i < BUCKETSIZE; i++){
            delete blocks[i]; 
        }

    }

};

class Node{
    public:

        Bucket* bucket;
        bool isLeaf; // keep track of leafs 

    Node(int index){ // pas index of node

        isLeaf = checkLeaf(index); // leaf boolean
        bucket = new Bucket(); // deleted with ~
    }

    Node(){ // dummy node for when removing things from server 

        isLeaf = false;
        bucket = new Bucket(); // deleted with ~

    }

    void writeToBucket(Block* b){ // write a block to the first empty bucket availibe 
        for(int i = 0; i < BUCKETSIZE; i++){
            if(bucket->blocks[i]->uid == -1){
                delete bucket->blocks[i]; 
                bucket->blocks[i] = b; 
                break; 
            }
        }
        return; 
    }

    void printNode(int index){ // print node info + bucket

        cout << "\n --- NODE " << index << " --- " << endl; 
        cout << "Left Index: " << left(index) << " Right Index: " << right(index) << " Parent Index: " << parent(index) << endl; 
        bucket->printBucket();
        
    }

    ~Node(){
        delete bucket; 
    }
};

class Tree{
    public:
        int levels; 
        Node* nodes[numNodes];

    Tree(){
        levels = log2(N);
        for (int i = 0; i < numNodes; i++){
            nodes[i] = new Node(i);  // intialize dummy blocks for bucket deleted with ~
        }
    }

    void printTree(){
        cout << "\n --- TREE of size " << numNodes << " --- " << endl;
        
        for(int i = 0; i < numNodes; i++){
            nodes[i]->printNode(i);
        }

    }

    ~Tree(){

        for(int i = 0; i < numNodes; i++){
            delete nodes[i]; 
        }

    }
};

class Server{ 
    public:
        Tree* tree;

    Server(){
        tree = new Tree(); // deleted with ~
    }

    void printServer(){
        
        cout << " ----- SERVER STATE START ----- " << endl; 
        tree->printTree(); 
        cout << " ------ SERVER STATE END ------ " << endl;
    }

    ~Server(){
        delete tree; 
    }
    
};

class Client{
    public: 
        map<int,int> position_map; // client will store position map
        vector<Block*> stash; 

        unsigned char key[KEY_SIZE/BYTE_SIZE]; // deleted in Client deconstructor ]; 

    Client(){ // constructor for Node
        position_map.clear(); 
        generateKey(key);

    }

    void initServer(Server* s){

        for(int i =0; i < numNodes; i++){
            Node* n = new Node(); // deleted with ~
            n->bucket->encryptBucket(key);
            delete s->tree->nodes[i]; 
            s->tree->nodes[i] = n;  
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

        cout << "\n --- Client Stash ---" << endl;
        for(Block* i : stash){
            i->printBlock(); 
        }

        cout << " --------------------------" << endl;
    }

    Node* fetch(Server* s,int nid){ // fetch a node from the server and replace with dummy node

        Node* dummy = new Node();
        dummy->bucket->encryptBucket(key);

        Node* wanted  = s->tree->nodes[nid]; 
        wanted->bucket->decryptBucket(key);

        s->tree->nodes[nid] = dummy; // deleted with ~
        return wanted; 

    }

    void deleteStashDummys(){
        int size = stash.size();

        Block* check; 

        for(int i = 0; i < size; i ++){
            check = stashGetBack();
            if (check->uid == -1){
                delete check; 
            }
            else{
                stashPutFront(check); 
            }

        }

        stash.shrink_to_fit(); // clear unused data
    }

    void fillStash(Server* s, int leaf){
        int path[PATHSIZE]; // deleted
        getPath(leaf, path); // get the indexes of the path
        for(int i = 0; i < PATHSIZE; i++){
            Node* current = fetch(s, path[i]); 
            for(int z = 0; z < BUCKETSIZE; z++){
                Block* original = current->bucket->blocks[z];
                Block* copy = new Block(original->uid,original->leaf,original->data); 
                stash.push_back(copy); 
            }
            delete current; 
        }
        deleteStashDummys();
    }

    Block* stashGetBack(){

        Block* check = stash.back();
        stash.pop_back();

        return check; 

    }

    void stashPutFront(Block* newFront){
        stash.insert(stash.begin(), newFront);
    }

    string read(Server* s, int uid){

        int oldLeaf = position_map[uid]; // create random new leaf for block
        fillStash(s,oldLeaf); 
        Block* found; 

        for(Block* i : stash){
            if(i->uid  == uid){
                found = i; 
                break; 
            }
        }
        string store = found->data; // store answer before encrypting it away!
        
        int newLeaf = randomLeaf();
        found->leaf = newLeaf; 
        position_map[uid] = newLeaf;

        evict(s,randomLeafLeft()); // evicit a path on the left and right side
        evict(s,randomLeafRight());
        return store; 

    }

    void write(Server* s, int uid, string data){

        if(position_map.find(uid) == position_map.end()){ // if uid not in position map (first write to tree)
            position_map[uid] = randomLeaf(); // assign a leaf node
            int leaf = position_map[uid];

            fillStash(s,leaf); // fetch path from stash

            Block* alpha = new Block(uid, leaf, data); // create new block and add it to the stash
            stash.push_back(alpha);

        }
        else{ // uid is in tree so get it and update it
            
            int oldLeaf = position_map[uid]; // create random new leaf for block
            int newLeaf = randomLeaf();
            position_map[uid] = newLeaf;

            fillStash(s,oldLeaf); // fetch path from stash

            for(Block* i : stash){ // iterate over stash, looking to update block 
                if(i->uid  == uid){
                    i->data = data; 
                    i->leaf = newLeaf; 
                    break; 
                }
            }
        }

        int rleft = randomLeafLeft(); 
        int rright = randomLeafRight();

        evict(s,rleft); // evicit a path on the left and right side 
        evict(s,rright); 
    }

    int deepestLevelStash(int leaf){ // find the deepest level a block in the stash can be stored
        int deepestLevel = -1; 
        for(Block* i : stash){
            int intersect = commonAncestor(leaf, i->leaf); 
            int intersectLevel = getLevelOff1(intersect); 

            if(intersectLevel > deepestLevel){
                deepestLevel = intersectLevel; 
            }
        }
        return deepestLevel; 
    }

    int deepestLevelNode(Node* n, int leaf){
        int deepestLevel = -1; 

        for(int i=0; i < BUCKETSIZE; i++){
            int intersect = commonAncestor(n->bucket->blocks[i]->leaf, leaf);
            int intersectLevel = getLevelOff1(intersect); 

            if(intersectLevel > deepestLevel){
                deepestLevel = intersectLevel; 
            }
        }

        return deepestLevel; 
    }

    Block* getDeepestAtNode(Node* n, int leaf){

        int deepestLevel = -1;
        int deepestIndex = -1;  

        for(int i=0; i < BUCKETSIZE; i++){
            int intersect = commonAncestor(n->bucket->blocks[i]->leaf, leaf);
            int intersectLevel = getLevelOff1(intersect); 

            if(intersectLevel > deepestLevel){
                deepestLevel = intersectLevel;
                deepestIndex= i;  
            }
        }

        Block* temp = n->bucket->blocks[deepestIndex]; 
        n->bucket->blocks[deepestIndex] = new Block(); 
        return temp; 


    }

    Block* getDeepestAtStash(int leaf){

        int deepestLevel = -1; 
        Block* deepestBlock; 

        int stashSize = stash.size();
        for(int i = 0; i < stashSize; i++){
            
            Block* maybe = stashGetBack(); 
    
            int intersect = commonAncestor(leaf, maybe->leaf); 
            int intersectLevel = getLevelOff1(intersect); 

            if(intersectLevel > deepestLevel){
                if(deepestLevel == -1){ // if this is the first block 
                    deepestLevel = intersectLevel;
                    deepestBlock = maybe;
                }
                else{ // you need to add deepestBlock back to the stash
                    stashPutFront(deepestBlock); 
                    deepestLevel = intersectLevel;
                    deepestBlock = maybe;
                }
                  
            }
            else{
                stashPutFront(maybe); 
            }
        }
        return deepestBlock; 
    }

    void prepareDeepest(int* deepest, Node** nodes, int leaf){

        int src = -1; 
        int goal = -1; 

        if (!stash.empty()){
            src = 0; 
            goal = deepestLevelStash(leaf);
        }

        for(int i = 1; i <= PATHSIZE; i++){
            if (goal >= i){
                deepest[i] = src; 
            }

            int l = deepestLevelNode(nodes[i],leaf); 

            if (l > goal){
                goal = l; 
                src = i; 
            }
        }
    }

    void prepareTarget(int* target, Node** nodes, int leaf, int* deepest){
        int src = -1; 
        int dest = -1; 

        for(int i = PATHSIZE; i >0; i--){
            if(i==src){
                target[i] = dest;
                dest = -1; 
                src = -1; 
            }

            if(((dest == -1) && !(nodes[i]->bucket->isFull()) || (target[i] != -1)) && (deepest[i] != -1)){
                src = deepest[i]; 
                dest = i; 
            }
        }
        int i = 0; // for stash
        if(i==src){
            target[i] = dest;
            dest = -1; 
            src = -1; 
        }
    }

    void evict(Server* s, int leaf){

        Block* dumdum = new Block(); 

        int path[PATHSIZE]; 
        getPath(leaf, path); 
        Node* nodes[PATHSIZE+1];
        Node** nP = nodes; 

        for(int i = 0; i < PATHSIZE; i++){
            nodes[i+1] = fetch(s, path[i]); // keep nodes[0] empty (stash level)
        }

        int deepest[PATHSIZE+1]; // deepest should also include spot for stash 
        std::fill_n(deepest, (PATHSIZE+1), -1);
        prepareDeepest(deepest,nP,leaf); 
 

        int target[PATHSIZE+1]; // target should also include spot for stash 
        std::fill_n(target, (PATHSIZE+1), -1);
        prepareTarget(target,nP,leaf, deepest); 

        Block* hold = dumdum; 
        int dest = -1; 

        for(int i = 0; i < PATHSIZE+1; i++){ // you could be storing a value at the leaf node (offset for this reason)
            Block* towrite = dumdum; 

            if((hold->data != DUMMY) && (i==dest)){
                towrite = hold; 
                hold = dumdum; 
                dest = -1; 
            }

            if(target[i] != -1){
                if(i == 0){
                    hold = getDeepestAtStash(leaf); 
                }
                else{
                    hold = getDeepestAtNode(nodes[i], leaf);
                }
                dest = target[i]; 
            }

            if(towrite->data !=DUMMY){
                nodes[i]->writeToBucket(towrite); 
            }
        }

        delete dumdum;


        for(int i = 0; i < PATHSIZE; i++){
            int index = path[i]; 
            Node* n = nodes[i+1]; // offset because of empty stash space
            delete s->tree->nodes[index]; 
            n->bucket->encryptBucket(key); 
            s->tree->nodes[index] = n; 
        }
    }

    int findNodeID(Server* s, int uid){ // return the nodeID of where uid is being stored // FOR TESTING PURPOSES ONLY
        int leaf = position_map[uid]; 
        int path[PATHSIZE]; 
        getPath(leaf, path); 

        for(int i = 0; i < PATHSIZE; i++){
            int nodeID = path[i]; 
            Node* curr = s->tree->nodes[nodeID]; 

            for(int b = 0; b < BUCKETSIZE; b++){
                if(curr->bucket->blocks[b]->uid == uid){
                    return nodeID; // return the nodeID
                }
            }
        }
        return -1; // uid not found on path
    }

    ~Client(){ //deallocate
        position_map.clear(); 

        for (int i = 0; i < stash.size(); i++){
            delete stash[i];
        }

        stash.shrink_to_fit();
        stash.clear(); 
    }
};

string randomAlphaString(const int len) {
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
        c1->write(s,i,randomAlphaString(DATA_SIZE)); // initially fill tree with data
    }

    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);

    int writes = duration.count();

    return (writes/MAX_DATA);
}

int testReads(Client* c1, Server* s){
    auto start = high_resolution_clock::now();

    for(int i = 0; i < MAX_DATA; i++){
        string data =  c1->read(s,i); // initially fill tree with data
    }

    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);
    int reads = duration.count();

    return (reads/MAX_DATA); 
}

void singleTest(int* reading, int* writing, int index){

    Client* c1 = new Client(); 
    Server* s = new Server();

    c1->initServer(s);  

    writing[index] = testWrites(c1,s);
    reading[index] = testReads(c1,s);  

    delete c1; 
    delete s; 

}

void multipleTests(int iterations){
    int* reads = new int[iterations]; 
    int* writes = new int[iterations]; 
    int* avg = new int[iterations]; 

    for(int i = 0; i < iterations; i++){
        singleTest(reads, writes, i); 
    }


    for(int i = 0; i < iterations; i++){
        avg[i] = int((reads[i] + writes[i])/2);
    }

    cout << "Results for Tree of Size 2^" << log2(N) << " (" << N << ")"  << endl; 
    cout << "Tree filled to " << TEST_CAPACITY*100 << "%" << endl; 
    cout << "Data strings of length " << DATA_SIZE << "\n" << endl;  

    cout << "___ Average Read Latency in microseconds ___" << endl; 
    printArraySize(reads, iterations); 

    cout << "___ Average Write Latency in microseconds ___" << endl; 
    printArraySize(writes, iterations); 

    cout << "___ Average Data Access Latency in microseconds ___" << endl; 
    printArraySize(avg, iterations); 

    delete [] reads; 
    delete [] writes; 
    delete [] avg; 

}

bool evictTest1(){ // return true if test case passes, false otherwise

    Client* c1 = new Client(); 
    Server* s = new Server();

    c1->initServer(s);

    delete s->tree->nodes[0]->bucket->blocks[0];
    delete s->tree->nodes[0]->bucket->blocks[1];

    delete s->tree->nodes[2]->bucket->blocks[1];
    delete s->tree->nodes[2]->bucket->blocks[0];

    delete s->tree->nodes[5]->bucket->blocks[0];
    delete s->tree->nodes[5]->bucket->blocks[1];

    s->tree->nodes[0]->bucket->blocks[0] = new Block(1,14,"a");
    s->tree->nodes[0]->bucket->blocks[1] = new Block(2,12,"b");

    s->tree->nodes[2]->bucket->blocks[0] = new Block(3,13,"c");
    s->tree->nodes[2]->bucket->blocks[1] = new Block(4,11,"d");

    s->tree->nodes[5]->bucket->blocks[0] = new Block(5,11,"e");
    s->tree->nodes[5]->bucket->blocks[1] = new Block(6,12,"f");

    c1->position_map[1] = 14;
    c1->position_map[2] = 12;

    c1->position_map[3] = 13;
    c1->position_map[4] = 11;

    c1->position_map[5] = 11;
    c1->position_map[6] = 12; 

    c1->evict(s,11); 

    bool checkA = c1->findNodeID(s,1) == 0; 
    bool checkB = c1->findNodeID(s,2) == 2; 
    bool checkC = c1->findNodeID(s,3) == 2;

    bool checkD = c1->findNodeID(s,4) == 11; 
    bool checkE = c1->findNodeID(s,5) == 5; 
    bool checkF = c1->findNodeID(s,6) == 5;

    delete s; 
    delete c1;

    return (checkA || checkB || checkC || checkD || checkE || checkF); 

}

bool evictTest2(){ // return true if test case passes, false otherwise

    Client* c1 = new Client();
    Server* s = new Server();
    c1->initServer(s);


    delete s->tree->nodes[0]->bucket->blocks[0];
    delete s->tree->nodes[0]->bucket->blocks[1];

    delete s->tree->nodes[1]->bucket->blocks[1];
    delete s->tree->nodes[1]->bucket->blocks[0];

    delete s->tree->nodes[4]->bucket->blocks[0];
    delete s->tree->nodes[4]->bucket->blocks[1];


    s->tree->nodes[0]->bucket->blocks[0] = new Block(1,7,"a");
    s->tree->nodes[0]->bucket->blocks[1] = new Block(2,8,"b");

    s->tree->nodes[1]->bucket->blocks[0] = new Block(3,9,"c");
    s->tree->nodes[1]->bucket->blocks[1] = new Block(4,9,"d");

    s->tree->nodes[4]->bucket->blocks[0] = new Block(5,10,"e");
    s->tree->nodes[4]->bucket->blocks[1] = new Block(6,10,"f");
    
    c1->position_map[1] = 7;
    c1->position_map[2] = 8;

    c1->position_map[3] = 9;
    c1->position_map[4] = 9;

    c1->position_map[5] = 10;
    c1->position_map[6] = 10; 

    c1->evict(s,10); 

    bool checkA = c1->findNodeID(s,1) == 1; 
    bool checkB = c1->findNodeID(s,2) == 0; 
    bool checkC = c1->findNodeID(s,3) == 4;

    bool checkD = c1->findNodeID(s,4) == 1; 
    bool checkE = c1->findNodeID(s,5) == 10; 
    bool checkF = c1->findNodeID(s,6) == 4;

    delete s; 
    delete c1;

    return (checkA || checkB || checkC || checkD || checkE || checkF); 

}

bool evictTest3(){ // return true if test case passes, false otherwise

    Client* c1 = new Client();
    Server* s = new Server();
    c1->initServer(s);

    delete s->tree->nodes[0]->bucket->blocks[0];
    delete s->tree->nodes[0]->bucket->blocks[1];

    delete s->tree->nodes[1]->bucket->blocks[1];
    delete s->tree->nodes[1]->bucket->blocks[0];

    delete s->tree->nodes[4]->bucket->blocks[0];
    delete s->tree->nodes[4]->bucket->blocks[1];

    delete s->tree->nodes[9]->bucket->blocks[0];

    Block* a  = new Block(1,10,"a"); 
    c1->stashPutFront(a); 

    s->tree->nodes[0]->bucket->blocks[0] = new Block(2,9,"b");
    s->tree->nodes[0]->bucket->blocks[1] = new Block(3,10,"c");

    s->tree->nodes[1]->bucket->blocks[0] = new Block(4,7,"d");
    s->tree->nodes[1]->bucket->blocks[1] = new Block(5,8,"e");

    s->tree->nodes[4]->bucket->blocks[0] = new Block(6,10,"f");
    s->tree->nodes[4]->bucket->blocks[1] = new Block(7,9,"g");

    s->tree->nodes[9]->bucket->blocks[0] = new Block(8,9,"h");

    c1->position_map[1] = 10; 

    c1->position_map[2] = 9;
    c1->position_map[3] = 10;

    c1->position_map[4] = 7;
    c1->position_map[5] = 8;

    c1->position_map[6] = 10;
    c1->position_map[7] = 9; 

    c1->position_map[8] = 9;

    c1->evict(s,9); 

    // c1->printClient();
    // s->printServer(); 

    bool checkA = c1->findNodeID(s,1) == 0; 
    bool checkC = c1->findNodeID(s,3) == 0;

    bool checkD = c1->findNodeID(s,4) == 1; 
    bool checkE = c1->findNodeID(s,5) == 1; 

    bool checkF = c1->findNodeID(s,6) == 4;
    bool checkG = c1->findNodeID(s,7) == 4; 

    bool checkB = c1->findNodeID(s,2) == 9; 
    bool checkH = c1->findNodeID(s,8) == 9;

    bool empty = c1->stash.empty(); 

    delete s; 
    delete c1;

    return (checkA || checkB || checkC || checkD || checkE || checkF || checkG || checkH || empty); 

}

int main(){
    cout << "hello world" << endl;

    multipleTests(2); 

    Client* c1 = new Client(); 
    Server* s = new Server();

    c1->initServer(s);

    c1->write(s, 1,"Hello"); 
    c1->write(s, 2,"Working"); 
    c1->write(s, 3,"yay!");

    c1->write(s, 4,"more"); 
    c1->write(s, 5,"data"); 
    c1->write(s, 6,"is good");

    c1->write(s, 7,"triple"); 
    c1->write(s, 8,"checking"); 
    c1->write(s, 9,"this stuff");

    c1->write(s, 10,"a"); 
    c1->write(s, 11,"b"); 
    c1->write(s, 12,"c"); 
    c1->write(s, 13,"d"); 
    c1->write(s, 14,"e");
    c1->write(s, 15,"f"); 
    c1->write(s, 16,"g"); 
    c1->write(s, 17,"h");
    c1->write(s,18,"i");

    c1->write(s, 19,"j"); 
    c1->write(s, 20,"k"); 

    c1->write(s, 21,"l");
    c1->write(s, 22,"m");
    c1->write(s, 23,"n");
    c1->write(s, 24,"o");
    c1->write(s, 25,"p");
    c1->write(s, 26,"q");
    c1->write(s,27,"r");
    c1->write(s, 28,"s");
    c1->write(s,29,"t");

    string hello = c1->read(s, 1); 

    string working = c1->read(s, 2); 
    string yay = c1->read(s, 3);

    string more = c1->read(s, 4); 
    string data = c1->read(s, 5); 
    string good = c1->read(s, 6);

    c1->printClient(); 
    s->printServer(); 
 
    cout << hello << " " << working << " " << yay << " " << more << " " << data << " " << good << endl;

    delete s; 
    delete c1;
}   