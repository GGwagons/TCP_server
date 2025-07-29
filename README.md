# TCP Server

This project implements a robust TCP server that handles timestamped price data with multiple clients. The project is organized with clean separation of source code, tests, and build artifacts.

## Project Structure

```
├── src/               # Server source code
│   └── main.c
├── tests/             # Test programs
│   ├── test_client.c
│   ├── stress_test.c
│   └── malformed_test.c
├── include/           # Header files
│   └── server.h
├── objects/           # Server object files (generated)
├── test_objects/      # Test object files (generated)
├── Makefile           # Build system
└── README.md          # This file
```

## Building

```bash
make all       # Build server and all test programs
make clean     # Remove object files only
make fclean    # Remove all built programs and object files
make re        # Clean and rebuild everything
```

**Build Features:**
- Organized object files in separate directories (`objects/` for server, `test_objects/` for tests)
- Automatic dependency tracking
- Clean separation of source and build artifacts
- Parallel build support

## Running the Server

```bash
# Start server normally
make server
# OR manually:
./price_server <port>

# Start server with memory checking
make server-val
# OR manually with valgrind:
valgrind --leak-check=full --show-leak-kinds=all --track-fds=yes ./price_server <port>
```

**Examples:**
```bash
make server           # Starts on port 8080
./price_server 9999   # Custom port
```

## Test Programs

**Quick Start:**
1. **Terminal 1**: `make server` (starts server on port 8080)
2. **Terminal 2**: `make test` (or `make fulltest` for all tests)

### 1. Basic Test Client (`test_client`)
Comprehensive test covering all basic functionality and edge cases:

```bash
make test
# OR manually:
./test_client 127.0.0.1 8080
```

**What it tests:**
- Basic insert/query operations (from Task.txt)
- Extreme values (INT32_MAX, INT32_MIN)
- Duplicate timestamps
- Large datasets (200+ entries)
- Out-of-order insertions
- Boundary conditions
- Zero and negative prices
- Empty ranges
- Rapid insert/query alternation

### 2. Multi-Client Stress Test (`stress_test`)
Tests concurrent client handling and session isolation:

```bash
make stress
# OR manually:
./stress_test 127.0.0.1 8080
```

**What it tests:**
- 10 concurrent clients
- 50 inserts per client (500 total)
- Session isolation (each client's data is separate)
- Concurrent queries
- Server stability under load

### 3. Malformed Message Test (`malformed_test`)
Tests server robustness against invalid/malformed messages:

```bash
make malformed
# OR manually:
./malformed_test 127.0.0.1 8080
```

**What it tests:**
- Partial messages
- Invalid message types
- Empty messages
- Oversized messages
- Non-printable characters
- Extreme value combinations
- Rapid-fire message sending

### 4. Run All Tests
Execute all test suites in sequence:

```bash
make fulltest
```

## Test Results Interpretation

### Expected Behaviors:
- **Session isolation**: Each client only sees its own data
- **Invalid ranges**: Queries with mintime > maxtime return 0
- **Empty ranges**: Queries with no matching data return 0
- **Malformed messages**: Server ignores invalid messages without crashing
- **Memory management**: No memory leaks (verify with valgrind)
- **File descriptor management**: No FD leaks
- **Concurrent handling**: Multiple clients work simultaneously

### Running with Valgrind:
```bash
make server-val    # Run server with valgrind memory checking
# OR in separate terminals:
# Terminal 1: valgrind --leak-check=full ./price_server 8080
# Terminal 2: make test
```

## Build System Details

The Makefile provides several convenient targets:

**Build Targets:**
- `make all` - Build everything (server + all tests)
- `make price_server` - Build only the server
- `make test_client` - Build only the basic test client
- `make stress_test` - Build only the stress test
- `make malformed_test` - Build only the malformed message test

**Server Targets:**
- `make server` - Start server on port 8080
- `make server-val` - Start server with valgrind on port 8080

**Test Targets:** (require server running separately)
- `make test` - Run comprehensive test suite
- `make stress` - Run multi-client stress test
- `make malformed` - Run malformed message test
- `make fulltest` - Run all tests in sequence

**Cleanup Targets:**
- `make clean` - Remove object directories only
- `make fclean` - Remove all built programs and objects
- `make re` - Full rebuild (clean + build)

## Protocol Specification

### Message Format:
- **Insert**: 'I' + 4-byte timestamp + 4-byte price (9 bytes total)
- **Query**: 'Q' + 4-byte mintime + 4-byte maxtime (9 bytes total)
- **Response**: 4-byte average price (for queries only)

All integers are in network byte order (big-endian).

### Key Features:
- **Per-client sessions**: Each connection maintains separate price data
- **Chronological ordering**: Prices are stored sorted by timestamp
- **Dynamic memory**: Automatically grows storage as needed
- **Error handling**: Graceful handling of memory allocation failures
- **Signal handling**: Clean shutdown on SIGINT/SIGQUIT
- **Bounds checking**: Protection against buffer overflows

## Debugging Tips

1. **Server crashes**: Use `make server-val` to check for memory leaks
2. **Wrong query results**: Verify timestamp ranges and data insertion order
3. **Connection issues**: Ensure server is running and port is available
4. **Performance issues**: Monitor with multiple concurrent clients using `make stress`
5. **Build issues**: Use `make clean` then `make all` to rebuild from scratch

## File Organization

- **Source files**: Keep server code in `src/`, tests in `tests/`
- **Object files**: Automatically organized in `objects/` and `test_objects/`
- **Executables**: Built in project root for easy access
- **Headers**: Centralized in `include/` directory

