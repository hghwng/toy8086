#include "cpu.h"

template<typename T>
constexpr int sgnbit() {
  return 1 << (sizeof(T) * 8 - 1);
}

template<typename T>
inline void Cpu::op_add(T &dst, T &src) {
  T sum = dst + src;

  // Algorithm for setting the overflow flag:
  //    if dst and src have different signs (dst ^ src), won't overflow
  //    if dst and src have the same sign (dst ^ src ^ 0x80)
  //        if the result have different sign (sum ^ src)
  //            set OF = 1
  //
  // finally, only keep the sign bit (& 0x80) and ignore other bits
  ctx_.flag.o = (dst ^ src ^ sgnbit<T>()) & (sum ^ src) & sgnbit<T>();
  ctx_.flag.c = sum < src;
  ctx_.flag.set_a(dst, src, sum);
  ctx_.flag.set_szp(sum);
  dst = sum;
}

template<typename T>
inline void Cpu::op_adc(T &dst, T &src) {
  T sum = dst + src + ctx_.flag.c;
  ctx_.flag.o = (dst ^ src ^ sgnbit<T>()) & ((sum ^ src) & sgnbit<T>());
  ctx_.flag.c = (sum < src) | (sum == src && ctx_.flag.c);
  ctx_.flag.set_a(dst, src, sum);
  ctx_.flag.set_szp(sum);
  dst = sum;
}

template<typename T>
inline void Cpu::op_sub(T &dst, T &src) {
  T result = dst - src;
  ctx_.flag.c = result > dst;

  // Algorithm for setting the overflow flag (result = dst - src):
  //    if dst and src have the same sign, won't overflow
  //    if dst and src have different signs
  //        if result and dst have different sign
  //            set OF = 1
  ctx_.flag.o = (dst ^ src) & (result ^ dst) & sgnbit<T>();
  ctx_.flag.set_a(dst, src, result);
  ctx_.flag.set_szp(result);
  dst = result;
}

template<typename T>
inline void Cpu::op_sbb(T &dst, T &src) {
  T result = dst - src - ctx_.flag.c;
  ctx_.flag.c = (result > dst) | (ctx_.flag.c && result == dst);
  ctx_.flag.o = (dst ^ src) & (result ^ dst) & sgnbit<T>();
  ctx_.flag.set_a(dst, src, result);
  ctx_.flag.set_szp(result);
  dst = result;
}

template<typename T>
inline void Cpu::op_and(T &dst, T &src) {
  dst &= src;
  ctx_.flag.c = ctx_.flag.o = 0;
  ctx_.flag.set_szp(dst);
}

template<typename T>
inline void Cpu::op_or(T &dst, T &src) {
  dst |= src;
  ctx_.flag.c = ctx_.flag.o = 0;
  ctx_.flag.set_szp(dst);
}

template<typename T>
inline void Cpu::op_xor(T &dst, T &src) {
  dst ^= src;
  ctx_.flag.c = ctx_.flag.o = 0;
  ctx_.flag.set_szp(dst);
}

template<typename T>
inline void Cpu::op_cmp(T dst, T src) {
  op_sub(dst, src);
}

inline void Cpu::opw_push(word data) {
  ctx_.sp -= sizeof(word);
  *mem_.get<word>(ctx_.seg.get(Segment::kSegSs), ctx_.sp) = data;
}

inline void Cpu::opw_pop(word &data) {
  data = *mem_.get<word>(ctx_.seg.get(Segment::kSegSs), ctx_.sp);
  ctx_.sp += sizeof(word);
}

inline void Cpu::opw_xchg(word &dst, word &src) {
  word tmp = src;
  src = dst;
  dst = tmp;
}

template<typename D, typename S> void Cpu::op_in(D &dst, S src) {

};

template<typename D, typename S> void Cpu::op_out(D dst, S &src) {

};

void Cpu::dump_status() {
  fprintf(stderr, "AX = %04X CX = %04X DX = %04X BX = %04X\n",
          ctx_.a.x, ctx_.b.x, ctx_.c.x, ctx_.d.x);
  fprintf(stderr, "SP = %04X BP = %04X SI = %04X DI = %04X IP = %04X\n",
          ctx_.sp, ctx_.bp, ctx_.si, ctx_.di, ctx_.ip);
  fprintf(stderr, "CS = %04X SS = %04X DS = %04X ES = %04X FS = %04X GS = %04X\n",
          ctx_.seg.cs, ctx_.seg.ss, ctx_.seg.ds,
          ctx_.seg.es, ctx_.seg.fs, ctx_.seg.gs);
  fprintf(stderr, "OF = %d SF = %d ZF = %d AF = %d PF = %d CF = %d\n",
          ctx_.flag.o, ctx_.flag.s, ctx_.flag.z,
          ctx_.flag.a, ctx_.flag.p, ctx_.flag.c);
}

