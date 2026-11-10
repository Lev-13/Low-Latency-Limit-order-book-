# Ultra-Low-Latency Limit Order Book (LOB) Engine

A high-performance, ultra-low-latency Limit Order Book (LOB) and matching engine written in C++20. Designed for algorithmic trading and high-frequency trading (HFT) environments, this engine is optimized for maximum throughput and deterministic execution, achieving **over 3.7 million transactions per second (TPS)**.

## Core Features & Supported Order Types

The matching engine processes time-in-force and conditional orders with strict price-time priority:
*   **Market Orders:** Executed immediately against resting liquidity at the best available price.
*   **Limit Orders:** Added to the book at a specified price or better; provides liquidity.
*   **Market-Limit Orders:** Executes as a market order up to available volume, then converts the remaining quantity to a resting limit order at the executed price.
*   **Stop Orders:** Remains hidden until a trigger price is reached, then converts into a standard market order.
*   **Stop-Limit Orders:** Becomes a limit order once the specified trigger price is traded in the market.

## Architecture & Data Structures

To bypass the allocation overhead and latency spikes typical of standard library containers, this engine utilizes custom-built data structures:
*   **Custom Memory Pools:** Pre-allocated memory blocks eliminate dynamic heap allocations (`new`/`delete`) in the critical path, ensuring consistent ultra-low latency.
*   **Lock-Free Ring Buffers:** Used for lockless, high-throughput message passing and event queuing.
*   **Intrusive Linked Lists:** Ensures rapid $O(1)$ insertions and deletions for price-level order tracking without cache-miss penalties.
*   **Optimized AVL Trees:** Manages price levels with custom memory management. (Note: standard AVL deletion logic was overhauled to fix inherent memory unlinking defects, ensuring stable continuous execution under heavy load).

## Hardware-Aware Optimizations

Achieving 3.7M+ TPS requires strict adherence to mechanical sympathy and hardware architecture:
*   **Cache Locality:** Data structures are padded and aligned to 64-byte cache lines to prevent false sharing and optimize L1/L2/L3 cache hits.
*   **Zero-Copy Execution:** The order routing and matching paths avoid unnecessary data duplication, passing pointers to memory pool objects.
*   **Compiler Directives:** Built and profiled using GCC/Clang with aggressive optimization flags (`-O3 -march=native -flto`) to leverage the host architecture's vectorization and instruction sets.

## Build Instructions

This project uses CMake and requires a compiler that supports C++20.

```bash
# Clone the repository
git clone [https://github.com/yourusername/Low-Latency-Limit-order-book.git](https://github.com/yourusername/Low-Latency-Limit-order-book.git)
cd Low-Latency-Limit-order-book

# Create build directory
mkdir build && cd build

# Configure and compile with optimizations
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
