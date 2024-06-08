#!/bin/bash

# Number of clients to run simultaneously
NUM_CLIENTS=50

# Path to the client executable
CLIENT_EXEC="./clientHash.o"

# File to be modified
FILE="sample.txt"

# Function to modify the file content midway
modify_file() {
    echo "Modifying the file content..."
    echo "Random content: $(date +%s)" > "$FILE"
    echo "File content modified."
}

# Run multiple clients
for ((i = 0; i < NUM_CLIENTS; i++))
do
    $CLIENT_EXEC &
    if (( i == NUM_CLIENTS / 2 )); then
        sleep 1 # Introduce a delay to ensure the file is read by some clients first
        modify_file
    fi
done

# Wait for all client processes to finish
wait

echo "All clients have finished."
