#include "cpu.h"

inline void Cpu::opb_add(byte &dst, byte &src) {
  byte sum = dst + src;
  ctx_.flag.o = (dst ^ src ^ 0x80) & ((sum ^ src) & 0x80);
  ctx_.flag.c = sum < src;
  ctx_.flag.a = ((dst ^ src ^ sum) & 0x10) == 0x10;
  ctx_.flag.set_szp(sum);
  dst = sum;
}

inline void Cpu::opw_add(word &dst, word &src) {
  word sum = dst + src;
  ctx_.flag.o = (dst ^ src ^ 0x8000) & ((sum ^ src) & 0x8000);
  ctx_.flag.c = sum < src;
  ctx_.flag.a = ((dst ^ src ^ sum) & 0x10) == 0x10;
  ctx_.flag.set_szp(sum);
  dst = sum;
}

void Cpu::dump_status() {
  fprintf(stderr, "AX = %04X CX = %04X DX = %04X BX = %04X\n",
          ctx_.a.x, ctx_.b.x, ctx_.c.x, ctx_.d.x);
  fprintf(stderr, "BP = %04X SP = %04X SI = %04X DI = %04X IP = %04X\n",
          ctx_.bp, ctx_.sp, ctx_.si, ctx_.di, ctx_.ip);
  fprintf(stderr, "CS = %04X SS = %04X DS = %04X ES = %04X FS = %04X GS = %04X\n",
          ctx_.seg.cs, ctx_.seg.ss, ctx_.seg.ds,
          ctx_.seg.es, ctx_.seg.fs, ctx_.seg.gs);
  fprintf(stderr, "OF = %d SF = %d ZF = %d AF = %d PF = %d CF = %d\n",
          ctx_.flag.o, ctx_.flag.s, ctx_.flag.z,
          ctx_.flag.a, ctx_.flag.p, ctx_.flag.c);
}

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

      case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05:   // add
      case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15:   // adc
      case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25:   // and
      case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35:   // xor
      case 0x08: case 0x09: case 0x0a: case 0x0b: case 0x0c: case 0x0d:   // or
      case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c: case 0x1d:   // sbb
      case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d:   // sub
      case 0x38: case 0x39: case 0x3a: case 0x3b: case 0x3c: case 0x3d: { // cmp
        void *src, *dst;
        bool is_8bit = false;
        switch (b & 7) {
          case 0:   // Eb Gb
            is_8bit = true;
          case 1:   // Ev Gv
            decode_modrm(&dst, &src);
            break;

          case 2:   // Gb Eb
            is_8bit = true;
          case 3:   // Gv Ev
            decode_modrm(&src, &dst);
            break;

          case 4:   // AL Iv
            is_8bit = true;
          case 5:   // AX Iv
            dst = &ctx_.a.l;
            src = is_8bit ? to_ptr(fetch()) : to_ptr(fetchw());
            break;
        }

        switch (b >> 3) {
          // add
          case 0:
            if (is_8bit) {
              opb_add(to_byte(dst), to_byte(src));
            } else {
              opw_add(to_word(dst), to_word(src));
            }

          // adc
          case 1:

          // and
          case 2:

          // xor
          case 3:

          // or
          case 4:

          // sbb
          case 5:

          // sub
          case 6:

          // cmp
          case 7:
            break;
        }
        goto next_instr;
      } //  case of some opcode from 0x00 to 0x3d

      case 0xf4:
        return kExitHalt;

      default:
        fprintf(stderr, "Invalid opcode: %x\n", b);
        dump_status();
        return kExitInvalidOpcode;

next_instr:
    pfx_.repe = pfx_.repne = pfx_.lock = 0;
    ctx_.seg.reset();
    continue;
    }
  }
}

void Cpu::decode_modrm(void **rm, void **reg) {
  byte b = fetch();
  byte modbits = (b >> 6) & 3;
  byte regbits = (b >> 3) & 7;
  byte rmbits = b & 7;

  *reg = &ctx_.reg_gen[regbits >> 1].v[regbits & 1];

  if (modbits == 3) {
    *rm = &ctx_.reg_gen[rmbits >> 1].v[rmbits & 1];
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
