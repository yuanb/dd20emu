//Total : 32.2us per bit, about 3882 baud/s
//FM encoded 0 : ___n__________n___ : 1 us + 31.2us
//FM encoded 1 : ___n___n______n___ : 1us + 11 us + 1us + 19.2us

/* Pin assignment for Arduino Meage 2560*/
#define PIN_WRITEDATA 46  // must be pin 46 (PL3, OCP for timer5)

asm ("   .equ TIFR,    0x1A\n"  // timer 5 flag register
     "   .equ TOV,     0\n"     // overflow flag
     "   .equ OCF,     1\n"     // output compare flag
     "   .equ ICF,     5\n"     // input capture flag
     "   .equ TCCRC,   0x122\n" // timer 5 control register C
     "   .equ FOC,     0x80\n"  // force output compare flag
     "   .equ TCNTL,   0x124\n" // timer 5 counter (low byte)
     "   .equ ICRL,    0x126\n" // timer 5 input capture register (low byte)
     "   .equ OCRL,    0x128\n" // timer 5 output compare register (low byte)
     );

#define TIFR    TIFR5   // timer 5 flag register
#define TOV     TOV5    // overflow flag
#define OCF     OCF5A   // output compare flag
#define ICF     ICF5    // input capture flag
#define TCCRA   TCCR5A  // timer 5 control register A
#define COMA1   COM5A1  // timer 5 output compare mode bit 1
#define COMA0   COM5A0  // timer 5 output compare mode bit 0
#define TCCRB   TCCR5B  // timer 5 control register B
#define CS1     CS51    // timer 5 clock select bit 1
#define CS0     CS50    // timer 5 clock select bit 0
#define WGM2    WGM52   // timer 5 waveform mode bit 2
#define TCCRC   TCCR5C  // timer 5 control register C
#define FOC     FOC5A   // force output compare flag
#define OCR     OCR5A   // timer 5 output compare register
#define TCNT    TCNT5   // timer 5 counter

//PL3
#define OCDDR   DDRL    // DDR controlling WRITEDATA pin
#define OCBIT   3       // bit for WRITEDATA pin

