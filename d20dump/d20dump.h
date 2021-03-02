#ifndef __D20DUMP_H__
#define __D20DUMP_H__

//These numbers are spec numbers

#define TRK_NUM         40
#define SEC_NUM         16
#define SECSIZE_VZ      154
#define TRKSIZE_VZ  SECSIZE_VZ * SEC_NUM        //2464
#define PADDING_SIZE    16
//define    TRKSIZE_VZ_PADDED   TRKSIZE_VZ + 16

//Since a lot of the sectors in various formats of VZ disk images have short sync bytes ( 5x80h, 00h ), the normalized sector header is
//defined as short sync bytes

typedef struct SectorHeader {
    /* Normalized means if the GAP1 is 7 bytes, the first byte is skipped, only 6 bytes are sent to the output */
#ifdef  NORMALIZED_SECTOR_HDR
    //some disk images have 6 byets GAP1
    uint8_t     GAP1[6];        //0x80 5 times, then 0x00
#else
    /* spec GAP1 */
    uint8_t     GAP1[7];        //0x80 6 times, then 0x00
#endif
    uint8_t     IDAM_leading[4];//FE, E7, 18, C3
    uint8_t     TR;
    uint8_t     SC;
    uint8_t     TS_sum;
    uint8_t     GAP2[6];        //0x80 5 times, 0x00
    uint8_t     IDAM_closing[4];//C3, 18, E7, FE
} sec_hdr_t;

//defines the remaining # of bytes after SC field in sector header
#define SEC_HDR_REMAINING   sizeof((sec_hdr_t*)0)->TS_sum + sizeof((sec_hdr_t*)0)->GAP2 + sizeof((sec_hdr_t*)0)->IDAM_closing

typedef struct Sector {
    sec_hdr_t   header;
    uint8_t     data_bytes[126];
    uint8_t     next_track;
    uint8_t     next_sector;
    uint16_t    checksum;
} sector_t;

//defines the remaining # of bytes after sec_hdr_t.SC field in a sector
#define SEC_REMAINING       SEC_HDR_REMAINING + sizeof((sector_t*)0)->data_bytes + sizeof((sector_t*)0)->next_track + \
                            sizeof((sector_t*)0)->next_sector + sizeof((sector_t*)0)->checksum

typedef struct Track {
    sector_t    sector[SEC_NUM];
} track_t;

#endif  //__D20DUMP_H_