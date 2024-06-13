#!/bin/bash

gcc $(pkg-config --cflags --libs openssl) server.c -o server.o ../Common/searchSortAlgos.c ../Common/utilities.c                            
gcc $(pkg-config --cflags --libs openssl) clientSortSearch.c -o clientSortSearch.o ../Common/utilities.c
gcc $(pkg-config --cflags --libs openssl) clientHash.c -o clientHash.o ../Common/utilities.c

clear

./server.o