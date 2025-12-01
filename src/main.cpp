#include "dsa/dsa.hpp"
#include "fmt/base.h"

#include <dsa_stdexec/data_move.hpp>
#include <dsa_stdexec/run_loop.hpp>
#include <dsa_stdexec/scheduler.hpp>
#include <stacktrace>
#include <stdexec/execution.hpp>
#include <thread>

int main() {
  Dsa dsa;

  // Create PollingRunLoop that polls DSA
  dsa_stdexec::PollingRunLoop loop([&dsa] { dsa.poll(); });

  // Run the loop in a background thread
  std::thread loop_thread([&loop] { loop.run(); });

  std::string src = "Hello, World!";
  std::string dst(src.size(), 0);

  auto snd = stdexec::on(
      loop.get_scheduler(),
      dsa_stdexec::dsa_data_move(dsa, src.data(), dst.data(), src.size()));

  stdexec::sync_wait(std::move(snd));

  fmt::println("Scheduled with PollingRunLoop: {}", dst);

  loop.finish();
  loop_thread.join();

  return 0;
}
