#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/*
if lut value is 0, the sector size could be 154 or 155
    155 : TR0 to TR6 are used in hello1.dsk, SEC#0 of 0~6 track of hello1.dsk, 
            this is caused by 7 bytes(80 80 80 80 80 80 00) GAP1 and 7 bytes(80 80 80 80 80 80 00) of GAP2 leading
        CORRECT GAP1, WRONG GAP2(+1)
        
    154 : Each un-used sector which lut is 0, the GAP1 is always 7 bytes(80 80 80 80 80 80 00), GAP2 leading is 
            always 6 bytes(80 80 80 80 80 00)
        CORRECT GAP1, CORRECT GAP2

Algorithm : Output everything; report actual size
    use read buffer size 155, 
    read [current_offset, next_offset),
    return next_offset-current_offset to get_sector() caller

if lut value is 1, the sector size could be 154 or 153
    154 : TR:39, SEC#1 of hello,dsk(starting 0x17B9F) is a 154 bytes sector.
            this is caused by 6 bytes(80 80 80 80 80 00) of GAP1, 7 bytes(80 80 80 80 80 80 00) of GAP2 leading.
        WRONG GAP1(-1), WRONG GAP2(+1)

    153 : TR:39, SEC#11 of hello.dsk(starting 0x17A6B) is a 153 bytes sector. 
            6 bytes(80 80 80 80 80 00) of GAP1, 6 bytes(80 80 80 80 80 00) of GAP2 leading.
        WRONG GAP1(-1), CORRECT GAP2

Algorithm : add 0x80 to the front, report actual size + 1
    use read buffer size 155
    *buf_ptr++ = 0x80;
    read [current_offset, next_offset) to buf_ptr,
    return next_offset-current_offset+1 to get_sector() caller

*/

//Arduino types to std types
#define uint8_t     u_int8_t
#define uint16_t    u_int16_t
#define uint32_t    u_int32_t

#include "d20dump.h"

uint8_t padding = 0;

//used in get_sector()
const uint8_t sector_interleave[SEC_NUM] = { 0, 11, 6, 1, 12, 7, 2, 13, 8, 3, 14, 9, 4, 15, 10, 5 };

// Used in build_sector_lut(), we need to store the reverse indexed offset in lut
// For instance, #5 is the last sector of a track. The value of inversed_sec_interleave[5] is 15, 
// the lut value of #5 will be stored in sec_lut[TRK][15]; Another example is #10 sector is the second 
// last sector of a track. The value of inversed_sec_interleave[10] is 14, the lut value of #10 will be
// stored in sec_lut[TRK][14]
// Thus during play back, in the for loop of [0, 15], sectors will be played back at the correct interleaved 
// order of 0, 11, 6, 1, 12, 7, 2
const uint8_t inversed_sec_interleave[SEC_NUM] = {0, 3, 6, 9, 12, 15, 2, 5, 8, 11, 14, 1, 4, 7, 10, 13};

/*40 tracks, 8 bytes/track packed*/
// 0 : Spec sector with 7 sync bytes
// 1 : Sector with 6 sync bytes
// 2 : Sectors failed to locate
int8_t sec_lut[TRK_NUM][SEC_NUM/2] = { 0 };

void fill_sec_lut()
{
    for(int i=0; i<TRK_NUM; i++)
        for(int j=0; j<SEC_NUM/2; j++)
            sec_lut[i][j] = 2;
}

/**
 *  return:
 *  -1 : Unrecognized format
 *  n  : file size 
 * 
 */
int check_disk_type(FILE *fp)
{
    int result = -1;

    if (fp == NULL)
        return result;

    fseek(fp, 0L, SEEK_END);
    long int fsize = ftell(fp);

    if (fsize == 98560) {
        padding = 0;
        printf("Type 1 format, 0 padding.\r\n");
        result = fsize;
    }
    else if (fsize >= 99184 || fsize == 99200) {
        //confirm padding bytes are all '0'
        if (fseek(fp, TRKSIZE_VZ, SEEK_SET) != 0) {
            return result;
        }

        uint8_t padding_bytes[PADDING_SIZE];
        size_t t = fread(padding_bytes, sizeof(padding_bytes), 1, fp);
        if (t != 1) {
            return result;
        }

        bool padding_found = true; 
        for(int i=0; i<sizeof(padding_bytes); i++) {
            if (padding_bytes[i] != 0) {
                padding_found = false;
                break;
            }
        }
        if (padding_found == true) {
            padding = PADDING_SIZE;
        }
        printf("Type 2/3 format, padding = %d bytes\r\n", padding);
        result = fsize;
    }
    else {
        printf("Unrecognized disk format.\r\n");
    }
    return result;
}

