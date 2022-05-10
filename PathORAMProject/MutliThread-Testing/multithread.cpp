#include <iostream>
#include <stdint.h>
#include <vector>
#include <thread>
#include <time.h>
#include <chrono>
using namespace std;
using namespace std::chrono;
#define SIZE 10000

void doubleArray(int* a){

    for(int i = 0; i < SIZE; i++){
        a[i] = a[i]*2;
    }
}

void doubleNumber(int* a){
    *a = *a *2; 
}

void printArray(int* a){
    cout << "["; 
    for(int i = 0; i < SIZE; i++){
        cout << a[i] << ", "; 
    }
    cout << "]" << endl; 
}

int multi(){
    cout << "--- Multi ---" << endl; 
    auto start = high_resolution_clock::now();

    int a[SIZE]; 

    for(int i =0; i < SIZE; i++){
        a[i] = i; 
    }

    std::vector<std::thread> threads;

    for (int i = 0; i < SIZE; i++) {
        threads.push_back(std::thread(doubleNumber, &a[i]));
    }
    
    cout << threads.size() << endl; 

    for (auto &th : threads) {
        th.join();
    }

    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);
    cout << "Time taken by function: " << duration.count() << " microseconds" << endl;
    cout << "--- Mutli ---" << endl; 

    return duration.count(); 
}

int serial(){

    cout << "--- Serial ---" << endl; 
    auto start = high_resolution_clock::now();
    int a[SIZE]; 

    for(int i =0; i < SIZE; i++){
        a[i] = i; 
    }

    for(int i =0; i < SIZE; i++){
        doubleNumber(&a[i]);
    }

    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);
    cout << "Time taken by function: " << duration.count() << " microseconds" << endl;
    cout << "--- Serial ---" << endl;

    return duration.count(); 
}


int main () {

    //auto start = high_resolution_clock::now();
    cout << "heyo" << endl; 

    // int a[SIZE]; 

    // for(int i =0; i < SIZE; i++){
    //     a[i] = i; 
    // }

    // int* aP = a;

    // doubleArray(aP); 
    // printArray(aP); 

    // int b = 5; 
    // int* bP = &b; 
    // doubleNumber(bP);

    // cout << b << endl;

    // std::vector<std::thread> threads;
 
    // for (int i = 0; i < SIZE; i++) {
    //     threads.push_back(std::thread(doubleNumber, &a[i]));
    // }
    
    // for (auto &th : threads) {
    //     th.join();
    // }

    // printArray(aP); 

    // unsigned int n = std::thread::hardware_concurrency();
    // std::cout << n << " concurrent threads are supported.\n";

    // auto stop = high_resolution_clock::now();
    // auto duration = duration_cast<microseconds>(stop - start);
    // cout << "Time taken by function: " << duration.count() << " microseconds" << endl;
    int s = serial();
    int m = multi();

    cout << "multi is " << m/s << " X slower" << endl;

    return 0;
}

// auto start = high_resolution_clock::now();
// auto stop = high_resolution_clock::now();
// auto duration = duration_cast<microseconds>(stop - start);
// cout << "Time taken by function: " << duration.count() << " microseconds" << endl;