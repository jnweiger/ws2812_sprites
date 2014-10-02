/*
 * exercising the TIOCGSERIAL ioctl on arietta. at91sam9
 *
 * New ioctl() to set exotic baud rates and other things...
 *
 * (C) 2014 juewei@fabfolk.com
 */
#include <linux/serial.h>
#include <asm-generic/ioctls.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

int main(int ac, char **av)
{
  struct serial_struct s;
  int fd = open(av[1], O_RDWR);
  printf("%s -> fd=%d\n", av[1], fd);

  s.baud_base = 0;
  s.custom_divisor = 0;
  s.xmit_fifo_size = 0;

  errno = 0;
#if 1
  int r = ioctl(fd, TIOCGSERIAL, &s);	
  // oh, TIOCGSERIAL is already implemented somewhere.
  // baud_base=8333333 custom_divisor=0 xmit_fifo_size=1
#endif
#if 0
  int r = ioctl(fd, TCGETX, &s);
  printf("TCGETX=%d -> r=%d, errno=%d\n", r, TCGETX, errno);
#endif
  printf("baud_base=%d custom_divisor=%d xmit_fifo_size=%d\n", 
        s.baud_base, s.custom_divisor, s.xmit_fifo_size);

  close(fd);
}
