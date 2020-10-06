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

/*
int sector_interleave[SEC_NUM] = { 0, 11, 6, 1, 12, 7, 2, 13, 8, 3, 14, 9, 4, 15, 10, 5 };
void put_track()
{
  //for(int n=0; n<TRKSIZE_VZ; n++)
  
  for(int i=0; i<SEC_NUM; i++)
  {
    sector_t *sec = (sector_t *)&fdc_data[sector_interleave[i] * sizeof(sector_t)];
    for (int j = 0; j < SECSIZE_VZ; j++)
    {
      byte v = ((byte *)sec)[j];
      put_byte(v); 
    }
  }
}
*/
byte current_sector = 0;
void handle_wr() {
  if (drv_enabled && !write_request) {
    put_sector(f, vtech1_track_x2/2, current_sector);
    if (++current_sector >= SEC_NUM)
      current_sector = 0;
  }
}

inline void put_sector(File f, byte n, byte s)
{
  if (get_sector(f, n, s) != -1)
  {
    for(int j=0; j < SECSIZE_VZ; j++)
    {
      byte v = (byte *)fdc_sector[j];
      put_byte(v);
    }
  }
}
