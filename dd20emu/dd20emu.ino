
/*
   DD-20 emulator
   Author: Bill Yow

*/
#include <SPI.h>
#include <SD.h>

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
#define TRK_NUM    40
#define SEC_NUM     16
#define SECSIZE_VZ  154
#define TRKSIZE_VZ  SECSIZE_VZ * SEC_NUM
//#define DEBUG_TRACK 1

#define TRKSIZE_FM  3172

typedef struct SectorHeader {
  uint8_t   GAP1[7];
  uint8_t   IDAM_leading[4];
  uint8_t   TR;
  uint8_t   SC;
  uint8_t   TS_sum;
  uint8_t   GAP2[6];
  uint8_t   IDAM_closing[4];
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

uint8_t   fdc_data[TRKSIZE_VZ];
uint8_t   fm_track_data[TRKSIZE_FM];

uint8_t vtech1_fdc_latch = 0;
uint8_t vtech1_track_x2 = 80;

bool drv_enabled = false;
bool write_request = false;

bool old_drv_enabled = -1;
bool old_wr_req = -1;
int old_track = -1;

//Emulator variables
const int ledPin =  LED_BUILTIN;// the number of the LED pin Arduino
//const int ledPin = 2;         //Ethernet shield
uint8_t ledState = LOW;         // ledState used to set the LED

const byte rdDataPin = 13;
#define PIN_EN_REG    PINE
#define PIN_EN_BIT    5
#define PIN_WR_REG    PINE
#define PIN_WRREQ_BIT 4
const byte enDrvPin    = 3;
const byte wrReqPin = 2;

//Mega 2560 only, ststepPin0epPin interrupt register
#define PIN_STEP_REG    PIND  // interrupt 0 is on AVR pin PD2
#define PIN_STEP3_BIT   3
#define PIN_STEP2_BIT   2
#define PIN_STEP1_BIT   1
#define PIN_STEP0_BIT   0
const byte stepPin3 = 18;
const byte stepPin2 = 19;
const byte stepPin1 = 20;
const byte stepPin0 = 21;

#define PHI0(n) (((n)>>PIN_STEP0_BIT)&1)
#define PHI1(n) (((n)>>PIN_STEP1_BIT)&1)
#define PHI2(n) (((n)>>PIN_STEP2_BIT)&1)
#define PHI3(n) (((n)>>PIN_STEP3_BIT)&1)

char filename[] = "FLOPPY1.DSK";

void serial_log( const char * format, ... )
{
  char buffer[100];
  va_list args;
  va_start (args, format);
  vsprintf (buffer, format, args);
  Serial.println(buffer);
  va_end (args);
}

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

  attachInterrupt(digitalPinToInterrupt(enDrvPin), driveEnabled, CHANGE);
  attachInterrupt(digitalPinToInterrupt(wrReqPin), writeRequest, CHANGE);

  Serial.println("Begin DD-20 emulation\r");
  get_track(0);
}

void loop() {
  handle_drive_enable();
  handle_wr_request();
  handle_steps();
  handle_wr();
}

void handle_drive_enable() {
  bool isEnabled = drv_enabled;
  if (isEnabled != old_drv_enabled) {
    digitalWrite(LED_BUILTIN, isEnabled);
    old_drv_enabled = isEnabled;
    serial_log("Enabled=%d", drv_enabled);
  }
}

void handle_wr_request() {
  if (!drv_enabled)
    return;

  bool wrRequest = write_request;
  if (wrRequest != old_wr_req) {
    serial_log("Trk: %d, %s", vtech1_track_x2 / 2, wrRequest ? "Write" : "Read");
    old_wr_req = wrRequest;
  }
}

void handle_steps() {
  if (!drv_enabled)
    return;

  uint8_t data = PIN_STEP_REG;
  if ( (PHI0(data) && !(PHI1(data) || PHI2(data) || PHI3(data)) && PHI1(vtech1_fdc_latch)) ||
       (PHI1(data) && !(PHI0(data) || PHI2(data) || PHI3(data)) && PHI2(vtech1_fdc_latch)) ||
       (PHI2(data) && !(PHI0(data) || PHI1(data) || PHI3(data)) && PHI3(vtech1_fdc_latch)) ||
       (PHI3(data) && !(PHI0(data) || PHI1(data) || PHI2(data)) && PHI0(vtech1_fdc_latch)) )
  {
    if (vtech1_track_x2 > 0)
      vtech1_track_x2--;
    if ( (vtech1_track_x2 & 1) == 0 ) {
      vtech1_get_track();
    }
    if (vtech1_track_x2 % 2 == 0) {
      serial_log("Seek: %d", vtech1_track_x2 / 2);
    }
  }
  else if ( (PHI0(data) && !(PHI1(data) || PHI2(data) || PHI3(data)) && PHI3(vtech1_fdc_latch)) ||
            (PHI1(data) && !(PHI0(data) || PHI2(data) || PHI3(data)) && PHI0(vtech1_fdc_latch)) ||
            (PHI2(data) && !(PHI0(data) || PHI1(data) || PHI3(data)) && PHI1(vtech1_fdc_latch)) ||
            (PHI3(data) && !(PHI0(data) || PHI1(data) || PHI2(data)) && PHI2(vtech1_fdc_latch)) )
  {
    if ( vtech1_track_x2 < 2 * 40 )
      vtech1_track_x2++;
    if ( (vtech1_track_x2 & 1) == 0 ) {
      vtech1_get_track();
    }
    if (vtech1_track_x2 % 2 == 0) {
      serial_log("Seek: %d", vtech1_track_x2 / 2);
    }
  }
  vtech1_fdc_latch = data;
}

void handle_wr() {
  if (drv_enabled && !write_request) {
    put_track();
  }
}

//Arduino interruption on pin change, nice Arduino interrupt tutorial
//https://arduino.stackexchange.com/questions/8758/arduino-interruption-on-pin-change
void driveEnabled() {
  //drv_enabled = !digitalRead(enDrvPin);
  drv_enabled = !(PIN_EN_REG & (1 << PIN_EN_BIT));
}

void writeRequest() {
  write_request = !(PIN_WR_REG & (1 << PIN_WRREQ_BIT));
}
