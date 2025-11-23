#pragma once

#include <array>
#include <atomic>
#include <new>
#include <type_traits>

template <typename T, unsigned N = 64>
class MpmcQueue {
 public:
  using IndexType = unsigned;

  // https://en.cppreference.com/w/cpp/thread/hardware_destructive_interference_size.html
  // alignas(/*std::hardware_destructive_interference_size*/ 64) std::atomic<IndexType> atm_rd_pos;
  // alignas(/*std::hardware_destructive_interference_size*/ 64) std::atomic<IndexType> atm_wr_pos;

  std::atomic<IndexType> atm_rd_pos;
  std::atomic<IndexType> atm_wr_pos;

  enum State : uint8_t { kEmpty, kFilling, kFull, kEmptying };

  static constexpr IndexType size() {
    return N;
  }

  static constexpr IndexType size_minus_one() {
    return N - 1u;
  }

  std::atomic<uint8_t> cellStates[N];
  T elements[N];

  MpmcQueue() : atm_rd_pos(0u), atm_wr_pos(0u) {
    static_assert(std::atomic<IndexType>::is_always_lock_free, "Help me please!");
    for (int i = 0; i < N; i++) {
      cellStates[i] = State::kEmpty;
    }
  }

  void push(const T& value) noexcept {
    for (;;) {
      // get the current write position and increment the read index (for
      // unsigned, overflow is well defined)
      const IndexType wr_pos = atm_wr_pos.fetch_add(1u, std::memory_order_acquire) & (size_minus_one());
      // get the current state of this load address
      uint8_t expectedState = State::kEmpty;
      /*Were we successful in modifying the state?*/
      // https://en.cppreference.com/w/cpp/atomic/atomic/compare_exchange
      if (cellStates[wr_pos].compare_exchange_strong(expectedState, State::kFilling, std::memory_order_acq_rel,
                                                     std::memory_order_relaxed)) {
        // elements[wr_pos] = std::move(value);
        elements[wr_pos] = value;
        cellStates[wr_pos].store(State::kFull, std::memory_order_release);
        break;
      }
      // std::this_thread::yield();
    }
  }

  void pop(T& value) noexcept {
    for (;;) {
      // get the current read position and increment the read index (for
      // unsigned, overflow is well defined)
      const IndexType rd_pos = atm_rd_pos.fetch_add(1u, std::memory_order_acquire) & (size_minus_one());
      // get the current state of this load address
      uint8_t expectedState = State::kFull;
      /*Was the state as expected?*/
      if (cellStates[rd_pos].compare_exchange_strong(expectedState, State::kEmptying, std::memory_order_acq_rel,
                                                     std::memory_order_relaxed)) {
        value = (elements[rd_pos]);
        cellStates[rd_pos].store(State::kEmpty, std::memory_order_release);
        break;
      }
      // std::this_thread::yield();
    }
  }
};