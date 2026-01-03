#pragma once
#ifndef DSA_INIT_HPP
#define DSA_INIT_HPP

#include <atomic>
#include <cstddef>
#include <memory>
#include <mutex>
#include <thread>

extern "C" {
#include <accel-config/libaccel_config.h>
#include <linux/idxd.h>
}

#include <dsa_stdexec/operation_base.hpp>

struct AccfgCtxDeleter {
  void operator()(accfg_ctx *ctx) const noexcept { accfg_unref(ctx); }
};

class AccfgCtx {
public:
  AccfgCtx() {
    accfg_ctx *ctx = nullptr;
    if (accfg_new(&ctx) != 0 || ctx == nullptr) {
      throw std::runtime_error("accfg_new failed to create libaccfg context");
    }
    ctx_.reset(ctx);
  }

  accfg_ctx *get() const noexcept { return ctx_.get(); }

private:
  std::unique_ptr<accfg_ctx, AccfgCtxDeleter> ctx_;
};

class Dsa {
public:
  explicit Dsa(bool start_poller = true);
  ~Dsa();

  void data_move(void *src, void *dst, size_t size);

  constexpr AccfgCtx const &context() const noexcept { return ctx_; }

  void submit(dsa_stdexec::OperationBase *op, dsa_hw_desc *desc);
  void submit(dsa_stdexec::OperationBase *op);
  void poll();

private:
  AccfgCtx ctx_;

  accfg_wq *wq_;

  enum accfg_wq_mode mode_;

  void *wq_portal_;

  static constexpr std::size_t kWqPortalSize = 0x1000;

  void *map_wq(accfg_wq *wq);

  dsa_stdexec::OperationBase *head_ = nullptr;
  std::mutex head_mutex_;
  std::thread poller_;
  std::atomic<bool> running_{false};

  Dsa(const Dsa &) = delete;
  Dsa &operator=(const Dsa &) = delete;
};

#endif