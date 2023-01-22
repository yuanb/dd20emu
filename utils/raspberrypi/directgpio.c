#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

//RPi GPIO Code Samples
// https://elinux.org/RPi_GPIO_Code_Samples

//Low Level Programming of the Raspberry Pi in C
//http://www.pieter-jan.com/node/15

#include <wiringPi.h>   //set priority

//dd20emu pin mapping
#define	_Y0		2	//BCM 2,  physical 3, yellow
#define	_Y1		17	//BCM 17, physical 11
#define	_Y2		27	//BCM 27, physical 13
#define	_Y3		22	//BVM 22, physical 15

#define	D0
#define	D1
#define D2
#define	D3
#define	D4
#define D5
#define D6
#define D7		19	//BCM 19, phy 35

#define	_WR		3	//BCM 3,  physical 5, green
#define	_RD		4	//BCM 4,  physical 7, blue
#define _Y3_RD      (1<<_Y3 | 1<<_RD)

#define	_EMUACK	26	//BCM 26, physical 37, orange

#define PHI0(n) (((n)>>0)&1)
#define PHI1(n) (((n)>>1)&1)
#define PHI2(n) (((n)>>2)&1)
#define PHI3(n) (((n)>>3)&1)

/***  Direct GPIO Access ***/
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

void setup_io();

void printButton(int g)
{
  if (GET_GPIO(g)) // !=0 <-> bit is 1 <- port is HIGH=3.3V
    printf("Button pressed!\n");
  else // port is LOW=0V
    printf("Button released!\n");
}

void dd20emu_setup(void) {
    // set /WR as input
    INP_GPIO(_WR);
    // set /RD as input
    INP_GPIO(_RD);
    // set /Y3 as input
    INP_GPIO(_Y3);

    // set _EMUACK as output
    INP_GPIO(_EMUACK);
    OUT_GPIO(_EMUACK);
    GPIO_SET = 1 << _EMUACK;

    //set D7 as output
    INP_GPIO(D7);
    OUT_GPIO(D7);
    GPIO_SET = 1 << D7;
}

int main(int argc, char **argv)
{
    uint8_t vtech1_fdc_status = 0;
    uint8_t vtech1_fdc_bits = 8;

    // Set up gpi pointer for direct register access
    setup_io();

    //piHiPri(99);
    dd20emu_setup();

    //volatile unsigned *gpio_read = gpio+13;

    while(1) {
        if (!GET_GPIO(_Y0) && !GET_GPIO(_WR)) {
            //Port #10H, write only
            //Bit 0 ~ 3, Step motor phase control (Active high)
            //Bit 4, Drive 1 enable (Active low)
            //Bit 5, Write data (Active high)
            //Bit 6, Write request (Active low)
            //Bit 7, Drive 2 enable (Active low)
            GPIO_CLR = 1 << _EMUACK;
            GPIO_SET = 1 << _EMUACK;
        }
        else if (!GET_GPIO(_Y1) && !GET_GPIO(_RD))
        {
            //Port #11H, read only, Bit 0 ~ 7 = Data byte read from disk
            if (vtech1_fdc_bits > 0) {
                if (vtech1_fdc_status & 0x80) {
                    vtech1_fdc_bits--;
                }
                if( vtech1_fdc_bits == 0 ) {
                    if( vtech1_fdc_status & 0x80 ) {
                        vtech1_fdc_bits = 8;
                    }
                    vtech1_fdc_status &= ~0x80;    
                }
            }          
        }
        else if (!GET_GPIO(_Y2) && !GET_GPIO(_RD))
        {
            //Port #12H, read only, Bit 7 = clock bit pulling input
            vtech1_fdc_status |= 0x80;
            GPIO_SET = 1 << D7;
            while(!GET_GPIO(_Y2));
            GPIO_CLR = 1 << D7;
        }
        else if (!GET_GPIO(_Y3) && !GET_GPIO(_RD)) {
            //Port #13H, read only, Bit 7 = 1 write protected
            GPIO_CLR = 1 << D7;
            while(!GET_GPIO(_Y3));
            GPIO_SET = 1 << D7;     
        }
    };
} // main


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