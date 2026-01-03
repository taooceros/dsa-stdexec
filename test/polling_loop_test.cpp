#include <dsa_stdexec/run_loop.hpp>
#include <functional>
#include <iostream>

using namespace dsa_stdexec;

void free_function_poll() { std::cout << "free function poll\n"; }

struct FunctorPoll {
  void operator()() const { std::cout << "functor poll\n"; }
};

int main() {
  // Test 1: Lambda
  {
    PollingRunLoop loop([] { std::cout << "lambda poll\n"; });
    auto sched = loop.get_scheduler();
    (void)sched;
  }

  // Test 2: Free function
  {
    PollingRunLoop loop(free_function_poll);
    auto sched = loop.get_scheduler();
    (void)sched;
  }

  // Test 3: Functor
  {
    PollingRunLoop loop(FunctorPoll{});
    auto sched = loop.get_scheduler();
    (void)sched;
  }

  // Test 4: std::function
  {
    std::function<void()> f = [] { std::cout << "std::function poll\n"; };
    PollingRunLoop loop(f);
    auto sched = loop.get_scheduler();
    (void)sched;
  }

  // Test 5: Default template argument (std::function)
  {
    PollingRunLoop<> loop;
    auto sched = loop.get_scheduler();
    (void)sched;
  }

  std::cout << "All compilations successful\n";
  return 0;
}
