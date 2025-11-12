#include <iostream>

#include "dsa/init.hpp"

int main() {
  try {
    Dsa dsa;
    accfg_ctx *ctx = dsa.context();
    accfg_device *device = nullptr;
    accfg_wq *wq = nullptr;
    int device_count = 0;
    int total_wq_count = 0;
    accfg_device_foreach(ctx, device) {
      device_count++;
      const char *devname = accfg_device_get_devname(device);
      int dev_id = accfg_device_get_id(device);
      const char *type_str = accfg_device_get_type_str(device);
      std::cout << "Device " << dev_id << " (" << devname << "): " << type_str
                << "\n";
      int wq_count = 0;
      accfg_wq_foreach(device, wq) {
        wq_count++;
        total_wq_count++;
        const char *wq_name = accfg_wq_get_devname(wq);
        const char *wq_type = accfg_wq_get_type_name(wq);
        auto mode = accfg_wq_get_mode(wq);
        auto state = accfg_wq_get_state(wq);
        uint64_t max_xfer = accfg_wq_get_max_transfer_size(wq);
        unsigned int max_batch = accfg_wq_get_max_batch_size(wq);
        std::cout << "  WQ " << accfg_wq_get_id(wq) << " (" << wq_name << ")"
                  << " type=" << wq_type << " mode="
                  << (mode == ACCFG_WQ_SHARED      ? "shared"
                      : mode == ACCFG_WQ_DEDICATED ? "dedicated"
                                                   : "unknown")
                  << " state="
                  << (state == ACCFG_WQ_ENABLED     ? "enabled"
                      : state == ACCFG_WQ_DISABLED  ? "disabled"
                      : state == ACCFG_WQ_QUIESCING ? "quiescing"
                      : state == ACCFG_WQ_LOCKED    ? "locked"
                                                    : "unknown")
                  << " max_xfer=" << max_xfer << " max_batch=" << max_batch
                  << "\n";
      }
      std::cout << "  WQs: " << wq_count << "\n";
    }
    if (device_count == 0) {
      std::cout << "No DSA/IAX devices found. Ensure devices are enabled and "
                   "WQs configured.\n";
    } else {
      std::cout << "Total devices: " << device_count
                << ", total WQs: " << total_wq_count << "\n";
    }
  } catch (const std::exception &ex) {
    std::cerr << "Error: " << ex.what() << "\n";
    return 1;
  }
  return 0;
}