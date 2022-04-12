#include <stdint.h>
#include <iostream>
#include <iterator>
#include <map>
#include <cmath> 
#include <string>
using namespace std;

class Client; 
class Server; 
class Block; 

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

    Block(int uidP, int leafP, string dataP){
        uid = uidP;
        leaf =leafP; 
        data = dataP; 
    }

    void printBlock(){
        cout << "BLOCK ID: " << uid << " LEAF: " << leaf << " DATA: " << data << endl; 

    }

};



int main(){
    cout << "hello world" << endl;


}
