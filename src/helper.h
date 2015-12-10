#ifndef _HELPER_H_
#define _HELPER_H_
#include <cstdio>
#include <cstdint>
using byte = uint8_t;
using word = uint16_t;

template<typename T>
inline word &to_word(T *p) {
  return *reinterpret_cast<word *>(p);
}

template<typename T>
inline byte &to_byte(T *p) {
  return *reinterpret_cast<byte *>(p);
}

template<typename T>
inline void *to_ptr(T &p) {
  return &p;
}

#endif
