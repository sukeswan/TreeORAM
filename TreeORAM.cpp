#include <stdint.h>
#include <iostream>
#include <iterator>
#include <vector>
#include <map>
#include <cmath> 
#include <string>
#include <cstring>
#include <bitset>
#include <thread>
#include <chrono>

#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/rand.h>

using namespace std;
using namespace std::chrono;

class Block; 
class Bucket; 
class Node; 
class Tree; 
class Server; 
class Client; 

// -------------------------- CHECK THESE PARAMETERS ------------------------

const int N = 8; // size of tree
const int numNodes = N*2-1; // number of nodes in tree ~2N 
const int BUCKETSIZE = 3; // Size of bucket is logN        *** MANUAL FILL ***
const string DUMMY = "Dummy"; // Dummy data stored in dummy blocks 
const int PATHSIZE  = (log2(N)+1); // length of the path from root to leaf 
const int THREADS = 1; // # of threads to use (hardware concurrency for this laptop is 12)
const int MAX_DATA = 1000; // # of random data strings to create
const int DATA_SIZE = 10; // length of random data strings 

const int KEY_SIZE = 256; // for AES
const int IV_SIZE = 128; 
const int BYTE_SIZE = 8; 
const int BUFFER_SIZE = 1024; 

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

int randomRange(int small, int big){ // pick random value from range
    return small + rand() % ((big - small + 1));
}

