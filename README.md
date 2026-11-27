# PRODUCER-CONSUMER

## OVERVIEW
this project is a multi-process bounded-buffer producer-consumer application written in C++. it uses POSIX shared memory and semaphores to synchronize multiple concurrent commodity price producers with a single consumer that displays a live terminal dashboard.

### FOLDER HIERARCHY
```
.
├── Makefile
├── README.md
├── consumer.cpp
└── producer.cpp
```

### FEATURES IMPLEMENTED
- **POSIX Shared Memory:** uses `shm_open()`, `ftruncate()`, and `mmap()` to map a shared circular buffer (`/shared_buffer`) with a flexible array member sized dynamically to `N` entries. both producers and the consumer safely handle initialization using `O_CREAT | O_EXCL` with fallback to `O_RDWR` on `EEXIST`.
- **Semaphore Synchronization:** embeds three process-shared POSIX semaphores (`mutex`, `isEmpty`, `isFull`) inside shared memory, initialized via `sem_init()` with `pshared = 1` to enforce mutual exclusion and bounded-buffer synchronization without busy waiting.
- **Producer (`producer.cpp`):**
  - accepts 5 command-line arguments: commodity name, mean ($\mu$), standard deviation ($\sigma$), sleep interval ($T$ in ms), and buffer size ($N$).
  - generates normally distributed random prices using C++ `<random>` (`std::default_random_engine` seeded with `std::random_device` and `std::normal_distribution`).
  - writes timestamped status logs (millisecond precision via `clock_gettime()`) to `stderr` for each generation, lock acquisition, buffer write, and sleep cycle.
- **Consumer (`consumer.cpp`):**
  - accepts the bounded-buffer size ($N$) as a command-line argument.
  - tracks 11 predefined commodities in alphabetical order (`ALUMINIUM` to `ZINC`) and calculates the moving average of the current and past 4 readings using `std::deque`.
  - renders a live, auto-refreshing terminal dashboard using ANSI escape sequences with color-coded trend arrows (green `↑` for increases, red `↓` for decreases) for both current prices and moving averages.
- **Input Validation and Error Handling:** validates argument counts and numeric bounds before execution, and checks return values on all POSIX system calls with `perror()`.

### COMPILATION AND EXECUTION
- to compile both executables, run `make` in the project directory
- to start the consumer, run `./consumer <buffer_size>` (e.g., `./consumer 40`)
- to start a producer, run `./producer <name> <mean> <stddev> <sleep_ms> <buffer_size>` (e.g., `./producer GOLD 1820 10 500 40`)
- to cleanup compiled binaries, type `make cleanup` in the terminal (and `rm -f /dev/shm/shared_buffer` to clear shared memory between runs)