#include "cpu.h"
#include <stdio.h>

int main(int argc, char **argv) {
  Cpu cpu;
  cpu.ctx_.seg.cs = 0;
  cpu.ctx_.seg.ss = 0x3000;
  cpu.ctx_.ip = 0x100;
  void *mem_ptr = cpu.mem_.get<void>(cpu.ctx_.seg.cs, cpu.ctx_.ip);

  FILE *f = fopen("/home/hugh/tmp/hi-world.com", "rb");
  fread(mem_ptr, 1, 0x10000, f);
  fclose(f);

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
  }

  return 0;
}
