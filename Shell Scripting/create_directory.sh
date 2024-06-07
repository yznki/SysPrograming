#!/bin/bash

read -p "Enter directory name: " dir_name

if [ -d "$dir_name" ]; then
    echo "Directory '$dir_name' already exists."
else
    mkdir -p "$dir_name"
    if [ $? -eq 0 ]; then
        echo "Directory '$dir_name' created successfully."
    else
        echo "Failed to create directory '$dir_name'."
    fi
fi
