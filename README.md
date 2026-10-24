# C++ Limit Order Book

A foundational **Limit Order Book (LOB) and matching engine** written in modern C++. The project implements core order management, price-time priority matching, and price-level liquidity aggregation. It is intended as a base implementation that can be expanded with additional order types, improved data structures, and low-latency optimisations.

## Current Features

- **Core matching engine:** Matches incoming bids and asks according to price-time priority (FIFO).
- **Type-safe domain primitives:** Uses fixed-width integer aliases such as `Price`, `Quantity`, and `OrderID` to make domain types explicit and help avoid unintended conversions.
- **Price-level aggregation:** Exposes total resting bid and ask liquidity by price level through `OrderBookLevelInfos`.
- **Order lifecycle and validation:** Tracks partial and full fills, with invariant checks to prevent orders from being over-filled.
- **Unit tests:** Includes assertion-based tests for order state transitions, error handling, and matching across multiple price levels.

## Project Structure

| File | Purpose |
|---|---|
| `types.h` | Fixed-width type aliases (including `std::int32_t`, `std::uint32_t`, and `std::uint64_t`) and price-level aggregation structures such as `OrderBookLevelInfos`. |
| `order.h` | Order representation, remaining-quantity tracking, and fill-validation logic. |
| `trade.h` | Trade execution structures recording matched bid and ask counterparties. |
| `orderbook.h` | Core order-book container, matching logic, and order-cancellation handling. |
| `main.cpp` | Program entry point and assertion-based test suite. |

## Build and Run

You will need a C++20-compatible compiler, such as `clang++` or `g++`.

From the project root, compile the project with Clang:

```bash
clang++ -std=c++20 -O3 main.cpp -o main
```

Run the executable:

```bash
./main
```

For development, you may prefer a build with additional warnings enabled:

```bash
clang++ -std=c++20 -Wall -Wextra -Wpedantic -g main.cpp -o main
```

The commands above assume the current implementation can be built from `main.cpp` and its included headers. Add any additional `.cpp` source files to the compile command if the project is split into multiple translation units later.

## Roadmap

Planned features are grouped into three phases. The checkboxes below indicate work that is planned, not functionality currently available.

### Phase 1: Order Types and Book Mechanics

- [ ] **Market orders:** Execute immediately against resting liquidity at the best available prices.
- [ ] **Time-in-force (TIF) conditions:** Add Immediate-or-Cancel (IOC), Fill-or-Kill (FOK), and Good-Till-Cancelled (GTC) behaviour.
- [ ] **Order modification (cancel/replace):** Support price and quantity changes, preserving queue priority when quantity is reduced where the matching rules permit.
- [ ] **Stop and iceberg orders:** Add conditional trigger orders and separate displayed and hidden quantities.

### Phase 2: Low-Latency and Memory Optimisations

- [ ] **Custom object pool or arena allocator:** Pre-allocate order memory to reduce general-purpose heap allocations on the matching hot path.
- [ ] **Cache-friendly data structures:** Explore flat or intrusive structures to improve CPU cache locality and reduce pointer-heavy traversal.
- [ ] **O(1) order lookup:** Map `OrderID` values to resting orders or their iterators for fast lookup and cancellation, subject to the chosen data structure.

### Phase 3: Tooling, Market Data, and Benchmarking

- [ ] **L2/L3 terminal visualiser:** Display order-book depth, bid-ask spread, volume by price level, and recent trade prints in the terminal.
- [ ] **Market-data replay:** Parse CSV or ITCH-style historical order flow to exercise the engine with realistic event sequences.
- [ ] **Benchmarking suite:** Measure throughput (orders per second) and latency percentiles (p50, p99, and p99.9, ideally in nanoseconds).

## Design Goals

The project is being developed around a few core goals:

- **Correctness first:** Make order-state transitions and matching behaviour testable and enforce key invariants.
- **Explicit domain types:** Use named aliases for core trading concepts rather than relying on interchangeable primitive values everywhere.
- **Incremental development:** Introduce new order behaviours and optimisations in stages, validating correctness before focusing on performance.
- **Measurable performance:** Use repeatable benchmarks before and after low-latency changes rather than assuming an optimisation is faster.

## Status

This is an evolving project. The features listed under **Current Features** describe the intended current implementation; items in the **Roadmap** are planned enhancements and should be marked complete as they are implemented and tested.
