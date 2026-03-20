# Generic Register Operation Optimizer 5 (groov)

`groov` is an open source hardware register abstraction library. It is designed
to provide a safe, efficient, and maintainable interface to hardware registers
in firmware.

`groov` presents an asynchronous interface for accessing registers. Hardware
registers are not always local to the processor's memory bus, they could be 
on a remote device over I2C, SPI, CAN, or another interface with a non-trivial
latency. An asynchronous interface is key to robustly and efficiently accessing
registers on remote buses.

`groov` uses [Intel's baremetal senders and receivers library](https://github.com/intel/cpp-baremetal-senders-and-receivers) to
provide a robust asynchronous execution framework. 

C++ standard support is as follows:

- C++23: [main branch](https://github.com/intel/generic-register-operation-optimizer/tree/main) (active development)
- C++20: [cpp20 branch](https://github.com/intel/generic-register-operation-optimizer/tree/cpp20) (supported)

See the [full documentation](https://intel.github.io/generic-register-operation-optimizer/).

Compiler support:

| Branch | GCC versions | Clang versions |
| --- | --- | --- |
| [main](https://github.com/intel/generic-register-operation-optimizer/tree/main) | 12 thru 14 | 19 thru 21 |
| [cpp20](https://github.com/intel/generic-register-operation-optimizer/tree/cpp20) | 12 thru 14 | 14 thru 21 |

# Quick start

The recommended way to use `groov` is with CMake and [CPM](https://github.com/cpm-cmake/CPM.cmake).
With this method,add the following to your `CMakeLists.txt`:

```cmake
CPMAddPackage("gh:intel/generic-register-operation-optimizer#d365927")
target_link_libraries(your_target PRIVATE groov)
```

Where `d365927` is the git hash (or tag, or branch) that you want to use.

Another option is to include `groov` as a 
[git submodule](https://github.blog/2016-02-01-working-with-submodules/) 
in your repo and add the `groov` directory in your `CMakeLists.txt` file:

```cmake
add_subdirectory(extern/generic-register-operation-optimizer)
target_link_libraries(your_target PRIVATE groov)
```

With either of these methods, `#include <groov/groov.hpp>` in your code to use `groov`.