static void rddata_timed_begin()
{
  digitalWrite(PIN_WRITEDATA, HIGH);
  pinMode(PIN_WRITEDATA, OUTPUT);
}
static void write_data(byte bitlen, byte *buffer, unsigned int n)
{
  asm (// define WRITEPULSE macro (used in write_data and format_track)
     ".macro WRITEPULSE length=0\n"
     "  .if \\length==1\n"
     "          sts   OCRL, r16\n"       // (2)   set OCRxA to short pulse length
     "  .endif\n"
     "  .if \\length==2\n"
     "          sts   OCRL, r17\n"       // (2)   set OCRxA to medium pulse length
     "  .endif\n"
     "  .if \\length==3\n"
     "          sts   OCRL, r18\n"       // (2)   set OCRxA to long pulse length
     "  .endif\n"
     "wait:     sbis  TIFR, OCF\n"       // (1/2) skip next instruction if OCFx is set
     "          rjmp  wait\n"            // (2)   wait more     "rjmp  .-4"
     "          ldi   r19,  FOC\n"       // (1)
     "          sts   TCCRC, r19\n"      // (2)   set OCP back HIGH (was set LOW when timer expired)
     "          sbi   TIFR, OCF\n"       // (2)   reset OCFx (output compare flag)
     ".endm\n");

  // wait through beginning of header gap (22 bytes of 0x4F)
  TCCRB |= bit(WGM2);             // WGMx2:10 = 010 => clear-timer-on-compare (CTC) mode 
  TCNT = 0;                       // reset timer
  //OCR = 352 * bitlen - 256;       // 352 MFM bit lengths (22 bytes * 8 bits/byte * 2 MFM bits/data bit) * cycles/MFM bit - 16us (overhead)
  //TIFR = bit(OCF);                // clear OCFx
  //while( !(TIFR & bit(OCF)) );    // wait for OCFx
  OCR = 255;                      // clear OCRH byte (we only modify OCRL below)
  TIFR = bit(OCF);                // clear OCFx
  
  // enable OC1A output pin control by timer (WRITE_DATA), initially high
  TCCRA  = bit(COMA0); // COMxA1:0 =  01 => toggle OC1A on compare match

  asm volatile
    (// define GETNEXTBIT macro for getting next data bit into carry (4/9 cycles)
     // on entry: R20         contains the current byte 
     //           R21         contains the bit counter
     //           X (R26/R27) contains the byte counter
     //           Z (R30/R31) contains pointer to data buffer
     ".macro GETNEXTBIT\n"
     "          dec     r21\n"           // (1)   decrement bit counter
     "          brpl    .+10\n"          // (1/2) skip the following if bit counter >=  0
     "          subi    r26, 1\n"        // (1)   subtract one from byte counter
     "          sbci    r27, 0\n"        // (1) 
     "          brmi    wdone\n"         // (1/2) done if byte counter <0
     "          ld  r20, Z+\n"       // (2)   get next byte
     "          ldi     r21, 7\n"        // (1)   reset bit counter (7 more bits after this first one)
     "          rol     r20\n"           // (1)   get next data bit into carry
     ".endm\n"

     // initialize pulse-length registers (r16, r17, r18)
     "          mov   r16, %0\n"         //       r16 = (2*bitlen)-1 = time for short ("01") pulse         
     "          add   r16, %0\n"
     "          dec   r16\n"
     "          mov   r17, r16\n"        //       r17 = (3*bitlen)-1 = time for medium ("001") pulse
     "          add   r17, %0\n"
     "          mov   r18, r17\n"        //       r18 = (4*bitlen)-1 = time for long ("0001") pulse
     "          add   r18, %0\n"

     // write 12 bytes (96 bits) of "0" (i.e. 96 "10" sequences, i.e. short pulses)
     "          ldi     r20, 0\n"        
     "          sts     TCNTL, r20\n"    //       reset timer
     "          ldi     r20, 96\n"       //       initialize counter
     "wri:      WRITEPULSE 1\n"          //       write short pulse
     "          dec     r20\n"           //       decremet counter
     "          brne    wri\n"           //       repeat until 0
//
//     // first sync "A1": 00100010010001001
//     "          WRITEPULSE 2\n"          //       write medium pulse
//     "          WRITEPULSE 3\n"          //       write long pulse
//     "          WRITEPULSE 2\n"          //       write medium pulse
//     "          WRITEPULSE 3\n"          //       write long pulse (this is the missing clock bit)
//     "          WRITEPULSE 2\n"          //       write medium pulse
//     
//     // second sync "A1": 0100010010001001
//     "          WRITEPULSE 1\n"          //       write short pulse
//     "          WRITEPULSE 3\n"          //       write long pulse
//     "          WRITEPULSE 2\n"          //       write medium pulse
//     "          WRITEPULSE 3\n"          //       write long pulse (this is the missing clock bit)
//     "          WRITEPULSE 2\n"          //       write medium pulse
//
//     // third sync "A1": 0100010010001001
//     "          WRITEPULSE 1\n"          //       write short pulse
//     "          WRITEPULSE 3\n"          //       write long pulse
//     "          WRITEPULSE 2\n"          //       write medium pulse
//     "          WRITEPULSE 3\n"          //       write long pulse (this is the missing clock bit)
//     "          WRITEPULSE 2\n"          //       write medium pulse
//
//     // start writing data
//     "          sts     OCRL, r16\n"     // (2)   set up timer for "01" sequence
//     "          ldi     r21, 0\n"        // (1)   initialize bit counter to fetch next byte
//
//     // just wrote a "1" bit => must be followed by either "01" (for "1" bit) or "00" (for "0" bit)
//     // (have time to fetch next bit during the leading "0")
//     "wro:      GETNEXTBIT\n"            // (4/9) fetch next data bit into carry
//     "          brcs    wro1\n"          // (1/2) jump if "1"
//     // next bit is "0" => write "00"
//     "          lds     r19,  OCRL\n"    // (2)   get current OCRxAL value
//     "          add     r19,  %0\n"      // (2)   add one-bit time
//     "          sts     OCRL, r19\n"     // (2)   set new OCRxAL value
//     "          rjmp    wre\n"           // (2)   now even
//     // next bit is "1" => write "01"
//     "wro1:     WRITEPULSE\n"            // (7)   wait and write pulse
//     "          sts     OCRL, r16\n"     // (2)   set up timer for another "01" sequence
//     "          rjmp    wro\n"           // (2)   still odd
//
//     // just wrote a "0" bit, (i.e. either "10" or "00") where time for the trailing "0" was already added
//     // to the pulse length (have time to fetch next bit during the already-added "0")
//     "wre:      GETNEXTBIT\n"            // (4/9) fetch next data bit into carry
//     "          brcs    wre1\n"          // (1/2) jump if "1"
//     // next bit is "0" => write "10"
//     "          WRITEPULSE\n"            // (7)   wait and write pulse
//     "          sts     OCRL, r16\n"     // (2)   set up timer for another "10" sequence
//     "          rjmp    wre\n"           // (2)   still even
//     // next bit is "1" => write "01"
//     "wre1:     lds     r19,  OCRL\n"    // (2)   get current OCRxAL value
//     "          add     r19,  %0\n"      // (2)   add one-bit time
//     "          sts     OCRL, r19\n"     // (2)   set new OCRxAL value
//     "          WRITEPULSE\n"            // (7)   wait and write pulse
//     "          sts     OCRL, r16\n"     // (2)   set up timer for "01" sequence
//     "          rjmp    wro\n"           // (2)   now odd
//
//     // done writing
//     "wdone:    WRITEPULSE\n"            // (9)   wait for and write final pulse

     :                                     // no outputs
     : "r"(bitlen), "x"(n), "z"(buffer) // inputs  (x=r26/r27, z=r30/r31)
     : "r16", "r17", "r18", "r19", "r20", "r21"); // clobbers


  // COMxA1:0 = 00 => disconnect OC1A (will go high)
  TCCRA = 0;

  // WGMx2:10 = 000 => Normal timer mode
  TCCRB &= ~bit(WGM2);
  
}

static void put_sector_t(uint8_t n, uint8_t s)
{
  if (vzdsk && vzdsk->get_sector(n, s) != -1)
  {
    //This should speed up disk read a bit, skip the odd tracks
    if (n != vtech1_track_x2/2)
      return;

    // set up timer
    TCCRA = 0;
    TCCRB = bit(CS0); // falling edge input capture, prescaler 1, no output compare
    TCCRC = 0;

    if (!write_request)
      write_data(32, fdc_sector, SECSIZE_VZ);
      
  }
}
