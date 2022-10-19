#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <linux/input.h>
#include <sys/mman.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>

#define RED_MASK 0xF800
#define GREEN_MASK 0x7E0
#define BLUE_MASK 0x1F
#define WHITE 0xFFFF
#define DIM 8 

#define UP 105
#define DOWN 106
#define LEFT 103
#define RIGHT 108

struct input_event ev;

void fill(short *fb, short len, short value) {
  for (int i = 0; i < len; i++) {
    fb[i] = value;
  }
}

int positive_modulo(int x, int N) {
  return (x % N + N) %N;
}

int main() {
  int fd = open("/dev/fb0", O_RDWR);
  int fd2 = open("/dev/input/event0", O_RDONLY);
  short masks[] = {RED_MASK, GREEN_MASK, BLUE_MASK};
  if (!fd) {
    printf("could not open fb0");
    exit(1);
  }
  if (!fd2) {
    printf("could not open event0");
    exit(1);
  }
  printf("Successfully opened fb0\n");

  short *fb = (short*) mmap(0, 8 * 8 * sizeof(short), PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
  memset(fb, 0, 8 * 8 * 2);
  close(fd);

  int column = 4;
  int row = 4;
  int i = 0;
  while(1) {
    read(fd2, &ev, sizeof(struct input_event));
    if (ev.value != 0) {
      printf("code is: %d", ev.code);
      fb[column * 8 + row] = 0;
      switch (ev.code) {
        case LEFT:
          column = positive_modulo(column - 1, 8);
          break;
        case UP:
          row = positive_modulo(row - 1, 8);
          break;
        case DOWN:
          row = positive_modulo(row + 1, 8);
          break;
        case RIGHT:
          column = positive_modulo(column + 1, 8);
          break;
      }
      printf("column: %d, row: %d", column, row);
      printf("index: %d\n", column * 8 + row);
      fb[column * 8 + row] = WHITE;
    }
  }
}