int sync_gap1(uint8_t* buf, uint8_t* TR, uint8_t* SEC)
{
    int result = -1;

    //Sync bytes
    if (*buf++ == 0x80 && *buf++ == 0x80 && *buf++ == 0x80 && *buf++ == 0x80 && *buf++ == 0x80)
    {
        int gap1_size = -1;
        if (*buf == 0x80) {
            buf++;
            if (*buf++ == 0x00) {
                gap1_size = 7;
            }
        } else if (*buf++ == 0x00) {
            gap1_size = 6;
        }

        if (gap1_size != -1) {
            //IDAM
            if (*buf++ == 0xFE && *buf++ == 0xE7 && *buf++ == 0x18 && *buf++ == 0xC3) {
                *TR = *buf++;
                *SEC = *buf++;
                if ((*TR+*SEC) == *buf++) {
                    //T+S checksum is invalid

                    //GAP2
                    if (*buf++ == 0x80 && *buf++ == 0x80 && *buf++ == 0x80 && *buf++ == 0x80 && *buf++ == 0x80)
                        result = gap1_size;
                }
            }
        }
    }

    return result;
}

int sync_gap2(uint8_t* buf)
{
    int result = -1;
    //Sync bytes
    if (*buf++ == 0x80 && *buf++ == 0x80 && *buf++ == 0x80 && *buf++ == 0x80 && *buf++ == 0x80)
    {
        int gap1_size = -1;
        if (*buf == 0x80) {
            buf++;
            if (*buf++ == 0x00) {
                gap1_size = 7;
            }
        } else if (*buf++ == 0x00) {
            gap1_size = 6;
        }
    }
    return result;
}

int build_sector_lut(FILE *fp, long int fsize)
{
    uint8_t buf[LUT_SCAN_STEP] = {0};
    long int offset = 0;
    int sectors = 0;
    uint8_t TR, SEC;
    int debug = 0;

    printf("Start building sector LUT.\r\n");

    while (offset < fsize-LUT_SCAN_STEP) {
        //printf("seek:%04X\r\n", offset);
        if (fseek(fp, offset, SEEK_SET) != 0 ||fread(buf, sizeof(buf), 1, fp) != 1) {
            printf("fseek/fead failed while scanning sectors\r\n");
            break;
        }

        // if (offset>32000)
        //     return -1;
        //if (offset>0x17980)
        //    debug = 1;
        int gap1_size = sync_gap1(buf, &TR, &SEC);
        if (gap1_size != -1) {
            uint8_t SEC_IDX = inversed_sec_interleave[SEC];
            uint8_t lut = offset - TR*(TRKSIZE_VZ+padding) - SEC_IDX*SECSIZE_VZ;
            if (SEC_IDX%2==0) {
                //high nibble
                sec_lut[TR][SEC_IDX/2] = lut << 4;
            }
            else {
                //low nibble
                sec_lut[TR][SEC_IDX/2] = sec_lut[TR][SEC_IDX/2] | lut;
            }

            if (debug==1)
                printf("TR:%d, SEC_IDX:%d, value:%d\r\n", TR, SEC_IDX, SEC_IDX%2==0 ? (sec_lut[TR][SEC_IDX/2] >>4) : (sec_lut[TR][SEC_IDX/2] & 0x0F));

            //7 bytes GAP1, 4 bytes IDAM leading, 1 byte TR, 1 byte SEC
            //6 bytes GAP1, 4 bytes IDAM leading, 1 byte TR, 1 byte SEC   
            if (debug==1)
                printf("offset=%04lX, ", offset);
            offset += (gap1_size + 4 + 1 + 1 + SEC_REMAINING);
            sectors++;
            if (debug==1)
                printf("%d bytes sync : TR: %d, SEC: %d, \tnext offset to read:%04lX\r\n", gap1_size, TR, SEC, offset);

            //if this is the last sector of a track
            if (SEC == sector_interleave[SEC_NUM-1] && padding!=0) {
                offset += padding;
                //printf("This is the last sector of TR:%d, moving to next track with padding=%d\r\n\r\n", TR, padding);
            }            
        }
        else {
            if (debug==1)
                printf(".....Syncing, skipped byte[%02X] at %04lX, ", buf[0], offset);
            offset++;
            if (debug==1)
                printf("\t\tnext offset to read:%04lX\r\n", offset);
        }
    }

#if 1 //Dump Sector LUT
    for(int i=0; i<TRK_NUM; i++)
    {
        printf("TR:%02d  ",i);
        for(int j=0; j<SEC_NUM/2; j++)
        {
            printf("%02X ", sec_lut[i][j]);
        }
        printf("\r\n");
    }
#endif

    return sectors;
}

