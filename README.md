
# Multi-function Simulator

## Project Overview
This project is a simulator for a client-server model that provides two main services:
1. **File Hashing Service:** The server generates a hash value for a file sent by the client.
2. **Array Sorting and Searching Service:** The server sorts a large numeric array and searches for a specific value within the array using multiple algorithms.

The server can handle multiple clients simultaneously, ensuring efficient and rapid processing to minimize response times.

## Running the Simulator
To run the simulator, execute the compiled binaries from the desired directories depending on the implementation you want to test.

## Contact
For any questions or further information, please contact Yazan Kiswani.

## Compile Files in Sys Programming
```
gcc $(pkg-config --cflags --libs openssl) server.c -o server.o ../Common/searchSortAlgos.c ../Common/utilities.c                            
gcc $(pkg-config --cflags --libs openssl) clientSortSearch.c -o clientSortSearch.o ../Common/utilities.c
gcc $(pkg-config --cflags --libs openssl) clientHash.c -o clientHash.o ../Common/utilities.c
```