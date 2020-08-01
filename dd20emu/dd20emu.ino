
/*
 * DD-20 emulator
 * Author: Bill Yow
 *
 */
#include <SPI.h>
#include <SD.h>

/*
 * Port 10h      Latch(write-only)
 *      11h     DATA(read-only)
 *      12h     Pooling(read-only)
 *      13h     Write protection status(read-only)
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


//Emulator variables
const int ledPin =  LED_BUILTIN;// the number of the LED pin Arduino
//const int ledPin = 2;   //Ethernet shield
uint8_t ledState = LOW;             // ledState used to set the LED

uint8_t vtech1_fdc_latch = 0;
uint8_t vtech1_track_x2 = 80;

bool drv_enabled = false;
bool write_request = false;

bool old_drv_enabled = -1;
bool old_wr_req = -1;
int old_track = -1;

#define PIN_EN_REG    PINE
#define PIN_EN_BIT    5
#define PIN_WR_REG    PINE
#define PIN_WRREQ_BIT 4
const byte enPin    = 3;
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

char str_buf[100]={ 0 };
char filename[] = "FLOPPY1.DSK";
  
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

  pinMode(enPin, INPUT_PULLUP);
  pinMode(wrReqPin, INPUT_PULLUP);

  pinMode(stepPin0, INPUT_PULLUP);
  pinMode(stepPin1, INPUT_PULLUP);
  pinMode(stepPin2, INPUT_PULLUP);
  pinMode(stepPin3, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(enPin), driveEnabled, CHANGE);
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
    sprintf(str_buf, "Enabled=%d", drv_enabled);
    Serial.println(str_buf);
  } 
}

void handle_wr_request() {
  if (!drv_enabled)
    return;

  bool wrRequest = write_request;
  if (wrRequest != old_wr_req) {
    sprintf(str_buf, "Trk: %d, %s", vtech1_track_x2/2, wrRequest ? "Write" : "Read");
    Serial.println(str_buf);
    old_wr_req = wrRequest;
  }  
}

void handle_steps() {
  if (!drv_enabled)
    return;

  uint8_t data = PIN_STEP_REG;
  if( (PHI0(data) && !(PHI1(data) || PHI2(data) || PHI3(data)) && PHI1(vtech1_fdc_latch)) ||
           (PHI1(data) && !(PHI0(data) || PHI2(data) || PHI3(data)) && PHI2(vtech1_fdc_latch)) ||
           (PHI2(data) && !(PHI0(data) || PHI1(data) || PHI3(data)) && PHI3(vtech1_fdc_latch)) ||
           (PHI3(data) && !(PHI0(data) || PHI1(data) || PHI2(data)) && PHI0(vtech1_fdc_latch)) )
  {
    if (vtech1_track_x2>0)
      vtech1_track_x2--;
    if( (vtech1_track_x2 & 1) == 0 ) {
      vtech1_get_track(); 
    }
    if (vtech1_track_x2%2 == 0) {
      sprintf(str_buf, "Seek: %d", vtech1_track_x2/2);
      Serial.println(str_buf);
    }
  }
  else if( (PHI0(data) && !(PHI1(data) || PHI2(data) || PHI3(data)) && PHI3(vtech1_fdc_latch)) ||
         (PHI1(data) && !(PHI0(data) || PHI2(data) || PHI3(data)) && PHI0(vtech1_fdc_latch)) ||
         (PHI2(data) && !(PHI0(data) || PHI1(data) || PHI3(data)) && PHI1(vtech1_fdc_latch)) ||
         (PHI3(data) && !(PHI0(data) || PHI1(data) || PHI2(data)) && PHI2(vtech1_fdc_latch)) )
  {
    if( vtech1_track_x2 < 2*40 )
      vtech1_track_x2++;
    if( (vtech1_track_x2 & 1) == 0 ) {
      vtech1_get_track();    
    }
    if (vtech1_track_x2%2 == 0) {
      sprintf(str_buf, "Seek: %d", vtech1_track_x2/2);
      Serial.println(str_buf);
    }
  }
  vtech1_fdc_latch = data;
}

void handle_wr() {
  if (drv_enabled && !write_request) {
//    put_track();
  }
}

//Arduino interruption on pin change, nice Arduino interrupt tutorial
//https://arduino.stackexchange.com/questions/8758/arduino-interruption-on-pin-change
void driveEnabled() {
  //drv_enabled = !digitalRead(enPin);
  drv_enabled = !(PIN_EN_REG & (1 << PIN_EN_BIT));
}

void writeRequest() {
  write_request = !(PIN_WR_REG & (1 << PIN_WRREQ_BIT));
}

void vtech1_get_track() {
  
}

int get_track(int n)
{ 
  int size = TRKSIZE_VZ;
  int offset = TRKSIZE_VZ * n;

  if (n<0 || n>39)
  {
    sprintf(str_buf, "Invalid track number %d", offset);
    Serial.println(str_buf);
    return -1;
  }

  File f = SD.open(filename, FILE_READ);
  if (f == false)
  {
    Serial.println("DSK File is not opened");
    return -1;
  }

  if (f.seek(offset) == false)
  {
    sprintf(str_buf, "Failed seek file to %d", offset);
    Serial.println(str_buf);
    return -1;
  }

  //Read track
  sprintf(str_buf, "Read track #%d", n);
  Serial.println(str_buf);
    
  if (f.read(fdc_data, size) == -1)
  {
    sprintf(str_buf, "Failed to read track %d", n);
    Serial.println(str_buf);
    return -1;
  }

  sprintf(str_buf, "Trk %d is ready", n);
  Serial.println(str_buf);

#ifdef  DEBUG_TRACK
  for(int i=0; i<SEC_NUM; i++)
  {
    sector_t *sec = (sector_t *)&fdc_data[i* sizeof(sector_t)];
    sec_hdr_t *hdr = &sec->header;

    sprintf(str_buf, "\r\nSector # %02x", i);
    Serial.println(str_buf);
    
    Serial.print("GAP1: ");
    for(int j=0; j<sizeof(hdr->GAP1); j++)
    {
      sprintf(str_buf, "%02X ", hdr->GAP1[j]);
      Serial.print(str_buf);
    }
 
    Serial.print("\r\nIDAM_leading: ");
    for(int j=0; j<sizeof(hdr->IDAM_leading); j++)
    {
      sprintf(str_buf, "%02X ", hdr->IDAM_leading[j]);
      Serial.print(str_buf);
    }

    sprintf(str_buf, "\r\nTR: %02X, SC: %02X, T+S: %02X", hdr->TR, hdr->SC, hdr->TS_sum);
    Serial.println(str_buf);

    Serial.print("GAP2: ");
    for(int j=0; j<sizeof(hdr->GAP2); j++)
    {
      sprintf(str_buf, "%02X ", hdr->GAP2[j]);
      Serial.print(str_buf);
    }

    Serial.print("\r\nIDAM_closing: ");
    for(int j=0; j<sizeof(hdr->IDAM_closing); j++)
    {
      sprintf(str_buf, "%02X ", hdr->IDAM_closing[j]);
      Serial.print(str_buf);
    }

    sprintf(str_buf, "\r\nNext TR: %02X, Next SC: %02X", sec->next_track, sec->next_sector);
    Serial.println(str_buf);

  }
#endif    
}

//Good values
#if 1
#define delay_1us   __builtin_avr_delay_cycles (14)
#define delay_11us  __builtin_avr_delay_cycles (170)
#define delay_20us  __builtin_avr_delay_cycles (300)
#define delay_31_2us  __builtin_avr_delay_cycles (485)
#else
//Testing values
#define delay_1us   __builtin_avr_delay_cycles (15)
#define delay_11us  __builtin_avr_delay_cycles (190)
#define delay_20us  __builtin_avr_delay_cycles (290)
#define delay_31_2us  __builtin_avr_delay_cycles (520)
#endif

#define RD_HIGH   PORTB |= 0x80;
#define RD_LOW    PORTB &= ~0x80;

inline void put_byte(byte b)
{
  byte mask[8] = { 0xf0, 0x40, 0x20, 0x10, 0x0f, 0x04, 0x02, 0x01 };  
  for (int i=0; i<8; i++)
  {
    //Clock bit
    RD_HIGH; delay_1us; RD_LOW;

    //Data bit
    if (b & mask[i])
    {
      delay_11us; 
      RD_HIGH; delay_1us; RD_LOW;
      delay_20us;
    }
    else
    {
      delay_31_2us; 
    }       
  }  
}

int sector_interleave[SEC_NUM] = { 0, 11, 6, 1, 12, 7, 2, 13, 8, 3, 14, 9, 4, 15, 10, 5 };
void put_track()
{
  //for(int n=0; n<TRKSIZE_VZ; n++)
  //for(int i=0; i<SEC_NUM; i++)
  int i=13;
  {
    sector_t *sec = (sector_t *)&fdc_data[i/*sector_interleave[i]*/ * sizeof(sector_t)];
    
#if 0
    sec_hdr_t *hdr = &sec->header;
    
    sprintf(str_buf, "\r\nTrk# %02X, Sect# %02X", hdr->TR, hdr->SC);
    Serial.println(str_buf);

    sprintf(str_buf, "Output sector# %d, length: %d", hdr->SC, SECSIZE_VZ);
    Serial.println(str_buf);

    Serial.println("Sec data:");
#endif
    for(int j=0; j<SECSIZE_VZ; j++)
    {
      byte v = ((byte *)sec)[j];
      //byte v = fdc_data[n];
      put_byte( v );
            
#if 0     
      if (j%32 == 0)
      {
        Serial.println("");
      }
      sprintf(str_buf, "%02X ", v );
      Serial.print(str_buf);
#endif
    }
#if 0
    Serial.println("");
#endif
  }
}
