/*
 * exercising the TIOCGSERIAL ioctl on arietta. at91sam9
 *
 * New ioctl() to set exotic baud rates and other things...
 *
 * (C) 2014 juewei@fabfolk.com
 */
#include <linux/serial.h>	// struct serial_struct
#include <asm-generic/ioctls.h>	// TCGETX
#include <sys/ioctl.h>		// ioctl()
#include <stdlib.h>		// atol()
#include <unistd.h>		// open()/close()
#include <fcntl.h>		// O_RDWR
#include <stdio.h>		// printf()
#include <errno.h>		// errno

int main(int ac, char **av)
{
  struct serial_struct s;
  int fd = open(av[1], O_RDWR);
  printf("%s -> fd=%d\n", av[1], fd);

  s.baud_base = 0;
  s.custom_divisor = 0;
  s.xmit_fifo_size = 0;

  errno = 0;
  int r = ioctl(fd, TCGETX, &s);
  printf("TCGETX=%d -> r=%d, errno=%d\n", TCGETX, r, errno);

  if (ac > 2)
    {
      s.custom_divisor = 0;
      s.baud_base = atol(av[2]);

      r = ioctl(fd, TCSETX, &s);
      printf("TCSETX=%d -> r=%d, errno=%d\n", TCSETX, r, errno);

      s.baud_base = 0;
      s.custom_divisor = 0;
      s.xmit_fifo_size = 0;

      r = ioctl(fd, TCGETX, &s);
      printf("TCGETX=%d -> r=%d, errno=%d\n", TCGETX, r, errno);
    }

  printf("baud_base=%d custom_divisor=%d xmit_fifo_size=%d\n", 
        s.baud_base, s.custom_divisor, s.xmit_fifo_size);
  printf("type(use_dma_tx)=%d flags(use_pdc_tx)=%d line=%d\n", 
        s.type, s.flags, s.line);

  // Attention: baud rate falls back to the previous stty settting, when the device is closed.
  close(fd);
  return 0;
}
