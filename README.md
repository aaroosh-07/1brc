# One Billion Row Challenge - C++ Implementation

This is my C++ implementation for the [One Billion Row Challenge](https://github.com/gunnarmorling/1brc), a challenge to process 1 billion temperature measurements as fast as possible.

The challenge input is a text file with 1 billion lines in the format:
```
station_name;temperature
```

The output is a sorted list of stations with their min/mean/max temperatures.

## Performance Results
base line implementation 4 min 0.6 sec
With Memory Mapped Files 3 min 14 sec

## Implementation Details

### File I/O
- Uses `mmap` for memory-mapped file access, avoiding explicit read operations

## Building and Running

Generate the test data (requires python):
```bash
/src/main/python/create_measurements.py
```

Building C++ code
```bash
g++ --std=c++20 -O3 -march=native -mtune=native -flto -DNDEBUG ./calculateAvg.cpp -o <exe_name>
```

## Requirements

- C++20 compiler with `<print>` support (GCC 14+, Clang 18+)

## License

Apache License 2.0 (same as original 1BRC repository)
