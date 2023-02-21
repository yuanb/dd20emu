#include "wrdata.h"

uint8_t wr_buf[WRBUF_SIZE] = {0};

void initICPTimer()
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

static uint16_t buf_idx = 0;
static int8_t bit_idx = 7;
//static uint8_t value = 0;

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

  //TODO and last puls was a short one, use a 10% threshold with calculated timer value
  //observed values:
  //short pulse 52,54,55,56,57
  //middle pulse 15F, 160,161,162,
  //long pulse 1DB, 1DC, 1DE, 1DF
  
  //uint8_t nibble;
  if (0x120<= PulseTime && PulseTime <= 0x166)
    //nibble = 1;
    bitSet(wr_buf[buf_idx], bit_idx);
  else if (0x1D0 <= PulseTime && PulseTime <=0x200 )
    //nibble = 0;
    bitClear(wr_buf[buf_idx], bit_idx);
  else
    return;

  //value = (value << bit_idx) | nibble;

  //wr_buf[buf_idx] = nibble;
  //bit_idx = 8;
  
  //wr_buf[buf_idx] = value;
  bit_idx--;

  if (bit_idx < 0) {
    bit_idx = 7;
    //value = 0;
    if (buf_idx < WRBUF_SIZE) {
      buf_idx++;
    }
  }
}
