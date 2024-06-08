#!/bin/bash

# Number of clients to run simultaneously
NUM_CLIENTS=10

# Path to the client executable
CLIENT_EXEC="./clientHash.o"

# Run multiple clients
for ((i = 0; i < NUM_CLIENTS; i++))
do
    $CLIENT_EXEC
done

# Wait for all client processes to finish
wait

echo "All clients have finished."
