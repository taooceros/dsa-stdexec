#include "init.hpp"
#include <stdexcept>
#
Dsa::Dsa() : ctx_(nullptr) {
  if (accfg_new(&ctx_) != 0 || ctx_ == nullptr) {
    throw std::runtime_error("accfg_new failed to create libaccfg context");
  }
}
#
Dsa::~Dsa() {
  if (ctx_) {
    accfg_unref(ctx_);
    ctx_ = nullptr;
  }
}
