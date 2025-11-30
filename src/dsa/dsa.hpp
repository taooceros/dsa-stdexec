#pragma once
#ifndef DSA_INIT_HPP
#define DSA_INIT_HPP

#include <cstddef>
#include <memory>

extern "C" {
#include <accel-config/libaccel_config.h>
#include <linux/idxd.h>
}

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
  Dsa();
  ~Dsa();

  void data_move(void *src, void *dst, size_t size);

  constexpr AccfgCtx const &context() const noexcept { return ctx_; }

private:
  AccfgCtx ctx_;

  accfg_wq *wq_;

  enum accfg_wq_mode mode_;

  void *wq_portal_;

  static constexpr std::size_t kWqPortalSize = 0x1000;

  void *map_wq(accfg_wq *wq);

  Dsa(const Dsa &) = delete;
  Dsa &operator=(const Dsa &) = delete;
};

#endif