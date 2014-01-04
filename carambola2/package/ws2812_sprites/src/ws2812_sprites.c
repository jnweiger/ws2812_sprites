/****************
* Helloworld.c
* The most simplistic C program ever written.
* An epileptic monkey on crack could write this code.
*****************/
#include <stdio.h>
#include "mmio.h"

struct mmio_options
{
	struct 		mmio		io;
};

int main(int argc, char **argv)
{
	struct mmio_options mo;

	unsigned long tbase = 15500000UL;
	if (argc > 1) tbase = strtoul(argv[1], NULL, 0);

#define GPIO_RANGE 0x30
#define GPIO_BASE 0x18040000
#define GPIO_OE   0x00000000
#define GPIO_IN   0x00000004
#define GPIO_OUT  0x00000008
#define GPIO_SET  0x0000000C
#define GPIO_CLR  0x00000010

// AR9331.pdf p65 
// GPIO0  green led
// GPIO11 user button
// GPIO12 reset button
// GPIO13 orange led eth1
// GPIO14 orange led eth0

	memset(&mo, 0, sizeof(mo));
	mo.io.range = 1;
	mmio_map(&mo.io, GPIO_BASE, GPIO_RANGE);

	fprintf(stderr, "Hello! world, see my blinkenlight? tbase=%ld\n", tbase);
	
	unsigned long i;
	int j;
	for (j = 0; j < 10; j++)
	  {
	    fprintf(stderr, "CLR %d\n", j);
	    for (i = tbase; i > 0; i--)
	      {
	        mmio_writel(&mo.io, GPIO_CLR, (1L<<14)); 
	        mmio_writel(&mo.io, GPIO_CLR, (1L<<13)); 
	      }
	    fprintf(stderr, "SET %d\n", j);
	    for (i = tbase; i > 0; i--)
	      {
	        mmio_writel(&mo.io, GPIO_SET, (1L<<14));
	        mmio_writel(&mo.io, GPIO_CLR, (1L<<13)); 
	      }
	  }
	fprintf(stderr, "Hello! Oh world, done.\n");

	mmio_unmap(&mo.io);
	return 0;
}
