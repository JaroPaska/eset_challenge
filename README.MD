# Quickstart

This is a simple program for searching files for occurrences of some search string. To download and build it, clone the repository and then build using CMake, for example
```
git clone https://github.com/JaroPaska/eset_challenge.git
cd eset_challenge
mkdir cmake-build-release
cd cmake-build-release
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```
Running the program
```
./minigrep <directory path or file path> <search string>
```

Additionally, a benchmark script written in Python 3 is provided. This script creates a file on disk, then runs minigrep and times the execution. The benchmark can be run using
```
python benchmark.py <minigrep path>
```