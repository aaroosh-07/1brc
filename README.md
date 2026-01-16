# One Billion Row Challenge - C++ Implementation

This is my C++ implementation for the [One Billion Row Challenge](https://github.com/gunnarmorling/1brc), a challenge to process 1 billion temperature measurements as fast as possible.

The challenge input is a text file with 1 billion lines in the format:
```
station_name;temperature
```

The output is a sorted list of stations with their min/mean/max temperatures.

## Performance Results

### AMD Ryzen Threadripper PRO 7995WX (96 cores, 192 threads with SMT)

| Configuration | Computation Time | Speedup | Process Wall Time | Speedup |
|---------------|------------------|---------|-------------------|---------|
| Java baseline | ~116s | 1x | ~116s | 1x |
| C++ (1 thread) | 7,965 ms | 15x | 8.5s | 14x |
| C++ (12 threads) | 682 ms | 170x | 1.18s | 98x |
| C++ (48 threads) | 199 ms | 583x | 0.73s | 159x |
| C++ (96 threads) | 155 ms | 748x | 0.69s | 168x |
| C++ (191 threads) | 127 ms | 913x | 0.65s | 178x |

Using 191 threads (one less than hardware_concurrency) is slightly faster than 192, as it leaves one hyperthread available for the OS and other processes.

### Apple M4 Pro (8 performance + 4 efficiency cores)

| Configuration | Computation Time | Speedup | Process Wall Time | Speedup |
|---------------|------------------|---------|-------------------|---------|
| C++ (1 thread) | 11,182 ms | 1x | 11.24s | 1x |
| C++ (2 threads) | 5,839 ms | 1.9x | 5.89s | 1.9x |
| C++ (4 threads) | 3,104 ms | 3.6x | 3.16s | 3.6x |
| C++ (8 threads) | 1,652 ms | 6.8x | 1.71s | 6.6x |
| C++ (12 threads) | 1,414 ms | 7.9x | 1.47s | 7.6x |

The M4 Pro shows excellent scaling up to 8 threads (the performance cores). Adding efficiency cores provides modest additional speedup, with 12 threads being ~15% faster than 8 threads.

The gap between computation wall time and process wall time (especially at high thread counts) is due to kernel cleanup overhead for memory-mapped pages - this ~500ms overhead appears to be unavoidable.

## Implementation Details

### File I/O
- Uses `mmap` for memory-mapped file access, avoiding explicit read operations
- Each thread processes a contiguous chunk of the file
- Tweaking mmap/madvise flags (like `MADV_SEQUENTIAL`) had minimal impact on this machine where the file fits in cache, but provides a significant improvement on systems with limited memory (e.g., MacBook M1 with 16GB RAM)

### Custom Hash Tables
- Linear-probing hash tables with 32-byte aligned keys
- Outperformed `std::unordered_map` and `std::flat_map` in this use case
- Two-tier storage: small keys (≤31 bytes) in custom table, large keys fall back to `std::unordered_map`

### Hashing
- Tested many hash functions: murmur3, a5hash, aHash, clhash, komihash, and wyhash
- Top performers in order: wyhash (fast with good distribution), AES-based mixing (fastest but more collisions), Murmur3 (reliable fallback when `__uint128_t` or AES aren't available)
- Only the first 8 bytes of each station name are hashed, since they're already nearly unique in the test data
- If station names were modified to have similar prefixes, this strategy would need adjustment

### SIMD Optimizations (AVX2/NEON)
- AVX2 vectorized parsing to find semicolons and newlines
- SIMD-accelerated key comparison in hash table lookups
- AVX512 was explored, but opportunities were limited and benchmarks showed no improvement over AVX2

### Temperature Parsing
- The temperature format is highly constrained: `-?[0-9]{1,2}\.[0-9]`
- Only a few possible formats: `-X.X`, `X.X`, `-XX.X`, `XX.X`
- Loads temperature bytes into a 64-bit integer and uses simple branches to parse
- Branchless and SIMD approaches were tested but proved slower - the branch predictor handles these patterns extremely well

### Parallel Merge
- Each thread builds its own hash map (no synchronization during parsing)
- Results merged in log(n) parallel rounds using `std::barrier`
- At each round, half the threads merge their results into the other half

### Surprises
- **Branchless isn't always faster**: Multiple attempts at branchless code (temperature parsing, hash table insertion) were consistently slower than branched versions. The highly predictable patterns let the branch predictor shine.
- **AVX512 masked operations didn't help**: I hypothesized that masked loads could reduce instruction count when parsing variable-length lines, but 64-byte chunks weren't useful for single-line parsing, and the clock rate penalty of AVX512 likely offset any gains. Results may differ on newer architectures without this penalty.
- **Kernel overhead dominates at scale**: At high thread counts, the ~500ms mmap cleanup time exceeds the actual computation time. There's no way to avoid this - even exiting early without calling `munmap` still attributes cleanup cost to the process, so `time` shows higher wall-clock time than our internal measurements.

## Building and Running

Generate the test data (requires Java and Maven):
```bash
mvn package
./create_measurements.sh 1000000000
```

Run the Java baseline:
```bash
./calculate_average_baseline.sh
```

Run the C++ implementation:
```bash
./calculate_average_graphicsMan.sh
```

## Requirements

- C++23 compiler with `<print>` support (GCC 14+, Clang 18+)
- AVX2 support (most x86-64 processors from 2013+)
- NEON support for ARM builds
- Java 21+ and Maven for generating test data

## License

Apache License 2.0 (same as original 1BRC repository)