void levelRange(int level, int range[]){ // get node index at level 

    int low = pow(2,level) - 1;
    int high = pow(2,level+1) -2; 

    range[0] = low; 
    range[1] = high; 

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
        for(int i = 0; i < size; i++){
            emptying[i] = 0; 
        }
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

    unsigned char* easy_encrypt(unsigned char *key){

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
        return ciphertext; 

    }

    void easy_decrypt(unsigned char *key){
    
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

    ~Block(){
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

    Block* pop(){ // pop off block of data if it exisits. otherwise return dummy

        Block* dummy = new Block(); // deleted 

        for(int i = 0; i < BUCKETSIZE; i++){

            if(blocks[i]->uid != -1){ // if data exists
                Block* temp = blocks[i]; 
                blocks[i] = dummy; 
                return temp; 
            }
        }
        return dummy; 

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
    

    void writeToBucket(int uid, int leaf, string data){

        for(int i = 0; i < BUCKETSIZE; i++){
            if (blocks[i]->uid == -1){
                delete blocks[i];
                blocks[i] = new Block(uid,leaf,data); // will delete when ~ called
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
            delete nodes[i]; 
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

Node* globalFetch(Server* s,int nid, unsigned char* key){ // fetch a node from the server and replace with dummy node

    Node* dummy = new Node();
    dummy->bucket->encryptBucket(key); 

    Node* wanted  = s->tree->nodes[nid];
    wanted->bucket->decryptBucket(key);  

    s->tree->nodes[nid] = dummy; // deleted with ~

    return wanted; 

}

void globalMultiFetch(Server* s, int start, int end, int* fetchingIndices, Node** store, unsigned char* key){
    for(int i=start; i <end; i++){  // fetch all the nodes from server
        store[i] = globalFetch(s, fetchingIndices[i], key); // parallelize 
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
                nodes[index]->bucket->blocks[b] = dummy; //replace with dummy block // deleted with ~
                b = BUCKETSIZE; 

            }
        }
}

void multiBucketCheck(int start, int end, Node** nodes,int targetUID, string* solution){

    for(int i=start; i <end; i++){  // fetch all the nodes from server
        checkBucket(nodes, i, targetUID, solution);
    }

}

void nodeToServer(Server* s, int i, int* path, Node** nodes, unsigned char* key){
    int nid = path[i]; 
    Node* n = nodes[i];
    n->bucket->encryptBucket(key);
    delete s->tree->nodes[nid];    
    s->tree->nodes[nid] = n; 
}

void multiNodesToServer(int start, int end, Server* s, int* path, Node** nodes, unsigned char* key){
    for(int i = start; i < end; i++){
        nodeToServer(s,i,path,nodes,key); 
    }
}

class Client{
    public: 
        map<int,int> position_map; // client will store position map
        unsigned char key[KEY_SIZE/BYTE_SIZE]; // deleted in Client deconstructor ]; 

    Client(){ // constructor for Node
        position_map.clear(); 
        generateKey(key);

    }

    Node* fetch(Server* s,int nid){ // fetch a node from the server and replace with dummy node

        Node* dummy = new Node();
        dummy->bucket->encryptBucket(key);

        Node* wanted  = s->tree->nodes[nid]; 
        wanted->bucket->decryptBucket(key);

        s->tree->nodes[nid] = dummy; // deleted with ~
        return wanted; 

    }

    void multiFetch(Server* s, int start, int end, int* fetchingIndices, Node** store){
        for(int i=start; i <end; i++){  // fetch all the nodes from server
            store[i] = fetch(s, fetchingIndices[i]); // parallelize 
        } // this function updated with parallel version called global multiFetch 
    }

    string readAndRemoveParallel(Server* s,int uid, int leaf){
        
        int path[PATHSIZE]; // deleted
        getPath(leaf, path); // get the indexes of the path
        int* pathP = path; 

        
        Node* nodes[PATHSIZE];
        Node** nP = nodes; 

        int step = PATHSIZE / THREADS; // how much to split up array 
        std::vector<std::thread> threads; 

        for (int i = 0; i < THREADS; i++) { // parallel fetching the buckets from the server
            int start = i * step; 
            int end = (i+1) * step; 
            threads.push_back(std::thread(globalMultiFetch, s, start, end, pathP, nP, key));
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
            threads.push_back(std::thread(multiNodesToServer, start, end, s, pathP, nP, key));
        }

        for (std::thread &t : threads) {
            if (t.joinable()) {
                t.join();
            }
        }  

        //Removes all elements in vector
        threads.clear();
        //Frees the memory which is not used by the vector
        threads.shrink_to_fit();

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

        root->bucket->encryptBucket(key);
        delete s->tree->nodes[0]; 
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

        root->bucket->encryptBucket(key); 
        delete s->tree->nodes[0]; 
        s->tree->nodes[0] = root;

        evict(s);  
    }

    void evict(Server* s){
        
        int numOfEvictions = 2*PATHSIZE-3;
        int evicting[numOfEvictions]; // number of blocks to evict *deleted*
        blocksToEvict(evicting); 
        
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

            delete real; 

            currentEvict->bucket->encryptBucket(key); 
            leftChild->bucket->encryptBucket(key); 
            rightChild->bucket->encryptBucket(key);

            delete s->tree->nodes[evictingIndex];
            delete s->tree->nodes[leftIndex];
            delete s->tree->nodes[rightIndex];

            s->tree->nodes[evictingIndex] = currentEvict; // put the eviction node and children back 
            s->tree->nodes[leftIndex] = leftChild; 
            s->tree->nodes[rightIndex] = rightChild; 
        }
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

        cout << " --------------------------" << endl;
    }

    ~Client(){ //deallocate
       
        position_map.clear(); 

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
    c1->initServer(s);  

    writing[index] = testWrites(c1,s);
    reading[index] = testReads(c1,s);  

    delete c1; 
    delete s; 

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

    delete [] reads; 
    delete [] writes; 

}

int main(){
    cout << "hello world" << endl;

    //multipleTests(1); 

    // Client* c1 =  Client();

    // Block b0(5,6, "Check");

    // b0.easy_encrypt(c1->key); 
    // b0.printBlock();

    // b0.easy_decrypt(c1->key); 
    // b0.printBlock();

    // unsigned char* key = generateKey(); 

    // Block* b0 = new Block(0,3, "Check0");
    // Block* b1 = new Block(1,4, "Check1");
    // Block* b2 = new Block(2,5, "Check2");

    // Bucket* bu = new Bucket(); 

    // bu->blocks[0] =  b0; 
    // bu->blocks[1] =  b1; 
    // bu->blocks[2] =  b2; 

    // bu->encryptBucket(key); 
    // bu->printBucket();

    // bu->decryptBucket(key); 
    // bu->printBucket();

    Client* c1 = new Client(); 
    Server* s = new Server();

    c1->initServer(s);  

    c1->write(s, 3,"Hello"); 
    c1->write(s, 8,"Working"); 
    c1->write(s, 1000,"yay!");
    
    c1->printClient(); 
    s->printServer();

    string hello = c1->read(s,3); 
    string working = c1->read(s,8);
    string yay = c1->read(s,1000); 

    c1->printClient(); 
    s->printServer(); 

    cout << hello << " " << working << " " << yay << endl; 

    delete c1; 
    delete s; 

    fscanf(stdin, "c"); // wait for user to enter input from keyboard
    //cout << uhoh << endl; 

}   