int validate_sector_lut(FILE *fp)
{
    int result = -1;
    uint8_t TR, SEC;
    uint8_t buf[LUT_SCAN_STEP] = {0};

    for(int i=0; i<TRK_NUM; i++)
    {
        for(int j=0; j<SEC_NUM; j++)
        {
            uint8_t value = j%2==0 ? (sec_lut[i][j/2] >>4) : (sec_lut[i][j/2] & 0x0F);
            uint32_t offset = value + i*(TRKSIZE_VZ+padding) + j*SECSIZE_VZ;
            //printf("n=%d, s=%d, sec_lut[i][j/2]=%02X, value=%d, offset=%04X\r\n", i,j, sec_lut[i][j/2], value, offset);
            fseek(fp, offset, SEEK_SET);
            if (fread(buf, sizeof(buf), 1, fp) != 1) {
                printf("fread failed\r\n");
                return result;
            }
            int gap1_size = sync_gap1(buf, &TR, &SEC);
            if (sync_gap1(buf, &TR, &SEC) ==-1)
            {
              printf("check_gap1 failed at [%d][%d]\r\n", i,j);
              return result;
            }

            if (TR!=i || SEC !=sector_interleave[j])
            {
                printf("scan_sctor_lut failed at TR=%d, SEC=%d\r\n", TR, SEC);
                return result;
            }
        }
    }
    result = 1;

    printf("All sector offsets in lut are validated.\r\n");
    return result;
}

#define SECTOR_MAP_TR   0
#define SECTOR_MAP_SEC  15
#define SECTOR_MAP_LENGTH   78
int dump_sector_map(FILE* fp) {
    int result = -1;
/*
    uint8_t buf[SECSIZE_VZ] = {0};

    uint8_t lut_idx = inversed_sec_interleave[SECTOR_MAP_SEC];
    uint32_t offset = sec_lut[SECTOR_MAP_TR][lut_idx] + SECTOR_MAP_TR*(TRKSIZE_VZ+padding) + lut_idx*SECSIZE_VZ;

    fseek(fp, offset, SEEK_SET);
    if (fread(buf, sizeof(buf), 1, fp) != 1) {
        printf("fread failed\r\n");
        return result;
    }

    printf("Sector map dump at offset :%04X\r\n", (uint32_t)offset);
    printf("Note that the sector map does not include track 0 and the sector maps bits are not arranged out of order.\r\n");
    printf("The lut value of TR:%d, SEC:%d is %d\r\n", SECTOR_MAP_TR, SECTOR_MAP_SEC, sec_lut[SECTOR_MAP_TR][lut_idx]);
    for(int i=1; i<TRK_NUM; i++) {
        printf("TR:%02d : ", i);
        uint8_t* data_ptr = ((sector_t*)buf)->data_bytes;
        uint8_t low_byte = *(data_ptr+(i-1)*2+0);
        uint8_t high_byte = *(data_ptr+(i-1)*2+1);
        printf("%02X %02X: ", low_byte, high_byte);
        for(int j=0; j<7; j++) {
            printf("%02X ", ((sector_t*)buf)->header.GAP2[j]);
        }
        printf("\r\n");
    }
    printf("\r\n");

    printf("\r\nEnd of sector map dump\r\n");*/
    return result;
}

int catalog(FILE* fp)
{
    int result = -1;
    return result;
}

int main(int argc, char *argv[])
{
    if (argc == 2) {
        printf("Laser310 DD20 disk image dump: '%s'\r\n", argv[1]);
        printf("SECSIZE_VZ=%d, sizeof(sector_t)=%d\r\n", SECSIZE_VZ, (int)sizeof(sector_t));

        FILE *fp = fopen(argv[1], "rb");
        if (fp != NULL) {
            fill_sec_lut();
            long int fsize = check_disk_type(fp);
            int total = build_sector_lut(fp, fsize);
            printf("%d sectors found!\r\n", total);

            if (total == 640) {
                if (validate_sector_lut(fp) == -1 )
                {
                    fclose(fp);
                    return -1;
                }

                if (dump_sector_map(fp) == -1)
                {
                    fclose(fp);
                    return -1;
                }
                if (catalog(fp) == -1)
                {
                    fclose(fp);
                    return -1;
                }
            }
            fclose(fp);
        }
        else {
            printf("Failed to open %s\r\n", argv[1]);
        }
    }
    else {
        printf("d20dump image.dsk\r\n");
    }
}