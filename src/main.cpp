#include <iostream>
#include <vector>

#include "dsa/dsa.hpp"
#include "fmt/base.h"

#include <atomic>
#include <dsa_stdexec/data_move.hpp>
#include <dsa_stdexec/scheduler.hpp>
#include <stdexec/execution.hpp>
#include <thread>

int main() {
  Dsa dsa;
  dsa_stdexec::DsaScheduler scheduler(dsa);

  std::string src = "Hello, World!";
  std::string dst(src.size(), 0);

  // Simple sync_wait with just() | dsa_data_move()
  // Note: dsa_data_move returns a sender.
  // We can use stdexec::start_detached or sync_wait.

  // Example 1: Direct usage
  auto snd =
      dsa_stdexec::dsa_data_move(dsa, src.data(), dst.data(), src.size());
  stdexec::sync_wait(std::move(snd));

  fmt::println("Direct: {}", dst);

  // Reset dst
  std::fill(dst.begin(), dst.end(), 0);

  // Example 2: On scheduler (simulated)
  // Since our scheduler just returns a sender that completes immediately,
  // this is mostly to show API compatibility.
  // In a real scenario, the scheduler would ensure we are on the right
  // thread/context. Here, dsa_data_move submits to hardware, so it "runs" on
  // hardware. The completion happens when we poll.

  // We need to drive the progress.
  // sync_wait will block. But who calls poll()?
  // Dsa::poll() needs to be called.
  //
  // If we use sync_wait, it blocks until set_value is called.
  // But set_value is called by notify(), which is called by poll().
  // So we cannot block in sync_wait if we are the one supposed to poll!
  //
  // We need a way to poll while waiting.
  // stdexec::sync_wait does not know about Dsa::poll().
  //
  // We need a custom run_loop or we need to run poll() in a separate thread.
  // Or we use a "polling" sync_wait?

  // Let's spawn a thread to poll.
  std::atomic<bool> running{true};
  std::thread poller([&]() {
    while (running) {
      dsa.poll();
      // Yield to avoid burning CPU too much if idle,
      // but for high perf we might spin.
      // For this test, let's sleep a bit or just yield.
      std::this_thread::yield();
    }
  });

  auto snd2 = stdexec::on(
      scheduler,
      dsa_stdexec::dsa_data_move(dsa, src.data(), dst.data(), src.size()));

  stdexec::sync_wait(std::move(snd2));

  fmt::println("Scheduled: {}", dst);

  running = false;
  poller.join();

  return 0;
}
