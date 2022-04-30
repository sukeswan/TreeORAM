# g++ TreeORAM.cpp -o TreeORAM
# ./TreeORAM > output.txt

# g++ crypto.cpp -o crypto
# ./crypto 

# g++ -std=c++11 -pthread multithread2.cpp -o multithread2
# ./multithread2

main:
	clear
	g++ -std=c++17 -lpthread -lcrypto  TreeORAM.cpp -o TreeORAM 
	./TreeORAM > testing.txt

# profile: 
# 	perf record ./TreeORAM
# 	perf annotate 
