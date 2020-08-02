
void vtech1_get_track() {

}

int get_track(int n)
{
  int size = TRKSIZE_VZ;
  int offset = TRKSIZE_VZ * n;

  if (n < 0 || n > 39)
  {
    serial_log("Invalid track number %d", offset);
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
    serial_log("Failed seek file to %d", offset);
    return -1;
  }

  //Read track
  serial_log("Read track #%d", n);

  if (f.read(fdc_data, size) == -1)
  {
    serial_log("Failed to read track %d", n);
    return -1;
  }

  serial_log("Trk %d is ready", n);

#ifdef  DEBUG_TRACK
  for (int i = 0; i < SEC_NUM; i++)
  {
    sector_t *sec = (sector_t *)&fdc_data[i * sizeof(sector_t)];
    sec_hdr_t *hdr = &sec->header;

    serial_log("\r\nSector # %02x", i);

    Serial.print("GAP1: ");
    for (int j = 0; j < sizeof(hdr->GAP1); j++)
    {
      serial_log("%02X ", hdr->GAP1[j]);
    }

    Serial.print("\r\nIDAM_leading: ");
    for (int j = 0; j < sizeof(hdr->IDAM_leading); j++)
    {
      serial_log("%02X ", hdr->IDAM_leading[j]);
    }

    serial_log("\r\nTR: %02X, SC: %02X, T+S: %02X", hdr->TR, hdr->SC, hdr->TS_sum);

    Serial.print("GAP2: ");
    for (int j = 0; j < sizeof(hdr->GAP2); j++)
    {
      serial_log("%02X ", hdr->GAP2[j]);
    }

    Serial.print("\r\nIDAM_closing: ");
    for (int j = 0; j < sizeof(hdr->IDAM_closing); j++)
    {
      serial_log("%02X ", hdr->IDAM_closing[j]);
    }

    serial_log("\r\nNext TR: %02X, Next SC: %02X", sec->next_track, sec->next_sector);
  }
#endif
}
