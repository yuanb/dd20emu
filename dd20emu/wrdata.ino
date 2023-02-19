

#include "wrdata.h"

/* Port and bit for WRDATA ICP5*/
/* Could also use ICP4 (PL0), which is defined as digital pin 49*/
//#define PORT_WRDATA PINL   //PL1 used in wrdata.ino
//#define WR_DATA_BIT 1
//const byte wrDataPin = 48;

#define TIFR    TIFR5   // timer 5 flag register
#define TOV     TOV5    // overflow flag
#define OCF     OCF5A   // output compare flag
#define ICF     ICF5    // input capture flag
#define TCCRA   TCCR5A  // timer 5 control register A
//#define COMA1   COM5A1  // timer 5 output compare mode bit 1
//#define COMA0   COM5A0  // timer 5 output compare mode bit 0
#define TCCRB   TCCR5B  // timer 5 control register B
#define CS2     CS52    // timer 5 clock select bit 2
#define CS1     CS51    // timer 5 clock select bit 1
#define CS0     CS50    // timer 5 clock select bit 0
//#define WGM2    WGM52   // timer 5 waveform mode bit 2
//#define TCCRC   TCCR5C  // timer 5 control register C
//#define FOC     FOC5A   // force output compare flag
//#define OCR     OCR5A   // timer 5 output compare register
#define TCNT    TCNT5   // timer 5 counter
#define TIMSK   TIMSK5
#define ICNC    ICNC5   //noise canceler
#define ICES    ICES5   //input capture edge select: 1-rising, 0-falling

unsigned int wr_buf[WRBUF_SIZE] = {0};

void initInputCaptureTimer()
{
  noInterrupts();
  
  //Mega2560 Timer 5 for ICP5
  TCCRA = 0;
  TCCRB = 0;

  //TIFR = bit(ICF) | bit(TOV);

  //TCCRB = bit(ICNC) | /*bit(ICES)*/ | bit (CS1);
  //0b11000010 - 1/8 prescaler (010) rising edge noise canceler
  //0b11000001 - 1/1 prescaler (001) rising edge noise canceler
  //0b10000001 - 1/1 prescaler (001) falling edge noise canceler
  TCCRB = 0b10000001;
  
  TCNT = 0;
  
  //TIMSK = bit (ICIE5)
  //TIMSK |= 0b00100000;    // enable input capture interrupt
  
  interrupts();
}


//Time Measurement via Arduino Timer
//https://www.youtube.com/watch?v=94S3onAUJm8&list=PLUKuIgyN0JeEO527wz_iOWDnu88-gWTaX&index=130&t=8s&ab_channel=AnasKuzechie

//Input Capture Interrupt in Arduino
//https://www.youtube.com/watch?v=N5VQZTDNcj0&list=PLUKuIgyN0JeEO527wz_iOWDnu88-gWTaX&index=117&ab_channel=AnasKuzechie

//Time Measurement via Arduino Timer
//https://www.youtube.com/watch?v=94S3onAUJm8&list=PLUKuIgyN0JeEO527wz_iOWDnu88-gWTaX&index=129&ab_channel=AnasKuzechie

//Time Measurement via Arduino Timer (part 2)
//https://www.youtube.com/watch?v=2iKfdpwK2Pg&list=PLUKuIgyN0JeEO527wz_iOWDnu88-gWTaX&index=131&t=1s&ab_channel=AnasKuzechie



//https://forum.arduino.cc/t/pwm-input-capture-interrupt/544530/3

volatile unsigned int PulseTime = 0;
static uint16_t buf_idx = 0;

ISR(TIMER5_CAPT_vect)
{
  static unsigned RisingEdgeTime = 0;
  static unsigned FallingEdgeTime = 0;
  
  // Which edge is armed?
  if (TCCRB & (1 << ICES5))
  {
    //set bit
    PORT_ICPFOLLOWER |= (1<< FOLLOWER_BIT);

    
    // Rising Edge
    RisingEdgeTime = ICR5;
    TCCRB &= ~(1 << ICES5); // Switch to Falling Edge
    //PulseTime = RisingEdgeTime - FallingEdgeTime;

    //if (buf_idx < sizeof(wr_buf));
    //  wr_buf[buf_idx++] = RisingEdgeTime - FallingEdgeTime;
  }
  else
  {
    //clear bit
    PORT_ICPFOLLOWER &= ~(1<<FOLLOWER_BIT);

    
    // Falling Edge
    FallingEdgeTime = ICR5;
    TCCRB |= (1 << ICES5); // Switch to Rising Edge
    //PulseTime = FallingEdgeTime - RisingEdgeTime;
    
    //if (buf_idx < sizeof(wr_buf));
    //  wr_buf[buf_idx++] = FallingEdgeTime - RisingEdgeTime;
  }
}
