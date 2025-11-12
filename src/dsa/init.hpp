#pragma once
#ifndef DSA_INIT_HPP
#define DSA_INIT_HPP

extern "C" {
#include <accel-config/libaccel_config.h>
#include <linux/idxd.h>
}

class Dsa {
public:
  Dsa();
  ~Dsa();

  accfg_ctx *context() const noexcept { return ctx_; }

private:
  accfg_ctx *ctx_;

  Dsa(const Dsa&) = delete;
  Dsa& operator=(const Dsa&) = delete;
};

#endif