#define EXECUTE_OP(_fn)                                     \
  if (is_8bit) op_##_fn<byte>(to_byte(dst), to_byte(src));  \
  else         op_##_fn<word>(to_word(dst), to_word(src));  \
  break

Cpu::ExitStatus Cpu::run() {
  for (;;) {
    byte b = fetch();
    switch (b) {
      // Prefixes
      case 0xf0: pfx_.lock  = true; continue;
      case 0xf2: pfx_.repne = true; continue;
      case 0xf3: pfx_.repe  = true; continue;
      case 0x26: ctx_.seg.set(Segment::kSegEs); continue;
      case 0x36: ctx_.seg.set(Segment::kSegSs); continue;
      case 0x2e: ctx_.seg.set(Segment::kSegCs); continue;
      case 0x3e: ctx_.seg.set(Segment::kSegDs); continue;

      case 0x80: case 0x81: case 0x82: case 0x83: {   // group 1
        void *dst;
        byte modrm = fetch();
        decode_rm(modrm, &dst);

        // 80 82 -> Eb Ib
        // 81    -> Ev Iv
        // 83    -> Ev Ib
        int is_8bit;
        word src;
        switch (b) {
          case 0x80:
          case 0x82:
            src = fetch();
            is_8bit = true;
            break;

          case 0x81:
            src = fetchw();
            is_8bit = false;
            break;

          case 0x83:
            src = fetch();
            is_8bit = false;
            break;
        }

        switch ((modrm >> 3) & 7) {
          case 0: EXECUTE_OP(add);
          case 1: EXECUTE_OP(or);
          case 2: EXECUTE_OP(adc);
          case 3: EXECUTE_OP(sbb);
          case 4: EXECUTE_OP(and);
          case 5: EXECUTE_OP(sub);
          case 6: EXECUTE_OP(xor);
          case 7: EXECUTE_OP(cmp);
          default:  // won't reach here
            goto invalid_instr;
        }
        goto next_instr;
      }   // 0x80 to 0x83, group 1

      case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05:  // add
      case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15:  // adc
      case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25:  // and
      case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35:  // xor
      case 0x08: case 0x09: case 0x0a: case 0x0b: case 0x0c: case 0x0d:  // or
      case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c: case 0x1d:  // sbb
      case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d:  // sub
      case 0x38: case 0x39: case 0x3a: case 0x3b: case 0x3c: case 0x3d:  // cmp
      case 0x88: case 0x89: case 0x8a: case 0x8b: {                      // mov
        void *src, *dst;
        bool is_8bit = false;
        byte tmp;
        switch (b & 7) {
          case 0:   // Eb Gb
            is_8bit = true;
          case 1:   // Ev Gv
            tmp = fetch();
            decode_rm(tmp, &dst);
            decode_mod(tmp, &src);
            break;

          case 2:   // Gb Eb
            is_8bit = true;
          case 3:   // Gv Ev
            tmp = fetch();
            decode_rm(tmp, &src);
            decode_mod(tmp, &dst);
            break;

          case 4:   // AL Ib
            is_8bit = true;
          case 5:   // AX Iv
            dst = &ctx_.a.l;
            src = is_8bit ? to_ptr(fetch()) : to_ptr(fetchw());
            break;

          default:
            goto invalid_instr;
        }

        switch (b & 0xf8) {
          case 0x00: EXECUTE_OP(add);
          case 0x10: EXECUTE_OP(adc);
          case 0x20: EXECUTE_OP(and);
          case 0x30: EXECUTE_OP(xor);
          case 0x08: EXECUTE_OP(or);
          case 0x18: EXECUTE_OP(sbb);
          case 0x28: EXECUTE_OP(sub);
          case 0x38: EXECUTE_OP(cmp);
          case 0x88:   // mov (partial)
            if (is_8bit) to_byte(dst) = to_byte(src);
            else         to_word(dst) = to_word(src);
            break;

          default:
            goto invalid_instr;
        }
        goto next_instr;
      } //  case of some opcode from 0x00 to 0x3d

      case 0x40: case 0x41: case 0x42: case 0x43:   // inc
      case 0x44: case 0x45: case 0x46: case 0x47:
      case 0x48: case 0x49: case 0x4a: case 0x4b:   // dec
      case 0x4c: case 0x4d: case 0x4e: case 0x4f:
      case 0x50: case 0x51: case 0x52: case 0x53:   // push
      case 0x54: case 0x55: case 0x56: case 0x57:
      case 0x58: case 0x59: case 0x5a: case 0x5b:   // pop
      case 0x5c: case 0x5d: case 0x5e: case 0x5f:
      /*90=NOP*/ case 0x91: case 0x92: case 0x93:   // xchg
      case 0x94: case 0x95: case 0x96: case 0x97:
      case 0xb8: case 0xb9: case 0xba: case 0xbb:   // mov
      case 0xbc: case 0xbd: case 0xbe: case 0xbf: {
        word &reg = ctx_.reg_all[b & 7];
        switch (b & 0xf8) {
          case 0x40: {   // inc
            word v = 1;
            op_add<word>(reg, v);
            break;
          }

          case 0x48: {   // dec
            word v = 1;
            op_sub<word>(reg, v);
            break;
          }

          case 0x50:    // push
            opw_push(reg);
            break;

          case 0x58:    // pop
            opw_pop(reg);
            break;

          case 0x90:    // xchg
            opw_xchg(reg, ctx_.a.x);
            break;

          case 0xb8:    // mov
            reg = fetchw();
            break;

          default:
            goto invalid_instr;
        }
        goto next_instr;
      }   // 16-bit register operations (inc, dec, push, pop, mov)

      case 0xb0: case 0xb1: case 0xb2: case 0xb3: // mov [reg], ib
      case 0xb4: case 0xb5: case 0xb6: case 0xb7: {
        byte &reg = ctx_.reg_gen[b & 3].v[(b & 4) / 4];
        reg = fetch();
        goto next_instr;
      }   // 8-bit mov-to-register operations

      case 0x70: case 0x71: case 0x72: case 0x73:
      case 0x74: case 0x75: case 0x76: case 0x77:
      case 0x78: case 0x79: case 0x7a: case 0x7b:
      case 0x7c: case 0x7d: case 0x7e: case 0x7f: {
        char offset = fetch();  // caution: signed offset
        bool condition_met;
        switch (b & 0xf) {
          case 0x0:   // jo
            condition_met = ctx_.flag.o;
            break;
          case 0x1:   // jno
            condition_met = !ctx_.flag.o;
            break;
          case 0x2:   // jb
            condition_met = ctx_.flag.c;
            break;
          case 0x3:   // jae
            condition_met = !ctx_.flag.c;
            break;
          case 0x4:   // je
            condition_met = ctx_.flag.z;
            break;
          case 0x5:   // jne
            condition_met = !ctx_.flag.z;
            break;
          case 0x6:   // jbe
            condition_met = ctx_.flag.c || ctx_.flag.z;
            break;
          case 0x7:   // ja
            condition_met = !(ctx_.flag.c || ctx_.flag.z);
            break;
          case 0x8:   // js
            condition_met = ctx_.flag.s;
            break;
          case 0x9:   // jns
            condition_met = !ctx_.flag.s;
            break;
          case 0xa:   // jp
            condition_met = ctx_.flag.p;
            break;
          case 0xb:   // jnp
            condition_met = !ctx_.flag.p;
            break;
          case 0xc:   // jl
            condition_met = ctx_.flag.s != ctx_.flag.o;
            break;
          case 0xd:   // jnl
            condition_met = ctx_.flag.s == ctx_.flag.o;
            break;
          case 0xe:   // jle
            condition_met = ctx_.flag.s != ctx_.flag.o || ctx_.flag.z;
            break;
          case 0xf:   // jg
            condition_met = ctx_.flag.s == ctx_.flag.o && !ctx_.flag.z;
            break;
        }

        if (!condition_met) goto next_instr;
        ctx_.ip += offset;
        goto next_instr;
      }   // jcc: jump when condition is met

      case 0x90:    // nop
        goto next_instr;

      case 0xc2:    // ret Iw
        ctx_.sp += fetchw();
      case 0xc3:    // ret
        opw_pop(ctx_.ip);
        goto next_instr;
      case 0xca:    // ret FAR Iw
        ctx_.sp += fetchw();
      case 0xcb:    // ret FAR
        opw_pop(ctx_.ip);
        opw_pop(ctx_.seg.cs);
        goto next_instr;

      case 0xcc:    // int 3
        handle_interrupt(3);
        goto next_instr;
      case 0xcd:    // int
        handle_interrupt(fetch());
        goto next_instr;

      case 0xe4:    // in AL, Ib
        op_in(this->ctx_.a.l, fetch());
        goto next_instr;
      case 0xe5:    // in AX, Ib
        op_in(this->ctx_.a.x, fetch());
        goto next_instr;
      case 0xe6:    // out AL, Ib
        op_out(fetch(), this->ctx_.a.l);
        goto next_instr;
      case 0xe7:    // out AX, Ib
        op_out(fetch(), this->ctx_.a.x);
        goto next_instr;
      case 0xec:    // in AL, DX
        op_in(this->ctx_.a.l, this->ctx_.d.x);
        goto next_instr;
      case 0xed:    // in AX, DX
        op_in(this->ctx_.a.x, this->ctx_.d.x);
        goto next_instr;
      case 0xee:    // out DX, AL
        op_out(this->ctx_.d.x, this->ctx_.a.l);
        goto next_instr;
      case 0xef:    // out DX, AX
        op_out(this->ctx_.d.x, this->ctx_.a.x);
        goto next_instr;

      case 0xe8:    // call Jv
        opw_push(ctx_.ip);
      case 0xe9:    // jmp Jv
        ctx_.ip += fetchw();
        goto next_instr;
      case 0x9a:    // call Ap
        opw_push(ctx_.seg.cs);
        opw_push(ctx_.ip);
      case 0xea:    // jmp Ap
        ctx_.seg.cs = fetchw();
        ctx_.ip = fetchw();
        goto next_instr;
      case 0xeb:    // jmp Jb
        ctx_.ip += fetch();
        goto next_instr;

      case 0xf4:    // hlt
        return kExitHalt;

invalid_instr:
      default:
        fprintf(stderr, "Invalid opcode: %x\n", b);
        dump_status();
        return kExitInvalidOpcode;
    }   // end of opcode switch

next_instr:
    pfx_.repe = pfx_.repne = pfx_.lock = 0;
    ctx_.seg.reset();
    continue;
  }   // end of fetch opcode loop
}

