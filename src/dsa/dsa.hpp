#pragma once
#ifndef DSA_INIT_HPP
#define DSA_INIT_HPP

#include <cstddef>

extern "C" {
#include <accel-config/libaccel_config.h>
#include <linux/idxd.h>
}

class Dsa {
public:
  Dsa();
  ~Dsa();

  void data_move(void *src, void *dst, size_t size);

  constexpr accfg_ctx *context() const noexcept { return ctx_; }

private:
  accfg_ctx *ctx_;

  accfg_wq *wq_;

  void *wq_portal_;

  static constexpr std::size_t kWqPortalSize = 0x1000;

  void *map_wq(accfg_wq *wq);

  Dsa(const Dsa &) = delete;
  Dsa &operator=(const Dsa &) = delete;
};

#endif