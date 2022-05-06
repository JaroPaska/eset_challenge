# About
This is a simple program for searching files for occurrences of a search string. It is a solution for ESET's [C++ Developer Challenge](https://join.eset.com/en/challenges/c-developer). A personal sidequest while making the project was to explore the use of constexpr and static_assert.

# Quickstart
To download and build it, clone the repository and then build using CMake, for example
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