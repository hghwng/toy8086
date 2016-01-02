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
    kSegEs,
    kSegCs,
    kSegSs,
    kSegDs,
    kSegFs,
    kSegGs,
    kSegMax
  };

  union {
    struct {
      word es, cs, ss, ds, fs, gs;
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
    memset(reg_seg, 0, sizeof(reg_seg));
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
    kExitDebugInterrupt,
    kExitInvalidOpcode,
    kExitInvalidInstruction,
    kContinue
  };

  Memory mem_;
  Context ctx_;

  void dump_status();
  ExitStatus run();

  Cpu() {
    memset(&ctx_.reg_all, 0, sizeof(ctx_.reg_all));
  }

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

  void *decode_rm(byte b, bool is_8bit = true);
  void *decode_reg(byte b, bool is_8bit = true);

  template<typename T> void op_add(T &dst, T &src);
  template<typename T> void op_adc(T &dst, T &src);
  template<typename T> void op_sub(T &dst, T &src);
  template<typename T> void op_sbb(T &dst, T &src);
  template<typename T> void op_and(T &dst, T &src);
  template<typename T> void op_or(T &dst, T &src);
  template<typename T> void op_xor(T &dst, T &src);
  template<typename T> void op_cmp(T dst, T src);
  template<typename T> void op_test(T dst, T src);

  template<typename T> void op_not(T &dst);
  template<typename T> void op_neg(T &dst);
  inline void op_mul(byte imm);
  inline void op_mul(word imm);
  inline void op_imul(byte imm);
  inline void op_imul(word imm);
  inline void op_div(byte imm);
  inline void op_div(word imm);
  inline void op_idiv(byte imm);
  inline void op_idiv(word imm);

  template<typename T> void op_rol(T &dst, T src);
  template<typename T> void op_ror(T &dst, T src);
  template<typename T> void op_rcl(T &dst, T src);
  template<typename T> void op_rcr(T &dst, T src);
  template<typename T> void op_shl(T &dst, T src);
  template<typename T> void op_shr(T &dst, T src);
  template<typename T> void op_sar(T &dst, T src);

  void op_push(word data);
  void op_pop(word &data);
  void op_xchg(word &dst, word &src);
  template<typename D, typename S> void op_in(D &dst, S src);
  template<typename D, typename S> void op_out(D dst, S &src);

  Cpu::ExitStatus handle_interrupt(byte interrupt);
};
