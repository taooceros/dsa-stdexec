#pragma once
#include <exception>
#include <optional>
#include <stdexec/execution.hpp>
#include <tuple>
#include <variant>

namespace dsa_stdexec {

namespace detail {

template <class Loop, class... Values> struct SyncWaitReceiver {
  using receiver_concept = stdexec::receiver_t;
  Loop &loop_;
  std::optional<std::tuple<Values...>> &result_;
  std::exception_ptr &error_;

  void set_value(Values... values) && noexcept {
    try {
      result_.emplace(std::move(values)...);
    } catch (...) {
      error_ = std::current_exception();
    }
    loop_.finish();
  }

  void set_error(std::exception_ptr e) && noexcept {
    error_ = std::move(e);
    loop_.finish();
  }

  void set_stopped() && noexcept {
    loop_.finish();
  }

  auto get_env() const noexcept {
    return stdexec::empty_env{};
  }
};

} // namespace detail

template <class Sender, class Loop> auto wait_start(Sender &&snd, Loop &loop) {
  // Try to deduce result type.
  // If stdexec::value_types_of_t is not available, we might need a fallback.
  // But let's assume it is available or we can use completion_signatures.

  using ResultType = stdexec::value_types_of_t<Sender, stdexec::empty_env,
                                               std::tuple, std::optional>;

  // Helper lambda to instantiate receiver with deduced types
  return [&]<class... Values>(
             std::optional<std::tuple<Values...>> *) -> ResultType {
    std::optional<std::tuple<Values...>> result;
    std::exception_ptr error;

    auto op = stdexec::connect(
        std::forward<Sender>(snd),
        detail::SyncWaitReceiver<Loop, Values...>{loop, result, error});
    stdexec::start(op);

    loop.run();

    if (error) {
      std::rethrow_exception(error);
    }
    return result;
  }((ResultType *)nullptr);
}

} // namespace dsa_stdexec
