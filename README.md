# Distributed Key-Value Store with Raft Consensus

A distributed key-value store implementation using gRPC for communication and Raft consensus algorithm for consistency across the cluster.

## Features

- Distributed key-value store with sharding
- Raft consensus algorithm implementation
- Consistent hashing for data distribution
- Command-line client interface
- Fault tolerance and leader election
- Concurrent request handling

## Architecture

### Components

1. **Client**
   - Command-line interface for PUT, GET, and ERASE operations
   - Round-robin load balancing for server connections
   - Reference: `client/src/main.cpp`

2. **Server**
   - Implements Raft consensus algorithm
   - Handles data sharding using consistent hashing
   - Manages distributed state across the cluster
   - Reference: `server/src/main.cpp`

3. **Raft Implementation**
   - Leader election
   - Log replication
   - Consensus management
   - Reference: `server/include/RaftNode.h`

## Building the Project
TODO


## Running the System

1. Start the servers:

./server/server



2. Start the client:

   ./client/client


## Usage

The client supports the following commands:

PUT key value # Insert or update a key-value pair
GET key # Retrieve a value by key
ERASE key # Delete a key-value pair


Example:

Enter command (PUT key value, GET key, ERASE key): PUT user1 john
Enter command (PUT key value, GET key, ERASE key): GET user1
Value found: john



## Testing

The project includes comprehensive unit tests for both the Raft consensus implementation and consistent hashing mechanism:




Key test files:
- `test/testRaftNode.cpp`: Tests for Raft consensus
- `test/testConsistentHashing.cpp`: Tests for consistent hashing

## Implementation Details

### Raft Consensus
- Leader election with randomized timeouts
- Log replication across the cluster
- Commit index management
- State machine replication

### Sharding
- Consistent hashing for data distribution
- Virtual nodes for better distribution
- Automatic rebalancing on node changes

### Concurrency
- Thread-safe operations
- Mutex-protected state changes
- Asynchronous request handling
