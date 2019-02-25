#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <stdbool.h>


//dd20emu pin mapping
#define	_Y0		2	//BCM 2,  physical 3, yellow
#define	_Y1		17	//BCM 17, physical 11
#define	_Y2		27	//BCM 27, physical 13
#define	_Y3		22	//BVM 22, physical 15

#define	_D0
#define	_D1
#define _D2
#define	_D3
#define	_D4
#define _D5
#define _D6
#define D7		19	//BCM 19, phy 35

#define	_WR		3	//BCM 3,  physical 5, green
#define	_RD		4	//BCM 4,  physical 7, blue
#define	_EMUACK	26	//BCM 26, physical 37, orange

// Access from ARM Running Linux

#define BCM2708_PERI_BASE        0x20000000
#define GPIO_BASE                (BCM2708_PERI_BASE + 0x200000) /* GPIO controller */

#define PAGE_SIZE (4*1024)
#define BLOCK_SIZE (4*1024)

int  mem_fd;
void *gpio_map;

// I/O access
volatile unsigned *gpio;

// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or SET_GPIO_ALT(x,y)
#define INP_GPIO(g) *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g) *(gpio+((g)/10)) |=  (1<<(((g)%10)*3))
#define SET_GPIO_ALT(g,a) *(gpio+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))

#define GPIO_SET *(gpio+7)  // sets   bits which are 1 ignores bits which are 0
#define GPIO_CLR *(gpio+10) // clears bits which are 1 ignores bits which are 0

#define GET_GPIO(g) (*(gpio+13)&(1<<g)) // 0 if LOW, (1<<g) if HIGH

#define GPIO_PULL *(gpio+37) // Pull up/pull down
#define GPIO_PULLCLK0 *(gpio+38) // Pull up/pull down clock

//
// Set up a memory regions to access GPIO
//
void setup_io()
{
   /* open /dev/mem */
   if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
      printf("can't open /dev/mem \n");
      exit(-1);
   }

   /* mmap GPIO */
   gpio_map = mmap(
      NULL,             //Any adddress in our space will do
      BLOCK_SIZE,       //Map length
      PROT_READ|PROT_WRITE,// Enable reading & writting to mapped memory
      MAP_SHARED,       //Shared with other processes
      mem_fd,           //File to map
      GPIO_BASE         //Offset to GPIO peripheral
   );

   close(mem_fd); //No need to keep mem_fd open after mmap

   if (gpio_map == MAP_FAILED) {
      printf("mmap error %d\n", (int)gpio_map);//errno also set!
      exit(-1);
   }

   // Always use volatile pointer!
   gpio = (volatile unsigned *)gpio_map;


} // setup_io

void dd20emu_setup()
{
	// /Y0 input
	// /Y1 input
	// /Y2 input
	// /Y3 input
	// /WR input
	// /RD input

	// /EMUACK output
	INP_GPIO(_EMUACK); // must use INP_GPIO before we can use OUT_GPIO
    OUT_GPIO(_EMUACK);	

	// D7 output
	INP_GPIO(D7);
	OUT_GPIO(D7);
}

void main()
{
	setup_io();

	dd20emu_setup();
	//bool _wr = HIGH;
	//bool _rd = HIGH;
	//uint8_t nPort = 0xff;

	INP_GPIO(4); // must use INP_GPIO before we can use OUT_GPIO
	OUT_GPIO(4);

	int a=0;

	while(1) {
  		GPIO_SET = 1<<4;
		for(int i=0; i<50;i++) {
			a = a+1;
		}
  		GPIO_CLR = 1<<4;
		for(int i=0; i<50; i++) {
			a = a-1;
		}
	}
}
