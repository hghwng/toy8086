#include "cpu.h"
#include <stdio.h>

struct MzHeader {
  word magic;
  word block_remain;
  word block_num;
  word reloc_num;
  word hdr_size;
  word alloc_min;
  word alloc_max;

  word ss;
  word sp;
  word cksum;
  word ip;
  word cs;

  word reloc_offset;
  word overlay_num;
};

bool load_binary(const char *path, Cpu &cpu) {
  FILE *f = fopen(path, "rb");
  if (!f) return false;

  fseek(f, 0, SEEK_END);
  size_t size = ftell(f);
  fseek(f, 0, SEEK_SET);

  MzHeader hdr;
  fread(&hdr, 1, sizeof(MzHeader), f);

  if (size < sizeof(MzHeader) || hdr.magic != 0x5a4d) {
    // load as COM
    rewind(f);

    cpu.ctx_.seg.cs = cpu.ctx_.seg.ds =
      cpu.ctx_.seg.es = cpu.ctx_.seg.ss = 0x700;
    cpu.ctx_.sp = 0xfffe;
    cpu.ctx_.ip = 0x100;

    void *mem_ptr = cpu.mem_.get<void>(cpu.ctx_.seg.cs, cpu.ctx_.ip);
    fread(mem_ptr, 1, 0x10000, f);
    fclose(f);
    return true;
  }

  constexpr int kParaSize = 16;
  constexpr int kBlockSize = 512;

  size_t img_size = (hdr.block_num - 1) * kBlockSize;
  img_size += hdr.block_remain == 0 ? kBlockSize : hdr.block_remain;
  img_size -= hdr.hdr_size * kParaSize;
  fseek(f, hdr.hdr_size * kParaSize, SEEK_SET);

  void *mem_ptr = cpu.mem_.get<void>(0, 0);
  fread(mem_ptr, 1, img_size, f);

  cpu.ctx_.sp = hdr.sp;
  cpu.ctx_.ip = hdr.ip;
  cpu.ctx_.seg.ss = hdr.ss;
  cpu.ctx_.seg.cs = hdr.cs;
  cpu.ctx_.seg.ds = hdr.cs + (img_size >> 4) + 1;
  return true;
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s [FILE]\n", argv[0]);
    return 1;
  }

  Cpu cpu;
  if (!load_binary(argv[1], cpu)) {
    fprintf(stderr, "Failed to load file %s.\n", argv[1]);
    return 2;
  }

  auto st = cpu.run();
  switch (st) {
    case Cpu::kExitHalt:
      printf("Program exited normally.\n");
      break;
    case Cpu::kExitInvalidOpcode:
      printf("Program exited because of invaild opcode.\n");
      break;
    case Cpu::kExitDebugInterrupt:
      printf("Program exited because of debug interrupt.\n");
      break;
    case Cpu::kExitInvalidInstruction:
      printf("Program exited because of invaild instruction.\n");
      break;
    case Cpu::kContinue:
      break;
  }

  return 0;
}
