
# Multi-function Simulator

## Project Overview
This project is a simulator for a client-server model that provides two main services:
1. **File Hashing Service:** The server generates a hash value for a file sent by the client.
2. **Array Sorting and Searching Service:** The server sorts a large numeric array and searches for a specific value within the array using multiple algorithms.

The server can handle multiple clients simultaneously, ensuring efficient and rapid processing to minimize response times.

## Project Structure
```
.vscode/
|-- c_cpp_properties.json
|-- settings.json
Common/
|-- searchSortAlgos.c
|-- searchSortAlgos.h
|-- utilities.c
|-- utilities.h
Multiprogramming/
|-- main.c
|-- main.o
Sequential/
|-- main.c
|-- main.o
|-- Sample.txt
```

## Compilation and Execution
1. **Compilation:**
   ```sh
   gcc main.c -o main.o ../Common/searchSortAlgos.c ../Common/utilities.c -I$(brew --prefix openssl)/include -L$(brew --prefix openssl)/lib -lssl -lcrypto
   ```

2. **Execution:**
   ```sh
   ./main.o
   ```

## Configuration Details
### .vscode/c_cpp_properties.json
```json
{
    "configurations": [
        {
            "name": "Mac",
            "includePath": [
                "${workspaceFolder}/**",
                "/opt/homebrew/opt/openssl@3/include"
            ],
            "defines": [],
            "macFrameworkPath": [
                "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/System/Library/Frameworks"
            ],
            "compilerPath": "/usr/bin/clang",
            "cStandard": "c17",
            "cppStandard": "c++17",
            "intelliSenseMode": "macos-clang-arm64"
        }
    ],
    "version": 4
}
```

### .vscode/settings.json
```json
{
    "files.associations": {
        "*.java": "java",
        "*.html": "html",
        "searchsortalgos.h": "c",
        "utilities.h": "c"
    },
    "code-runner.executorMap": {
        "c": "cd $dir && gcc $fileName -o $fileNameWithoutExt.o ../Common/searchSortAlgos.c ../Common/utilities.c -I$(brew --prefix openssl)/include -L$(brew --prefix openssl)/lib -lssl -lcrypto && $dir$fileNameWithoutExt.o"
    }
}
```

## Features
- **File Hashing Service:** Generates and stores hash values for files. Re-sends precomputed hash if the file is sent again.
- **Array Sorting and Searching Service:** Sorts arrays using four different algorithms and searches for values using four different search algorithms. Clients can specify the algorithms to use.

## Running the Simulator
To run the simulator, execute the compiled binaries from the `Multiprogramming` or `Sequential` directories depending on the implementation you want to test.

## To Compile Everything
```sh
gcc ../Common/hashing.c -o ../Common/hashing.o -I$(brew --prefix openssl)/include -L$(brew --prefix openssl)/lib -lssl -lcrypto
gcc $(pkg-config --cflags --libs openssl) server.c -o server.o
gcc $(pkg-config --cflags --libs openssl) client.c -o client.o
```

## Future Work
- **Multithreading:** Implement multithreaded versions of the services to handle more clients simultaneously.
- **Enhancements:** Add more algorithms and optimize the existing ones for better performance.

## Contact
For any questions or further information, please contact Yazan Kiswani.
