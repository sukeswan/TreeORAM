#include <iostream>
#include <stdint.h>
#include <vector>
#include <thread>
#include <chrono>
using namespace std;
using namespace std::chrono;
#define SIZE 60000
#define TARGET 6840
#define THREADS 1

void doubleArray(int* a){

    for(int i = 0; i < SIZE; i++){
        a[i] = a[i]*2;
    }
}

void doubleArrayPart(int* a, int start, int end){

    for(int i = start; i < end; i++){
        a[i] = a[i]*2;
    }
}

void findValue(int* a, int start, int end, int target, int* write){

    for(int i = start; i < end; i++){
        if(a[i]==target){
            *write = a[i]; // value should store the target 
        }
    }
}

void multiFind(){
    cout << "--- Mutli ---" << endl; 
    auto start = high_resolution_clock::now();

    int number_of_threads = THREADS;
    int number_of_elements = SIZE;
    int step = number_of_elements / number_of_threads;
    std::vector<std::thread> threads;

    int a[SIZE]; 
    for(int i =0; i < SIZE; i++){
        a[i] = i; 
    }

    int* aP = a;
    int valueFound = -1; 

    for (int i = 0; i < number_of_threads; i++) {
        int start = i * step; 
        int end = (i+1) * step; 
        threads.push_back(std::thread(findValue, aP, start, end, TARGET, &valueFound));
    }

    for (std::thread &t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }
    cout << valueFound << endl; 
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);
    cout << "Time taken by function: " << duration.count() << " microseconds" << endl;
    cout << "--- Mutli ---" << endl; 

}

void serialFind(){

    cout << "--- Serial Find ---" << endl; 
    auto start = high_resolution_clock::now();

    int a[SIZE]; 
    for(int i = 0; i < SIZE; i++){
        a[i] =i; 
    }
    int* aP = a; 
    int valueFound = -1; 

    findValue(aP, 0, SIZE, TARGET, &valueFound); 

    cout << valueFound << endl; 

    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);
    cout << "Time taken by function: " << duration.count() << " microseconds" << endl;
    cout << "--- Serial Find ---" << endl;
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

    int number_of_threads = THREADS;
    int number_of_elements = SIZE;
    int step = number_of_elements / number_of_threads;
    std::vector<std::thread> threads;

    int a[SIZE]; 
    for(int i =0; i < SIZE; i++){
        a[i] = i; 
    }

    int* aP = a; 

    for (int i = 0; i < number_of_threads; i++) {
        threads.push_back(std::thread(doubleArrayPart, aP, i * step, (i + 1) * step));
    }

    for (std::thread &t : threads) {
        if (t.joinable()) {
            t.join();
        }
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

    serialFind(); 
    multiFind();
    return 0;
}
