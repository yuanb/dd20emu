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

//RD Data PB7
#define RD_HIGH   PORTB |= 0x80;
#define RD_LOW    PORTB &= ~0x80;

inline void put_byte(byte b)
{
  byte mask[8] = { 0xf0, 0x40, 0x20, 0x10, 0x0f, 0x04, 0x02, 0x01 };
  for (int i = 0; i < 8; i++)
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
  int i = 13;
  {
    sector_t *sec = (sector_t *)&fdc_data[i/*sector_interleave[i]*/ * sizeof(sector_t)];

#if 0
    sec_hdr_t *hdr = &sec->header;

    serial_log("\r\nTrk# %02X, Sect# %02X", hdr->TR, hdr->SC);
    serial_log("Output sector# %d, length: %d", hdr->SC, SECSIZE_VZ);
    Serial.println("Sec data:");
#endif
    for (int j = 0; j < SECSIZE_VZ; j++)
    {
      byte v = ((byte *)sec)[j];
      //byte v = fdc_data[n];
      put_byte( v );

#if 0
      if (j % 32 == 0)
      {
        Serial.println("");
      }
      serial_log("%02X ", v );
#endif
    }
#if 0
    Serial.println("");
#endif
  }
}
