#include "cpu.h"
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char **argv) {
  Cpu cpu;
  cpu.ctx_.seg.cs = 0;
  cpu.ctx_.seg.ss = 0x3000;
  cpu.ctx_.ip = 0x100;
  void *mem_ptr = cpu.mem_.get<void>(cpu.ctx_.seg.cs, 0);

  int fd = open("../test/com", O_RDONLY);
  read(fd, mem_ptr, 0x10000);
  close(fd);

  auto st = cpu.run();
  switch (st) {
    case Cpu::kExitHalt:
      printf("Program exited normally.\n");
      break;
    case Cpu::kExitInvalidOpcode:
      printf("Program exited because of invaild opcode.\n");
      break;
  }

  return 0;
}
