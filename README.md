# Simple c++11 demo of "Dining philosophers problem"
See https://en.wikipedia.org/wiki/Dining_philosophers_problem

## Configure
In your build directory

`cmake <path_to_source_dir>`

or

`cmake -DCMAKE_TOOLCHAIN_FILE=<path_to_your_cmake_toolchain_file> <path_to_source_dir>`

## Build
`cmake --build <path_to_build_dir>`

## Run
`<path_to_build_dir>/philosophers <number_of_philosophers> <max_interval_ms>`

## Command-line arguments
 * \<max_interval_ms\> number of philosophers/forks (default = 64)
 * \<max_interval_ms\> maximal interval eating/thinking state for philosophers in ms (default = 10000)

## Legend:
 * '|' - eating
 * '-' - waiting for forks
 * ' ' - thinking
 * '#' - dead (if enabled)
