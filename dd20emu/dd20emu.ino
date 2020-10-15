
/*
   DD-20 emulator
   Author: Bill Yow

*/
#include <SPI.h>
#include <SD.h>
/*
 * /Users/billyuan/Library/Arduino15/packages/arduino/tools/avr-gcc/7.3.0-atmel3.6.1-arduino7/bin/avr-objdump -S dd20emu.ino.elf > /Users/billyuan/Documents/Arduino/dd20emu/dd20emu.lst
 * 
 */

/*
   Pin definitions, Arduino Mega2560
   GND
   13,  PB7,    RD Data
   2,   PE4,    /WRReq
   3,   PE5,    /EnDrv

   18,  PD3,    Step3
   19,  PD2,    Step2
   20,  PD1,    Step1
   21,  PD0,    Step0
*/

/*
   Port 10h     Latch(write-only)
        11h     DATA(read-only)
        12h     Pooling(read-only)
        13h     Write protection status(read-only)
*/

/**
 * Disk image formats:
    1. file size 98,560 bytes, 40 tracks x 16 sectors x 154 bytes
 *  2. file size 99184 bytes
 *  3. file size is 99200 bytes
 */

#define TRK_NUM    40
#define SEC_NUM     16
#define SECSIZE_VZ  154
#define TRKSIZE_VZ  SECSIZE_VZ * SEC_NUM
//#define DEBUG_TRACK 1

typedef struct SectorHeader {
  uint8_t   GAP1[7];    //0x80 6 times, 0x00
  uint8_t   IDAM_leading[4];  //FE, E7, 18. C3
  uint8_t   TR;
  uint8_t   SC;
  uint8_t   TS_sum;
  uint8_t   GAP2[6];    //0x80 5 times, 0x00
  uint8_t   IDAM_closing[4];  //C3, 18, E7, FE
} sec_hdr_t;

typedef struct Sector {
  sec_hdr_t   header;
  uint8_t     data_bytes[126];
  uint8_t     next_track;
  uint8_t     next_sector;
  uint16_t    checksum;
} sector_t;

typedef struct Track {
  sector_t    sector[16];
} track_t;

//uint8_t   fdc_data[TRKSIZE_VZ];
uint8_t fdc_sector[SECSIZE_VZ];

//#define TRKSIZE_FM  3172
//uint8_t   fm_track_data[TRKSIZE_FM];

uint8_t vtech1_fdc_latch = 0;
uint8_t vtech1_track_x2 = 80;

//Emulator variables
const int ledPin =  LED_BUILTIN;// the number of the LED pin Arduino
//const int ledPin = 2;         //Ethernet shield
uint8_t ledState = LOW;         // ledState used to set the LED

const byte rdDataPin = 13;
#define PIN_EN_REG      PINE
#define PIN_EN_BIT      5
#define PIN_EN_MASK     0x20      //1 << PIN_EN_MASK
#define PIN_WR_REG      PINE
#define PIN_WRREQ_BIT   4
#define PIN_WRREQ_MASK  0x10      //1 << PIN_WRREQ_BIT
const byte enDrvPin     = 3;
const byte wrReqPin     = 2;

//Mega 2560 only, ststepPin0epPin interrupt register
#define PIN_STEP_REG    PIND  // interrupt 0 is on AVR pin PD2
#define PIN_STEP3_BIT   3
#define PIN_STEP2_BIT   2
#define PIN_STEP1_BIT   1
#define PIN_STEP0_BIT   0

const byte stepPin0 = 21;
const byte stepPin1 = 20;
const byte stepPin2 = 19;
const byte stepPin3 = 18;

#define PHI0(n) (((n)>>PIN_STEP0_BIT)&1)
#define PHI1(n) (((n)>>PIN_STEP1_BIT)&1)
#define PHI2(n) (((n)>>PIN_STEP2_BIT)&1)
#define PHI3(n) (((n)>>PIN_STEP3_BIT)&1)

//Disk image format 1, FLOPPY1.DSK and FLOPPY2.DSK
//Penguin wont load, D1B and VZCAVE wont run. The rest are ok
char filename[] = "FLOPPY1.DSK";

//Disk image format 2 (formatted from vzemu)
//char filename[] = "HELLO.DSK";
File f;

extern bool drv_enabled;
extern bool write_request;

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  if (!SD.begin())
  {
    Serial.println("Failed to begin on SD");
  }

  if (!SD.exists(filename))
  {
    Serial.println("Can't find FLOPPY1.DSK");
  }

  // put your setup code here, to run once:
  // set the digital pin as output:
  pinMode(ledPin, OUTPUT);

  pinMode(enDrvPin, INPUT_PULLUP);
  pinMode(rdDataPin, OUTPUT);
  pinMode(wrReqPin, INPUT_PULLUP);

  pinMode(stepPin0, INPUT_PULLUP);
  pinMode(stepPin1, INPUT_PULLUP);
  pinMode(stepPin2, INPUT_PULLUP);
  pinMode(stepPin3, INPUT_PULLUP);


  f = SD.open(filename, FILE_READ);
  if (f == false)
  {
    Serial.println("DSK File is not opened");
    return -1;
  }    

  attachInterrupt(digitalPinToInterrupt(wrReqPin), writeRequest, CHANGE);
  attachInterrupt(digitalPinToInterrupt(enDrvPin), driveEnabled, CHANGE);
  attachInterrupt(digitalPinToInterrupt(stepPin0), handle_steps, CHANGE);
  attachInterrupt(digitalPinToInterrupt(stepPin1), handle_steps, CHANGE);
  attachInterrupt(digitalPinToInterrupt(stepPin2), handle_steps, CHANGE);
  attachInterrupt(digitalPinToInterrupt(stepPin3), handle_steps, CHANGE);

  Serial.println("Begin DD-20 emulation\r");
}

uint8_t old_vtech1_track_x2 = 81;
void loop() {
  handle_wr();
}
