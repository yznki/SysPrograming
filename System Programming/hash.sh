#!/bin/bash

FILENAME=$1
HASH_FILE=$2
MTIME_FILE=$3

# Path to shasum (if not in the default PATH)
SHASUM_PATH="/usr/bin/shasum" # Update this if the path is different on your system

# Check if file exists
if [ ! -f "$FILENAME" ]; then
  echo "ERROR: FILE NOT FOUND" > "$HASH_FILE"
  exit 1
fi

# Compute the hash
$SHASUM_PATH -a 256 "$FILENAME" | awk '{print $1}' > "$HASH_FILE"

# Get the last modification time
stat -f %m "$FILENAME" > "$MTIME_FILE"
