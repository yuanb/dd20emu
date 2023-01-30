#define TEMPBUFFER_SIZE 80
byte tempbuffer[TEMPBUFFER_SIZE];


//https://github.com/dhansel/ArduinoFDC/blob/main/ArduinoFDC.ino
char *read_user_cmd(void *buffer, int buflen)
{
  char *buf = (char *) buffer;
  byte l = 0;
  do
    {
      int i = Serial.read();

      if( (i==13 || i==10) )
        { Serial.println(); break; }
      else if( i==27 )
        { l=0; Serial.println(); break; }
      else if( i==8 )
        { 
          if( l>0 )
            { Serial.write(8); Serial.write(' '); Serial.write(8); l--; }
        }
      else if( isprint(i) && l<buflen-1 )
        { buf[l++] = i; Serial.write(i); }
      
      //if( motor_timeout>0 && millis() > motor_timeout ) { ArduinoFDC.motorOff(); motor_timeout = 0; }
    }
  while(true);

  while( l>0 && isspace(buf[l-1]) ) l--;
  buf[l] = 0;
  return buf;
}

void print_enter_msg()
{
  serial_log(PSTR("Press ENTER to enter dd20emu shell.\r\n"));
}

void print_help_screen()
{
  serial_log(PSTR("?/help - Print screen\r\n"));
  serial_log(PSTR("status - Display emulator status\r\n"));
  serial_log(PSTR("dir - List current directory\r\n"));
  serial_log(PSTR("mount filename - Mount disk image\r\n"));
  serial_log(PSTR("catalog - Catalog DD20 disk image\r\n"));
  serial_log(PSTR("dumplut - Dump LUT table\r\n"));
  serial_log(PSTR("dumpsect n s - Dump sector on track n, sector s\r\n"));
  serial_log(PSTR("exit - Exit from shell, resume emulator\r\n"));
}

void print_status()
{
  serial_log(PSTR("Mounted image file: %s\r\n"), filename);
  serial_log(PSTR("tracking padding: %d\r\n"), vzdsk->get_track_padding());
  serial_log(PSTR("current track: %d\r\n"), vtech1_track_x2/2);
  serial_log(PSTR("sector size: %d\r\n"), SECSIZE_VZ);
}

void sd_dir()
{
  //TODO: separate SD object from vzdsk
  SdFat* pSD = vzdsk->get_sd();
  if (pSD->fatType() <=32) {
    serial_log(PSTR("Volume is FAT%d"), pSD->fatType());
    serial_log(PSTR("\r\n"));
  } else {
    serial_log(PSTR("Volume is exFAT\r\n"));
  }
  
  //List file not hidden, ends with DSK
}

void mount_image(char* filename)
{
  
}

void img_catalog()
{
  
}

extern int8_t sec_lut[TRK_NUM][SEC_NUM/2];
void dump_lut()
{
  serial_log(PSTR("\r\nsec_lut: '0' means 6x80h+1x00h; '1' mean 5x80h+1x00h \r\n"));
  for(int i=0; i<TRK_NUM; i++)
  {
    serial_log(PSTR("TR:%02d  "),i);
    for(int j=0; j<SEC_NUM/2; j++)
    {
      serial_log(PSTR("%02x "), sec_lut[i][j]);
    }
    serial_log(PSTR("\r\n"));
  }  
}

void dump_sector(int n, int s)
{
  serial_log(PSTR("TR:%d, SC:%d\r\n"), n,s);
  if(vzdsk->get_sector(n, s) != -1)
  {
    for(int i=0; i < SECSIZE_VZ; i++)
    {
      int finish = i+16 < SECSIZE_VZ ? i+16 : SECSIZE_VZ;
  
      for(int j=i; j<i+16; j++)
      {
        if (j<finish) {
          serial_log(PSTR("%02X "), fdc_sector[j]);
        }
        else {
          serial_log(PSTR("   "));
        }
      }
      for(int j=i; j<finish; j++)
      {
        if (fdc_sector[j]>0x20 && fdc_sector[j]<0x7f) {
          serial_log(PSTR("%c"), fdc_sector[j]);
        }
        else {
          serial_log(PSTR("."));
        }
      }

      i+=15;  //cause there is i++ in the loop
      serial_log(PSTR("\r\n"));      
    }

    //TODO: add checksum
    serial_log(PSTR("checksum: %d\r\n"), 0);
    serial_log(PSTR("\r\n"));
  }
}

void handle_shell()
{
  serial_log(PSTR("DD-20 emulator command shell.\r\n"));
  serial_log(PSTR("Emulator is PAUSED.\r\n"));
  serial_log(PSTR("type ?/help for help screen.\r\n"));
  bool in_shell = true;
  while(in_shell) {
    Serial.print(">");
    char *cmd = read_user_cmd(tempbuffer, TEMPBUFFER_SIZE);

    //help
    if( strncmp_P(cmd, PSTR("?"), 1)==0 || strncmp_P(cmd, PSTR("help"), 3)==0) {
      print_help_screen();
    }

    //status
    else if (strncmp_P(cmd, PSTR("status"), 6)==0) {
      print_status();
    }

    //dir
    else if (strncmp_P(cmd, PSTR("dir"), 3)==0) {
      sd_dir();
    }

    //mount
    else if (strncmp_P(cmd, PSTR("mount"), 5)==0) {
      //TODO parse filename
      mount_image(cmd);
    }

    //catalog
    else if (strncmp_P(cmd, PSTR("catalog"), 7)==0) {
      img_catalog();
    }

    //dumplut
    else if (strncmp_P(cmd, PSTR("dumplut"), 7)==0) {
      dump_lut();
    }

    //dumpsect
    else if (strncmp_P(cmd, PSTR("dumpsect "), 9)==0) {
      int n=-1,s=-1;
      sscanf(cmd+9, "%d %d", &n,&s);
      if (n!=-1 && s!=-1) {
        dump_sector(n, s);
      } else
        serial_log(PSTR("dumpsect n s\r\n"));
    }

    //exit
    else if (strncmp_P(cmd, PSTR("exit"), 4)==0) {
      serial_log(PSTR("Emulator is RESUMED.\r\n"));
      print_enter_msg();
      in_shell = false;
    }
    else if( cmd[0]!=0 )
    {
      serial_log(PSTR("Unknown command: %s\r\n"), cmd); 
    }
  }  
}
