
#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <iostream>
#include <thread>

#include "MpmcQueue.h"

TEST_CASE("General Testing") {
  
  // std::atomic<std::size_t> counter = 0;
  // const auto start = std::chrono::steady_clock::now();
  // for (int i = 0; i < 8'000'000; i++) {
  //   counter.fetch_add(1);
  // }
  // const auto end = std::chrono::steady_clock::now();
  // auto cpu_time_ns = end - start;
  
  // float avg_thread_time_ms = float(cpu_time_ns.count() / 1'000'000ull);
  // std::cout << "Average Thread Time: ms" << avg_thread_time_ms << std::endl;

  MpmcQueue<int, 64> queue;

  std::atomic<std::size_t> counter = 0;

  constexpr auto cycles = 1'000'000;
  auto count = 4;
  std::cout << count << std::endl;

  std::vector<std::chrono::nanoseconds> elapsed_times;
  elapsed_times.resize(2 * count);

  {
    std::vector<std::jthread> jThreadPool;
    jThreadPool.resize(2 * count);

    // 4 consumers
    for (int i = 0; i < count; i++) {
      jThreadPool[count + i] = std::move(std::jthread([&queue, &elapsed_time = elapsed_times[i + count], &counter] {
        const auto start = std::chrono::steady_clock::now();
        for (int i = 0; i < cycles; i++) {
          int s;
          queue.pop(s);
          counter.fetch_add(s);
        }
        const auto end = std::chrono::steady_clock::now();
        elapsed_time = end - start;
      }));
    }

    // 4 producers
    for (int i = 0; i < count; i++) {
      jThreadPool[i] = std::move(std::jthread([&queue, id = (i + 1), &elapsed_time = elapsed_times[i], &counter] {
        const auto start = std::chrono::steady_clock::now();
        for (int i = 0; i < cycles;i++) {
          queue.push(id);
        }
        const auto end = std::chrono::steady_clock::now();
        elapsed_time = end - start;
      }));
    }
  }

  std::chrono::nanoseconds cpu_time_ns{};
  for (int i = 0; i < count; i++) {
    cpu_time_ns += elapsed_times[i];
  }

  float avg_thread_time_ms = float(cpu_time_ns.count() / 1'000'000ull) / float(count * 2);

  std::cout << "Average Thread Time: ms" << avg_thread_time_ms << std::endl;

  auto result = counter.load();
  REQUIRE(result == ((1 + 2 + 3+4) * cycles));
}