#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#define PAGE_SIZE 4096UL
#define PAGE_MASK (PAGE_SIZE - 1)


struct ranges {
  volatile uint32_t  *regs;

  uint32_t            dbs_num;
  volatile uint8_t   *dbs;

  uint32_t            mem_size;
  volatile uint8_t   *mem;
};

int main(int argc, char **argv)
{
  int fd;
  void *map_base;
  struct ranges r;

  (void) argc;
  (void) argv;

  if ((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) {
    perror("open failed");
    return -1;
  }
  /* Map one page */
  map_base = mmap(0, 32 * 1024 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
      0xc0000000UL);
  if (map_base == (void *)-1) {
    perror("mmap failed");
    return -1;
  }

  r.regs = (volatile uint32_t *) map_base;
  r.dbs_num = r.regs[0];
  r.dbs = (volatile uint8_t *) map_base + r.regs[1];
  r.mem_size = r.regs[2];
  r.mem = (volatile uint8_t *) map_base + r.regs[3];

  printf("Regs = %p\n", r.regs);
  printf("  DoorbellsNum  = %d\n", r.dbs_num);
  printf("  Doorbells     = %p\n", r.dbs);
  printf("  MemSize       = %x\n", r.mem_size);
  printf("  Mem           = %p\n", r.mem);
  printf("Regs[PlAllocRx] = %x\n", r.regs[4]);
  printf("Regs[PlAllocDb] = %x\n", r.regs[5]);

  volatile uint64_t *db;
  printf("Writing Doorbell 0\n");
  db = (volatile uint64_t *) r.dbs;
  *db = 0x42424242;
  printf("Wrote Doorbell 0\n");

  uint32_t last_db = r.dbs_num - 1;
  printf("Writing Doorbell %d\n", last_db);
  db = (volatile uint64_t *) (r.dbs + last_db * 0x1000);
  *db = 0x43434343;
  printf("Wrote Doorbell %d\n", last_db);

  volatile uint32_t *mem_dw;
  volatile uint16_t *mem_w;
  mem_dw = (volatile uint32_t *) (r.mem);
  mem_w = (volatile uint16_t *) (r.mem + 2);
  printf("Writing memory 32-bit location 0x0 (%p)\n", mem_dw);
  *mem_dw = 0x12345678;
  printf("Reading memory 16-bit location 0x2 (%p)\n", mem_w);
  printf("  Result: %x\n", *mem_w);

  if (munmap(map_base, 32 * 1024 * 1024) == -1)
    return -1;
  close(fd);
  return 0;
}
