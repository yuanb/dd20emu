#include "wrdata.h"

uint8_t wr_buf[WRBUF_SIZE] = {0};
#ifdef PULSETIME
uint16_t wr_buf16[WRBUF16_SIZE] = {0};
#endif

bool skipFirst = true;
inline void initICP()
{
  skipFirst = true;
  //Mega2560 Timer 5 for ICP5
  TCCRA = 0;
  TCCRB = 0;

  //TIFR = bit(ICF) | bit(TOV);

  //TCCRB = bit(ICNC) | /*bit(ICES)*/ | bit (CS1);
  //0b11 000 010 - 1/8 prescaler (010) rising edge noise canceler
  //0b11 000 001 - 1/1 prescaler (001) rising edge noise canceler
  //0b10 000 001 - 1/1 prescaler (001) falling edge noise canceler
  TCCRB = 0b10000001;
  TCNT = 0;
  TIMSK |= (1<<ICIE);   // enable input capture interrupt
}

inline void disableICP()
{ 
  TIMSK &= ~(1<<ICIE);  // disable input capture interrupt 
  TCCRB = 0b10000000;   // disable Timer
  TCNT = 0;
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

uint16_t buf_idx = 0;
static int8_t bit_idx = 7;

ISR(TIMER5_CAPT_vect)
{
  TCNT = 0;
  uint16_t PulseTime = ICR5;
  
  // Which edge is armed?
  if (TCCRB & (1 << ICES5))
  {
    //set bit
    PORT_ICPFOLLOWER |= (1<< FOLLOWER_BIT);

    // Rising Edge
    TCCRB &= ~(1 << ICES5); // Switch to Falling Edge
  }
  else
  {
    //clear bit
    PORT_ICPFOLLOWER &= ~(1<<FOLLOWER_BIT);

    // Falling Edge
    TCCRB |= (1 << ICES5); // Switch to Rising Edge
  }

#ifdef  PULSETIME
    if (skipFirst) {
      skipFirst = false;
    } else {
      wr_buf16[buf_idx] = PulseTime;
      if (buf_idx < WRBUF16_SIZE-1) {
        buf_idx++;
      }
    }
#else
  if (0x120<= PulseTime && PulseTime <= 0x166)
    //nibble = 1;
    bitSet(wr_buf[buf_idx], bit_idx);
  else if (0x1D0 <= PulseTime && PulseTime <=0x200 )
    //nibble = 0;
    bitClear(wr_buf[buf_idx], bit_idx);
  else
    return;

  bit_idx--;

  if (bit_idx < 0) {
    bit_idx = 7;
    if (buf_idx < WRBUF_SIZE) {
      buf_idx++;
    }
  }
#endif  //PULSETIME
}
