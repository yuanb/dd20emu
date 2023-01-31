/*
    DD-20 emulator
    Copyright (C) 2020-2023 https://github.com/yuanb/dd20emu

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#define TEMPBUFFER_SIZE 80
byte tempbuffer[TEMPBUFFER_SIZE];

#include "vzdisk.h"


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
    }
  while(true);

  while( l>0 && isspace(buf[l-1]) ) l--;
  buf[l] = 0;
  return buf;
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
  serial_log(PSTR("\r\nVTech DD20 emulator, v0.0.a, 01/30/2023\r\n"));
  serial_log(PSTR("\r\nSector size: %d bytes"), sizeof(sector_t));
  serial_log(PSTR("\r\nSector header size: %d bytes\r\n"), sizeof(sec_hdr_t));
  
  serial_log(PSTR("Mounted image file: %s is "), diskimage);
  if (vzdsk->get_mounted()) {
    serial_log(PSTR("mounted\r\n"));
  }
  else {
    serial_log(PSTR("not mounted\r\n"));
  }
  serial_log(PSTR("tracking padding: %d\r\n"), vzdsk->get_track_padding());
  serial_log(PSTR("current track: %d\r\n"), vtech1_track_x2/2);
  serial_log(PSTR("Free memory: %d bytes\r\n"), freeMemory());
}

void print_enter_msg()
{
  serial_log(PSTR("Press ENTER to enter dd20emu shell.\r\n"));
}

void sd_dir()
{
  //TODO: separate SD object from vzdsk
  SdFat* sd_prt = vzdsk->get_sd();
  if (sd_prt->fatType() <=32) {
    serial_log(PSTR("Volume is FAT%d\r\n"), sd_prt->fatType());
  } else {
    serial_log(PSTR("Volume is exFAT\r\n"));
  }

  File root = sd_prt->open("/");
  while(true) {
    File entry =  root.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    if (!entry.isDirectory()) {
      if (entry.size() >=90000 && entry.size() <=100000) {
      //if (entry.size() == 99184 || entry.size() == 99200) {
        char filename[FILENAME_MAX];
        entry.getName(filename, FILENAME_MAX);
        serial_log(PSTR("%s\t\t\t%ld\r\n"), filename, entry.size());
      }
    }
  }  
  
  //List file not hidden, ends with DSK
}

void mount_image(char* filename)
{
  delete vzdsk;
  memset(diskimage, 0, sizeof(diskimage));
  strncpy(diskimage, filename, strlen(filename));
  vzdsk = new vzdisk();
  vzdsk->Open(diskimage);
}

void img_catalog()
{
  if (vzdsk->get_sector(0,0)) {
    sector_t *sector_ptr = (sector_t *)fdc_sector;
    uint8_t *data_ptr = sector_ptr->data_bytes;
    for(int i=0; i<16; i++) {
      serial_log(PSTR("%02X "), *(data_ptr+i));
    }

    serial_log(PSTR("\r\n"));
  }
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
    else if (strncmp_P(cmd, PSTR("mount "), 6)==0) {
      //TODO parse filename
      mount_image(cmd+6);
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
