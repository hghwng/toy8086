#include "helper.h"
#include <cstring>

class Memory {
private:
  byte *base_;

  // Memory size: 1 MiB
  static constexpr size_t bits_ = 20;
  static constexpr size_t size_ = 1 << bits_;
  static constexpr size_t mask_ = size_ - 1;

public:
  Memory() {
    base_ = new byte[size_ + 1];
    memset(base_, 0xcc, size_);
  }

  ~Memory() {
    delete[] base_;
  }

  template<typename T>
  T *get(size_t seg, size_t offset) {
    return (T *)(base_ + (((seg << 4) + offset) & mask_));
  }
};
