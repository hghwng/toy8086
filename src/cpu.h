#include "helper.h"
#include "mem.h"
#ifdef TOY8086_MSVC
#  include <intrin.h>
#  define __builtin_popcount __popcnt16
#endif

struct Flag {
  union {
    struct {
      bool o : 1;
      bool s : 1;
      bool z : 1;
      bool a : 1;
      bool p : 1;
      bool c : 1;
    };  // 6 fields
    word values;
  };

  template<typename T>
  void set_a(T dst, T src, T ret) {
    a = (dst ^ src ^ ret) & 0x10;
  }

  void set_szp(byte v) {
    z = (v == 0);
    s = (v & 0x80);
    p = !(__builtin_popcount(v) & 1);
  }

  void set_szp(word v) {
    z = (v == 0);
    s = (v & 0x8000);
    p = !(__builtin_popcount(static_cast<byte>(v)) & 1);
  }

};  // Flag

struct Segment {
  enum Id {
    kSegDefault       = -1,
    kSegCs,
    kSegSs,
    kSegDs,
    kSegEs,
    kSegFs,
    kSegGs,
    kSegMax
  };

  union {
    struct {
      word cs, ss, ds, es, fs, gs;
    };
    word reg_seg[kSegMax];
  };

  Id id;

  int get(Id default_segment = kSegDs) {
    if (id == kSegDefault) return reg_seg[default_segment];
    return reg_seg[id];
  }

  void set(Id id) {
    this->id = id;
  }

  void reset() {
    id = kSegDefault;
  }

  Segment() {
    reset();
  }
};  // Segment

struct Context {
  union GeneralRegister {
    struct {
      byte l;
      byte h;
    };
    byte v[2];
    word x;
  };

  union {
    struct {
      union {
        struct {
          GeneralRegister a, c, d, b;
        };
        GeneralRegister reg_gen[4];
      };

      union {
        struct {
          word sp, bp, si, di;
        };
        word reg_spc[4];
      };
    };
    word reg_all[8];
  };

  word ip;

  Segment seg;
  Flag flag;
};  // Context


class Cpu {
public:
  enum ExitStatus {
    kExitHalt,
    kExitInvalidOpcode,
  };

  Memory mem_;
  Context ctx_;

  void dump_status();
  ExitStatus run();

private:
  struct {
    bool repe:         1;
    bool repne:        1;
    bool lock:         1;
  } pfx_;

  byte& fetch() {
    byte &tmp = *mem_.get<byte>(ctx_.seg.cs, ctx_.ip);
    ctx_.ip += sizeof(byte);
    return tmp;
  }

  word& fetchw() {
    word &tmp = *mem_.get<word>(ctx_.seg.cs, ctx_.ip);
    ctx_.ip += sizeof(word);
    return tmp;
  }

  void decode_rm(byte b, void **rm);
  void decode_mod(byte b, void **mod);

  void opb_add(byte &dst, byte &src);
  void opw_add(word &dst, word &src);
  void opb_adc(byte &dst, byte &src);
  void opw_adc(word &dst, word &src);
  void opb_sub(byte &dst, byte &src);
  void opw_sub(word &dst, word &src);
  void opb_sbb(byte &dst, byte &src);
  void opw_sbb(word &dst, word &src);
  void opb_and(byte &dst, byte &src);
  void opw_and(word &dst, word &src);
  void opb_or(byte &dst, byte &src);
  void opw_or(word &dst, word &src);
  void opb_xor(byte &dst, byte &src);
  void opw_xor(word &dst, word &src);
  void opb_cmp(byte  dst, byte  src);
  void opw_cmp(word  dst, word  src);
  void opw_push(word data);
  void opw_pop(word &data);
  void opw_xchg(word &dst, word &src);
  template<typename D, typename S> void op_in(D &dst, S src);
  template<typename D, typename S> void op_out(D dst, S &src);

  void handle_interrupt(byte interrupt);
};
