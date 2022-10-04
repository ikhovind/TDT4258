#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

int main() {
  int fd = open("/dev/fb0", O_RDWR);
  if (!fd) {
    printf("could not open fb0");
    exit(1);
  }
  printf("Successfully opened fb0\n");

  short *fb = (short*) mmap(0, 8 * 8 * sizeof(short), PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
  memset(fb, 0, 8 * 8 * 2);
  close(fd);
  int i = 0;
  while(i < 64) {
    fb[i] = USHRT_MAX;
    usleep(20000);
    i++;
  }
  printf("loop finished\n");
}