void Cpu::decode_rm(byte b, void **rm) {
  byte modbits = (b >> 6) & 3;
  byte rmbits = b & 7;

  if (modbits == 3) {
    *rm = &ctx_.reg_gen[rmbits & 3].v[rmbits >> 2];
    return;
  }

  uint16_t base;
  switch (rmbits) {
    case 0: base = ctx_.b.x + ctx_.si;  break;
    case 1: base = ctx_.b.x + ctx_.di;  break;
    case 2: base = ctx_.bp + ctx_.si;   break;
    case 3: base = ctx_.bp + ctx_.di;   break;
    case 4: base = ctx_.si;             break;
    case 5: base = ctx_.di;             break;
    case 6:
      if (modbits == 0) {
        base = 0;
        modbits = 2;    // 01 110 xxx: EA = disp16
      } else {
        base = ctx_.bp;
      }
      break;
    case 7:
      base = ctx_.b.x;
      break;
  }

  if (modbits == 1) {
    *rm = mem_.get<void>(ctx_.seg.get(), base + fetch());
  } else if (modbits == 2) {
    *rm = mem_.get<void>(ctx_.seg.get(), base + fetchw());
  }
}

void Cpu::decode_mod(byte b, void **reg) {
  byte regbits = (b >> 3) & 7;
  *reg = &ctx_.reg_gen[regbits & 3].v[regbits >> 2];
}

void Cpu::handle_interrupt(byte interrupt) {
  switch (interrupt) {
    case 0x21: {  // DOS interrupt
      switch (ctx_.a.h) {
        case 0x01:  // get char from stdin
          ctx_.a.l = (char)getonechar();
          break;
        case 0x02:  // print char to stdout
          printf("%c", ctx_.d.l);
          ctx_.a.l = ctx_.d.l;  // side effect
          break;
      }
    }
  }
}
