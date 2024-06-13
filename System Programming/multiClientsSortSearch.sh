#!/bin/bash

# Number of clients to run simultaneously
NUM_CLIENTS=50

# Path to the client executable
CLIENT_EXEC="./clientSortSearch.o"

# Function to generate random input values for each client
generate_random_inputs() {
    sizeInMB=$((RANDOM % 1000 + 100)) # Random size between 100 and 1000 MB
    sortAlgo=$((RANDOM % 4 + 1))   # Random sort algorithm (1 to 4)
    searchAlgo=$((RANDOM % 4 + 1)) # Random search algorithm (1 to 4)
    keyToFind=$((RANDOM % 100 + 1)) # Random key to find (1 to 100)
    echo "$sizeInMB $sortAlgo $searchAlgo $keyToFind"
}

# Run multiple clients
for ((i = 0; i < NUM_CLIENTS; i++))
do
    inputs=$(generate_random_inputs)
    echo "Running client $((i + 1)) with inputs: $inputs"
    $CLIENT_EXEC <<< "$inputs" &
done

# Wait for all client processes to finish
wait

echo "All clients have finished."
