#ifndef DSA_STDEXEC_RUN_LOOP_HPP
#define DSA_STDEXEC_RUN_LOOP_HPP

#include "fmt/base.h"
#include <exception>
#include <functional>
#include <mutex>
#include <stacktrace>
#include <stdexec/execution.hpp>
#include <thread>
#include <utility>

namespace dsa_stdexec {

class PollingRunLoop;

struct Task {
  Task *next = this;
  union {
    void (*execute_fn)(Task *) noexcept;
    Task *tail;
  };

  void execute() noexcept { (*execute_fn)(this); }
};

template <class ReceiverId> struct Operation {
  using Receiver = stdexec::__t<ReceiverId>;

  struct task : Task {
    using __id = Operation;

    PollingRunLoop *loop;
    [[no_unique_address]] Receiver receiver;

    static void execute_impl(Task *p) noexcept {
      auto *op = static_cast<task *>(p);
      try {
        if (stdexec::get_stop_token(stdexec::get_env(op->receiver))
                .stop_requested()) {
          stdexec::set_stopped(static_cast<Receiver &&>(op->receiver));
        } else {
          stdexec::set_value(static_cast<Receiver &&>(op->receiver));
        }
      } catch (...) {
        stdexec::set_error(static_cast<Receiver &&>(op->receiver),
                           std::current_exception());
      }
    }

    explicit task(Task *tail) noexcept : Task{.tail = tail} {}

    task(Task *next, PollingRunLoop *loop, Receiver receiver)
        : Task{next, {&execute_impl}}, loop(loop),
          receiver(static_cast<Receiver &&>(receiver)) {}

    friend void tag_invoke(stdexec::start_t, task &self) noexcept {
      self.start();
    }

    void start() noexcept;
  };
};

class PollingRunLoop {
  template <class... Ts>
  using completion_signatures_ = stdexec::completion_signatures<Ts...>;

  template <class> friend struct Operation;

public:
  using PollFunc = std::function<void()>;

  struct Scheduler {
    using __t = Scheduler;
    using __id = Scheduler;
    auto operator==(const Scheduler &) const noexcept -> bool = default;

    struct ScheduleTask {
      using __t = ScheduleTask;
      using __id = ScheduleTask;
      using sender_concept = stdexec::sender_t;
      using completion_signatures =
          completion_signatures_<stdexec::set_value_t(),
                                 stdexec::set_error_t(std::exception_ptr),
                                 stdexec::set_stopped_t()>;

    private:
      friend Scheduler;

      template <class Receiver>
      using operation_t = Operation<stdexec::__id<Receiver>>::task;

      template <class Receiver>
      friend auto tag_invoke(stdexec::connect_t, ScheduleTask self,
                             Receiver receiver) {
        return self.connect(static_cast<Receiver &&>(receiver));
      }

      template <class Receiver> auto connect(Receiver receiver) const {
        return operation_t<Receiver>{&loop_->head_, loop_,
                                     static_cast<Receiver &&>(receiver)};
      }

      struct Env {
        PollingRunLoop *loop_;

        template <class CPO>
        friend auto tag_invoke(stdexec::get_completion_scheduler_t<CPO>,
                               const Env &self) noexcept -> Scheduler {
          return self.loop_->get_scheduler();
        }
      };

      friend auto tag_invoke(stdexec::get_env_t,
                             const ScheduleTask &self) noexcept -> Env {
        return Env{self.loop_};
      }

      explicit ScheduleTask(PollingRunLoop *loop) noexcept : loop_(loop) {}

      PollingRunLoop *const loop_;
    };

    friend PollingRunLoop;

    explicit Scheduler(PollingRunLoop *loop) noexcept : loop_(loop) {}

    friend auto tag_invoke(stdexec::schedule_t, const Scheduler &self) noexcept
        -> ScheduleTask {
      return self.schedule();
    }

    friend auto tag_invoke(stdexec::get_forward_progress_guarantee_t,
                           const Scheduler &) noexcept
        -> stdexec::forward_progress_guarantee {
      return stdexec::forward_progress_guarantee::parallel;
    }

    friend auto tag_invoke(stdexec::execute_may_block_caller_t,
                           const Scheduler &) noexcept -> bool {
      return false;
    }

    [[nodiscard]] auto schedule() const noexcept -> ScheduleTask {
      fmt::println("schedule");
      return ScheduleTask{loop_};
    }

    PollingRunLoop *loop_;
  };

  explicit PollingRunLoop(PollFunc poll = nullptr) : poll_(std::move(poll)) {}

  auto get_scheduler() noexcept -> Scheduler { return Scheduler{this}; }

  void run() {
    while (true) {
      Task *task = try_pop_front();
      if (task) {
        task->execute();
      } else {
        if (stop_) {
          break;
        }
        if (poll_) {
          poll_();
        }
        std::this_thread::yield();
      }
    }
  }

  void finish() {
    std::unique_lock lock{mutex_};
    stop_ = true;
  }

private:
  void push_back(Task *task) {
    std::unique_lock lock{mutex_};
    task->next = &head_;
    head_.tail = head_.tail->next = task;
  }

  auto try_pop_front() -> Task * {
    std::unique_lock lock{mutex_};
    if (head_.next == &head_) {
      return nullptr;
    }
    if (head_.tail == head_.next)
      head_.tail = &head_;
    return std::exchange(head_.next, head_.next->next);
  }

  std::mutex mutex_;
  Task head_{.tail = &head_};
  bool stop_ = false;
  PollFunc poll_;
};

template <class ReceiverId>
inline void Operation<ReceiverId>::task::start() noexcept {
  try {
    loop->push_back(this);
  } catch (...) {
    stdexec::set_error(static_cast<Receiver &&>(receiver),
                       std::current_exception());
  }
}

} // namespace dsa_stdexec

#endif
