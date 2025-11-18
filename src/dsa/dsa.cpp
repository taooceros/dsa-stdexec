#include "dsa.hpp"
#include "accel-config/libaccel_config.h"
#include "enum_format.hpp" // IWYU pragma: keep
#include "fmt/base.h"
#include <cerrno>
#include <climits>
#include <fcntl.h>
#include <fmt/format.h>
#include <stdexcept>
#include <sys/mman.h>
#include <system_error>
#include <unistd.h>

Dsa::Dsa() : ctx_(nullptr), wq_(nullptr), wq_portal_(nullptr) {
  if (accfg_new(&ctx_) != 0 || ctx_ == nullptr) {
    throw std::runtime_error("accfg_new failed to create libaccfg context");
  }

  try {
    accfg_ctx *ctx = context();
    accfg_device *device = nullptr;
    accfg_wq *wq = nullptr;
    int device_count = 0;
    int total_wq_count = 0;
    accfg_device_foreach(ctx, device) {
      device_count++;
      const char *devname = accfg_device_get_devname(device);
      int dev_id = accfg_device_get_id(device);
      std::string type_str = accfg_device_get_type_str(device);
      fmt::println("Device {0} ({1}): {2}", dev_id, devname, type_str);
      int wq_count = 0;

      if (type_str != "dsa") {
        continue;
      }

      accfg_wq_foreach(device, wq) {
        wq_count++;
        total_wq_count++;
        std::string wq_name = accfg_wq_get_devname(wq);
        std::string wq_type = accfg_wq_get_type_name(wq);
        auto mode = accfg_wq_get_mode(wq);
        auto state = accfg_wq_get_state(wq);
        uint64_t max_xfer = accfg_wq_get_max_transfer_size(wq);
        unsigned int max_batch = accfg_wq_get_max_batch_size(wq);
        fmt::println("  WQ {0} ({1}) type={2} mode={3} state={4} max_xfer={5} "
                     "max_batch={6}",
                     accfg_wq_get_id(wq), wq_name, wq_type, mode, state,
                     max_xfer, max_batch);

        if (accfg_wq_get_type(wq) != ACCFG_WQT_USER) {
          continue;
        }

        // if (mode != ACCFG_WQ_DEDICATED) {
        //   continue;
        // }

        if (state != ACCFG_WQ_ENABLED) {
          continue;
        }
        fmt::println("  Mapping WQ portal for {0}", wq_name);

        void *portal = map_wq(wq);
        if (portal == MAP_FAILED) {
          continue;
        }

        wq_ = wq;
        wq_portal_ = portal;

        fmt::println("  Mapped WQ portal for {0} at {1}", wq_name, portal);
        break;
      }

      if (wq_portal_ != nullptr) {
        break;
      }
    }
    if (device_count == 0) {
      fmt::println("No DSA/IAX devices found. Ensure devices are enabled and "
                   "WQs configured.");
    } else {
      fmt::println("Total devices: {0}, total WQs: {1}", device_count,
                   total_wq_count);
    }

    if (wq_portal_ == nullptr) {
      throw std::runtime_error(
          "Failed to locate and map a usable user work queue portal");
    }
  } catch (const std::exception &ex) {
    fmt::println("Error: {0}", ex.what());
    throw;
  }
}

Dsa::~Dsa() {
  if (wq_portal_ != nullptr) {
    munmap(wq_portal_, kWqPortalSize);
    wq_portal_ = nullptr;
  }

  if (ctx_) {
    accfg_unref(ctx_);
    ctx_ = nullptr;
  }
}

void Dsa::data_move(void *src, void *dst, size_t size) {
  struct dsa_completion_record comp __attribute__((aligned(32)));

  struct dsa_hw_desc desc = {};

  desc.opcode = DSA_OPCODE_MEMMOVE;

  desc.flags = IDXD_OP_FLAG_RCR;

  desc.flags |= IDXD_OP_FLAG_CRAV;

  desc.flags |= IDXD_OP_FLAG_CC; // ddio?

  desc.xfer_size = size;
  desc.src_addr = reinterpret_cast<uint64_t>(src);
  desc.dst_addr = reinterpret_cast<uint64_t>(dst);

  comp.status = 0;

  desc.completion_addr = reinterpret_cast<uint64_t>(&comp);
}

void *Dsa::map_wq(accfg_wq *wq) {
  char path[PATH_MAX] = {};
  if (accfg_wq_get_user_dev_path(wq, path, sizeof(path)) != 0) {
    fmt::println("    Failed to get user device path for WQ {0}",
                 accfg_wq_get_id(wq));
    return MAP_FAILED;
  }

  int fd = open(path, O_RDWR);
  if (fd < 0) {
    std::error_code ec(errno, std::generic_category());
    fmt::println("    Failed to open {0}: {1}", path, ec.message());
    return MAP_FAILED;
  }

  void *portal = mmap(nullptr, kWqPortalSize, PROT_WRITE,
                      MAP_SHARED | MAP_POPULATE, fd, 0);
  std::error_code mmap_error;
  if (portal == MAP_FAILED) {
    mmap_error = std::error_code(errno, std::generic_category());
  }

  close(fd);

  if (portal == MAP_FAILED) {
    fmt::println("    Failed to mmap portal {0}: {1}", path,
                 mmap_error.message());
  }

  return portal;
}
