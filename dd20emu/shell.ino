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
#include "vzdisk.h"
#include "wrdata.h"
#define TEMPBUFFER_SIZE 80

extern const uint8_t inversed_sec_interleave[SEC_NUM];

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
      else if( i==8 || i==0x7f)
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
  serial_log(PSTR("dumpsect n s - Dump sector on track n, sector s\r\n"));
  serial_log(PSTR("trklist - Dump track offsets\r\n"));
  serial_log(PSTR("sectormap - Dump sector map\r\n"));
  serial_log(PSTR("scandisk - Scan all sectors from all tracks\r\n"));
  serial_log(PSTR("wrprot [1/0]- Enable write protect\r\n"));
  serial_log(PSTR("wrbuf - Dump wrdata buf\r\n"));
  serial_log(PSTR("exit - Exit from shell, resume emulator\r\n"));
}

void print_status()
{
  serial_log(PSTR("\r\nVTech DD20 emulator, v0.0.b, 02/12/2023\r\n"));
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
  serial_log(PSTR("Write protection: %d\r\n"), write_protect);
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
    //TODO: Check file extension, file size
    if (!entry.isDirectory()) {
      if (entry.size() >=98560 && entry.size() <=99200) {
        char filename[FILENAME_MAX];
        entry.getName(filename, FILENAME_MAX);
        serial_log(PSTR("%s"), filename);
//        for(int i=0; i<FILENAME_MAX-strnlen(filename, FILENAME_MAX); i++)
//          serial_log(PSTR("%c"), ' ');
          
        serial_log(PSTR("\t%ld\r\n"), entry.size());
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
  /* DVZ VZ300 DOS FLOPPY FORMAT.doc is not accurate */
  /* the last 2 bytes of file entry is end addr, not file size */
  
  serial_log(PSTR("T  Name         TR SC Start  End\r\n"));
  serial_log(PSTR("--------------------------------\r\n"));
  for(uint8_t i=0; i<15; i++) {
    bool found = vzdsk->get_sector(0, pgm_read_byte_near(&inversed_sec_interleave[i]));

    if (found) {
      uint8_t* data_ptr = ((sector_t*)fdc_sector)->data_bytes;
      for(uint8_t j=0; j<8; j++) {
        catalog_entry* entry_ptr = (catalog_entry*)(data_ptr + j*16);
        if (entry_ptr->type == 0)
          return;
        serial_log(PSTR("%c  "), entry_ptr->type);
        for(uint8_t k=0; k<sizeof(entry_ptr->filename); k++) {
          serial_log(PSTR("%c"), entry_ptr->filename[k]);
        }
        serial_log(PSTR("\t%02d %02d %04X   %04X\r\n"), entry_ptr->tr, entry_ptr->sec, entry_ptr->start_addr, entry_ptr->end_addr);
      }
    }
  }
  
  if (vzdsk->get_sector(0,0)) {
    sector_t *sector_ptr = (sector_t *)fdc_sector;
    uint8_t *data_ptr = sector_ptr->data_bytes;
    for(int i=0; i<16; i++) {
      serial_log(PSTR("%02X "), *(data_ptr+i));
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

    serial_log(PSTR("\r\n"));
  }
}

void trklist()
{
  for(uint8_t i=0; i<TRK_NUM; i++) {
    serial_log(PSTR("TR:%02d - %08lX\r\n"), i, vzdsk->get_track_offset(i));
  }  
}

void sectormap()
{
  serial_log(PSTR("Sector map:"));
  bool found = vzdsk->get_sector(0, pgm_read_byte_near(&inversed_sec_interleave[15]) ); //Sector 15 has the sector map table 
  if (!found) {
    serial_log(PSTR("\r\nNot found.\r\n"));
    return;
  }
  for(uint8_t i=0; i<78; i++) {
    if (i%2==0) {
      serial_log(PSTR("\r\n%02d: "), i/2);
    }
    uint8_t value = fdc_sector[sizeof(sec_hdr_t)+i];
    for(uint8_t j=0; j<8; j++) {
      if (bitmask[j] & value) {
        serial_log(PSTR("x "));
      }
      else {
        serial_log(PSTR(". "));
      }
    }
  }
  serial_log(PSTR("\r\n"));
}
void scandisk()
{
  serial_log(PSTR("Sequential scanning\r\n"));
  for(uint8_t i=0; i<TRK_NUM; i++) {
    for(uint8_t j=0; j<SEC_NUM; j++) {
      serial_log(PSTR("\rTR:%d, SEC:%d"), i, j);
      bool found = vzdsk->get_sector(i,j);
      if (!found) {
        serial_log(PSTR("\r\nNot found:%d, %d\r\n"), i,j);
      }
    }
  }

  randomSeed(analogRead(0));

  serial_log(PSTR("\r\n100 random track scanning\r\n"));
  for(int i=0; i<100; i++) {
    uint8_t tr = random(0, TRK_NUM);
    for(uint8_t j=0; j<SEC_NUM; j++) {
      serial_log(PSTR("\r#%03d, TR:%02d, SEC:%02d"), i+1, tr,j);
      bool found = vzdsk->get_sector(tr,j);
      if (!found) {
        serial_log(PSTR("\r\nNot found:TR:%d, SEC:%d\r\n"), tr,j);
      }
    }
  }

  serial_log(PSTR("\r\n200 random sector scanning\r\n"));
  for(int i=0; i<200; i++) {
    uint8_t tr = random(0, TRK_NUM);
    uint8_t sec = random(0, SEC_NUM);
    serial_log(PSTR("\r#%03d, TR:%02d, SEC:%02d"),i+1, tr,sec);
    bool found = vzdsk->get_sector(tr, sec);
    if (!found) {
        serial_log(PSTR("\r\nNot found:%d, %d\r\n"), tr, sec);     
    }
  }

  serial_log(PSTR("\r\nEnd of scandisk\r\n"));
}

void wrprot(char* flag)
{
  if (flag[0] == '1') {
    write_protect = true;
    digitalWrite(wrProtPin, HIGH);
    serial_log(PSTR("Write protected\r\n"));
  }
  else if (flag[0] == '0') {
    write_protect = false;
    digitalWrite(wrProtPin, LOW);
    serial_log(PSTR("Write permitted\r\n"));
  }
  else
    serial_log(PSTR("wrprot 1 or 0\r\n"));
}

extern unsigned int wr_buf[WRBUF_SIZE];
void wrbuf()
{
  for(uint16_t i=0; i < WRBUF_SIZE; i++) {
    if (i%16 == 0) {
      serial_log(PSTR("\r\n"));
    }
    serial_log(PSTR("%04X "), wr_buf[i]);
  }

  serial_log(PSTR("\r\n"));
}

void handle_shell()
{

  int i = Serial.read();
  if (i!=0x0d && i!=0x0a) {
    return;
  }
  
  bool in_shell = true;
  byte *tempbuffer = new byte(TEMPBUFFER_SIZE);

  serial_log(PSTR("DD-20 emulator command shell.\r\n"));
  serial_log(PSTR("Emulator is PAUSED.\r\n"));
  serial_log(PSTR("type ?/help for help screen.\r\n"));

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

    //dumpsect
    else if (strncmp_P(cmd, PSTR("dumpsect "), 9)==0) {
      int n=-1,s=-1;
      sscanf(cmd+9, "%d %d", &n,&s);
      if (n!=-1 && s!=-1) {
        dump_sector(n, s);
      } else
        serial_log(PSTR("dumpsect n s\r\n"));
    }

    //trklist
    else if (strncmp_P(cmd, PSTR("trklist"), 7)==0) {
      trklist();
    }

    //sectormap
    else if (strncmp_P(cmd, PSTR("sectormap"), 9)==0) {
      sectormap();  
    }
    
    //scandisk
    else if (strncmp_P(cmd, PSTR("scandisk"), 8)==0) {
      scandisk();
    }

    //wrprot [1/0]
    else if (strncmp_P(cmd, PSTR("wrprot "), 7)==0) {
      wrprot(cmd+7);
    }

    //wrbuf
    else if (strncmp_P(cmd, PSTR("wrbuf"), 5)==0) {
      wrbuf();
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

  delete tempbuffer;
}
