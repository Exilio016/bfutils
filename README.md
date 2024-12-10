# BFUTILS
This project contains a collection of header-only libraries in C.

## How to use
To use any of these libraries you just need to copy it to your project.
By default, the .h files works like a normal header file, i.e. they declare the functions without any implementation, result in no code getting compiled.
So in addition, you need to instantiates the code in exactly one C source file. This is done by defining a specific macro (this is documented per-library).

For example to use `bfutils_vector.h` in one source file do:
```c
#define BFUTILS_VECTOR_IMPLEMENTATION
#include "bfutils_vector.h"
```
In the other source files, just include the library without the define:
```c
#include "bfutils_vector.h"
```
### CLI tool
The file [bfutils.c](./bfutils.c) builds to a CLI tool that downloads the bfutils header from github to your project.
#### Dependency
This tool depends on openssl. On ubuntu you can install the dependency with the following command:
```bash
sudo apt install libssl-dev
```
#### Build
To build the tool you can just run the following command:
```bash
cc -o bfutils bfutils.c -lssl
```
#### Usage
To use the tool just run `bfutils [option]`. Option can be one of the following:
| Option | Description |
| ------ | ----------- |
| -v | Downloads the bfutils_vector.h to current directory |
| -m | Downloads the bfutils_hash.h to current directory |
| -p | Downloads the bfutils_process.h to current directory |
| -t | Downloads the bfutils_test.h to current directory |
| -a | Downloads all bfutils headers to current directory |
| -h | Show the help menu and exits |

## Libraries
| File | Description |
| ---- | ----------- |
| [bfutils_vector.h](./bfutils_vector.h) | Provides dynamic arrays and string utilities |
| [bfutils_hash.h](./bfutils_hash.h) | Provides Hashmaps |
| [bfutils_process.h](./bfutils_process.h) | Utility funtions to create and work with process | 
| [bfutils_test.h](./bfutils_test.h) | Provides macros to create unit tests | 

## Tests
The source file [test.c](./test.c) contains unit tests for the libraries. The tests are created using `bfutils_vector.h`

## License
This project is licensed under the [MIT open source license](./LICENSE)
