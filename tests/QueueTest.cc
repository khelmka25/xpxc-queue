
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

  MpmcQueue<int, 1024> queue;

  std::atomic<std::size_t> counter = 0;

  constexpr auto cycles = 1'000'000;
  auto count = 4;
  std::cout << count << std::endl;

  std::vector<std::chrono::nanoseconds> consumerTimes(count);
  std::vector<std::chrono::nanoseconds> producerTimes(count);

  {

    std::atomic_flag f(false);

    // 4 consumers
    std::vector<std::jthread> consumerThreads(count);
    for (int i = 0; i < count; i++) {
      consumerThreads[i] = std::move(std::jthread([&queue, &f, &elapsed_time = consumerTimes[i], &counter] {
        f.wait(false);
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
    std::vector<std::jthread> producerThreads(count);
    for (int i = 0; i < count; i++) {
      producerThreads[i] = std::move(std::jthread([&queue, &f, id = (i + 1), &elapsed_time = producerTimes[i], &counter] {
        f.wait(false);
        const auto start = std::chrono::steady_clock::now();
        for (int i = 0; i < cycles;i++) {
          queue.push(id);
        }
        const auto end = std::chrono::steady_clock::now();
        elapsed_time = end - start;
      }));
    }

    f.test_and_set();
    f.notify_all();
  }

  std::chrono::nanoseconds cpu_time_ns{};
  std::cout << "Producers:" <<std::endl;
  for (int i = 0; i < count; i++) {
    std::cout << "Thread " << i << ": " << std::chrono::duration_cast<std::chrono::milliseconds>(producerTimes[i]) << std::endl;
    cpu_time_ns += producerTimes[i];
  }

  std::cout << "Consumers:" <<std::endl;
  for (int i = 0; i < count; i++) {
    std::cout << "Thread " << i << ": " << std::chrono::duration_cast<std::chrono::milliseconds>(consumerTimes[i]) << std::endl;
    cpu_time_ns += consumerTimes[i];
  }

  auto result = counter.load();
  REQUIRE(result == ((1+2+3+4) * cycles));
}