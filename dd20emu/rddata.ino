//Good values

//FM encoded 0 : ___n__________n___ : 1.125us + 31.75us
//FM encoded 1 : ___n___n______n___ : 1.125us + 11 us + 1.125us + 20 us


#define delay_1us   __builtin_avr_delay_cycles (16)
#define delay_11us  __builtin_avr_delay_cycles (170)
#define delay_20us  __builtin_avr_delay_cycles (300)
#define delay_31_2us  __builtin_avr_delay_cycles (488)


//RD Data PB7
#define RD_HIGH   PORTB |= 0x80;
#define RD_LOW    PORTB &= ~0x80;

#define FM_BIT_1  delay_11us; RD_HIGH;  delay_1us;  RD_LOW; delay_20us;
#define FM_BIT_0  delay_31_2us;

byte bitmask[8] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };

#define FM_ENCODE_BIT(v,m) {  if (v & m) { FM_BIT_1; } else { FM_BIT_0; } }

#define FM_OUTPUT_BIT(v,m) { RD_HIGH; delay_1us; RD_LOW;  FM_ENCODE_BIT(v,m); }

inline void put_byte(byte v)
{
  FM_OUTPUT_BIT(v,bitmask[0]);
  FM_OUTPUT_BIT(v,bitmask[1]);
  FM_OUTPUT_BIT(v,bitmask[2]);
  FM_OUTPUT_BIT(v,bitmask[3]);
  FM_OUTPUT_BIT(v,bitmask[4]);
  FM_OUTPUT_BIT(v,bitmask[5]);
  FM_OUTPUT_BIT(v,bitmask[6]);
  FM_OUTPUT_BIT(v,bitmask[7]);
}

uint8_t current_sector = 0;
void handle_wr() {
  if (drv_enabled && !write_request) {
    put_sector((uint8_t)vtech1_track_x2/2, current_sector);
    if (++current_sector >= SEC_NUM)
      current_sector = 0;      
  }
}

extern uint8_t fdc_sector[SECSIZE_VZ];
extern vzdisk* vzdsk;
inline void put_sector(uint8_t n, uint8_t s)
{
  if (vzdsk->get_sector(n, s) != -1)
  {     
    for(int j=0; j < SECSIZE_VZ; j++)
    {
      byte v = (byte *)fdc_sector[j];
      put_byte(v);
    }
  }
}
