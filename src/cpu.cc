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

inline void Cpu::opb_sub(byte &dst, byte &src) {
  byte result = dst - src;
  ctx_.flag.c = result > dst;
  //ctx_.flag.o =
  ctx_.flag.set_szp(result);
  dst = result;
}

inline void Cpu::opw_sub(word &dst, word &src) {
  word result = dst - src;
  ctx_.flag.c = result > dst;
  //ctx_.flag.o =
  ctx_.flag.set_szp(result);
  dst = result;
}

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
      case 0x38: case 0x39: case 0x3a: case 0x3b: case 0x3c: case 0x3d:   // cmp
      case 0x88: case 0x89: case 0x8a: case 0x8b: { // mov (partial)
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

          default:
            fprintf(stderr, "Invalid instruction %x", b);
        }

        switch (b >> 3) {
          // add
          case 0:
            if (is_8bit) {
              opb_add(to_byte(dst), to_byte(src));
            } else {
              opw_add(to_word(dst), to_word(src));
            }
            break;
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
            if (is_8bit) {
              opb_sub(to_byte(dst), to_byte(src));
            } else {
              opw_sub(to_word(dst), to_word(src));
            }
            break;
          // cmp
          case 7:
            break;

          // mov (partial)
          case 17:
            if (is_8bit) {
              to_byte(dst) = to_byte(src);
            } else {
              to_word(dst) = to_word(src);
            }
            break;

          default:
            fprintf(stderr, "Invalid instruction %x", b);
        }
        goto next_instr;
      } //  case of some opcode from 0x00 to 0x3d

      case 0x90: // NOP
        continue;

      case 0x40: case 0x41: case 0x42: case 0x43: // INC
      case 0x44: case 0x45: case 0x46: case 0x47:
      case 0x48: case 0x49: case 0x4a: case 0x4b: // DEC
      case 0x4c: case 0x4d: case 0x4e: case 0x4f:
      case 0x50: case 0x51: case 0x52: case 0x53: // PUSH
      case 0x54: case 0x55: case 0x56: case 0x57:
      case 0x58: case 0x59: case 0x5a: case 0x5b: // POP
      case 0x5c: case 0x5d: case 0x5e: case 0x5f:
      /*90=NOP*/ case 0x91: case 0x92: case 0x93: // XCHG
      case 0x94: case 0x95: case 0x96: case 0x97:
      case 0xb8: case 0xb9: case 0xba: case 0xbb: // MOV
      case 0xbc: case 0xbd: case 0xbe: case 0xbf: {
        word &reg = this->ctx_.reg_all[b & 7];
        switch (b & 0xf8) {
          case 0x40: // INC
            fprintf(stderr, "INC [reg] not implemented.");
            break;
          case 0x48: // DEC
            fprintf(stderr, "DEC [reg] not implemented.");
            break;
          case 0x50: // PUSH
            fprintf(stderr, "PUSH [reg] not implemented.");
            break;
          case 0x58: // POP
            fprintf(stderr, "POP [reg] not implemented.");
            break;
          case 0x90: // XCHG
            fprintf(stderr, "XCHG [reg], AX not implemented.");
            break;
          case 0xb8: { // MOV
            reg = fetchw();
            break;
          }
        }
        goto next_instr;
      } // 16-bit register operations (INC, DEC, PUSH, POP, MOV)

      case 0xb0: case 0xb1: case 0xb2: case 0xb3: // MOV [reg], Ib
      case 0xb4: case 0xb5: case 0xb6: case 0xb7: {
        byte &reg = this->ctx_.reg_gen[b & 3].v[(b & 4) / 4];
        reg = fetch();
        goto next_instr;
      } // 8-bit MOV-to-register operations

      case 0xcc:
        handle_interrupt(3);
        continue;

      case 0xcd:
        handle_interrupt(fetch());
        continue;

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

  *reg = &this->ctx_.reg_gen[regbits & 3].v[regbits >> 2];

  if (modbits == 3) {
    *rm = &this->ctx_.reg_gen[rmbits & 3].v[rmbits >> 2];
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

void Cpu::handle_interrupt(byte interrupt) {
  switch (interrupt) {
    case 0x21: {  // DOS interrupt
      switch (this->ctx_.a.h) {
        case 0x01:  // get char from stdin
          this->ctx_.a.l = (char) getonechar();
          break;
        case 0x02:  // print char to stdout
          printf("%c", this->ctx_.d.l);
          this->ctx_.a.l = this->ctx_.d.l;  // side effect
          break;
      }
    }
  }
}
