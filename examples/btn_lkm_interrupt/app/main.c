#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

struct input_event {
  struct timeval time;
  unsigned short type;
  unsigned short code;
  unsigned int value;
};

int main(int argc, char *argv[]) {
  char *input_dev = "/dev/input/event0";
  if (argc > 1) {
    input_dev = argv[1];
  }

  int fd;
  fd = open(input_dev, O_RDONLY);
  if (fd == -1) {
    perror("Unable to open");
    return EXIT_FAILURE;
  }

  while (1) {
    struct input_event event;
    ssize_t retval = read(fd, &event, sizeof(event));

    if (retval == -1) {
      perror("read()");
      return EXIT_FAILURE;
    } else if (retval != sizeof(event)) {
      perror("read()");
      fprintf(stderr, "Short read: got %zd (not %zd) bytes\n", retval,
              sizeof(event));
      return EXIT_FAILURE;
    }

    printf("input_event {\n");
    printf("  time:  %ld.%06ld\n", (long)event.time.tv_sec,
           (long)event.time.tv_usec);
    printf("  type:  0x%04hx (%hu)\n", event.type, event.type);
    printf("  code:  0x%04hx (%hu)\n", event.code, event.code);
    printf("  value: 0x%08x (%u)\n", event.value, event.value);
    printf("}\n");
  }

  close(fd);
  return EXIT_SUCCESS;
}
