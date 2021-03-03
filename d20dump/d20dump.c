#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

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
const uint8_t reversed_index_sec_interleave[SEC_NUM] = {0, 3, 6, 9, 12, 15, 2, 5, 8, 11, 14, 1, 4, 7, 10, 13};

/*40 tracks, 8 bytes/track packed*/
uint8_t sec_lut[TRK_NUM][SEC_NUM] = { 0 };

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
    else if (fsize == 99184 || fsize == 99200) {
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
        for(int i=0; i<PADDING_SIZE; i++) {
            if (padding_bytes[i] != 0) {
                padding_found = false;
                break;
            }
        }
        if (padding_found == true) {
            padding = 16;
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
    int gap1_size = -1;

    //Sync bytes
    if (*buf++ == 0x80 && *buf++ == 0x80 && *buf++ == 0x80 && *buf++ == 0x80 && *buf++ == 0x80)
    {
        if (*buf == 0x80) {
            buf++;
            if (*buf++ == 0x00) {
                gap1_size = 7;
            }
        } else if (*buf++ == 0x00) {
            gap1_size = 6;
        }
        //IDAM
        if (*buf++ == 0xFE && *buf++ == 0xE7 && *buf++ == 0x18 && *buf++ == 0xC3) {
            *TR = *buf++;
            *SEC = *buf;
        }
    }

    return gap1_size;
}

#define LUT_SCAN_STEP   13
int build_sector_lut(FILE *fp, long int fsize)
{
    uint8_t buf[LUT_SCAN_STEP] = {0};
    long int offset = 0;
    int sectors = 0;
    uint8_t TR, SEC;

    printf("Start building sector LUT.\r\n");

    while (offset < fsize-LUT_SCAN_STEP) {
        //printf("seek:%04X\r\n", offset);
        if (fseek(fp, offset, SEEK_SET) != 0 ||fread(buf, sizeof(buf), 1, fp) != 1) {
            printf("fseek/fead failed while scanning sectors\r\n");
            break;
        }

        int gap1_size = sync_gap1(buf, &TR, &SEC);
        if (gap1_size != -1) {
            uint8_t SEC_IDX = reversed_index_sec_interleave[SEC];
            sec_lut[TR][SEC_IDX] = offset - TR*(TRKSIZE_VZ+padding) - SEC_IDX*SECSIZE_VZ;

            //7 bytes GAP1, 4 bytes IDAM leading, 1 byte TR, 1 byte SEC
            //6 bytes GAP1, 4 bytes IDAM leading, 1 byte TR, 1 byte SEC   
            //printf("offset=%04lX, ", offset);
            offset += (gap1_size + 4 + 1 + 1 + SEC_REMAINING);
            sectors++;
            //printf("%d bytes sync : TR: %d, SEC: %d, \tnext offset to read:%04lX\r\n", gap1_size, TR, SEC, offset);

            //if this is the last sector of a track
            if (SEC == sector_interleave[SEC_NUM-1] && padding!=0) {
                offset += padding;
                //printf("This is the last sector of TR:%d, moving to next track with padding=%d\r\n\r\n", TR, padding);
            }            
        }
        else {
            //printf("............Searching sync bytes, skipped 1 byte at %04lX, ", offset);
            offset++;
            //printf("\t\tnext offset to read:%04lX\r\n", offset);
        }
    }

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
            uint32_t offset = sec_lut[i][j] + i*(TRKSIZE_VZ+padding) + j*SECSIZE_VZ;
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

int catalog(FILE* fp)
{
    int result = -1;
    return result;
}

int main(int argc, char *argv[])
{
    if (argc == 2) {
        printf("Laser310 DD20 disk image dump: '%s'\r\n", argv[1]);

        FILE *fp = fopen(argv[1], "rb");
        if (fp != NULL) {
            long int fsize = check_disk_type(fp);
            int total = build_sector_lut(fp, fsize);
            printf("%d sectors found!\r\n", total);

            if (total == 640) {
                if (validate_sector_lut(fp) == -1 )
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