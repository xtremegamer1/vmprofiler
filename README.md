# vmprofiler

VMProtect 3 Virtual Machines Profiler Library

# Build Requirements

```
clang-10 
cmake (3.x or up)
```

*linux build instructions*

```
# must be clang 10 or up
export CC=/usr/bin/clang
export CXX=/usr/bin/clang++

# clone and build
git clone https://githacks.org/vmp3/vmprofiler
cd vmprofiler
cmake -B build
cd build 
make
```