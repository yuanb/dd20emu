/*
 * dd20emu main file
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/*
 * Port 10h			Latch(write-only)
 *	11h			DATA(read-only)
 *	12h			Pooling(read-only)
 *	13h			Write protection status(read-only)
 */

#define	TRK_NUM		40
#define	SEC_NUM		16
#define	SECSIZE_VZ	154
#define TRKSIZE_VZ	SECSIZE_VZ * SEC_NUM

#define TRKSIZE_FM      3172    /* size of a standard FM mode track */

typedef struct SectorHeaders {
	uint8_t		GAP1[7];
	uint8_t		IDAM_leading[4];
	uint8_t		TR;
	uint8_t		SC;
	uint8_t		TS_sum;
	uint8_t		GAP2[6];
	uint8_t		IDAM_closing[4];
} sec_hdr_t;

typedef struct Sectors {
	sec_hdr_t	header;
	uint8_t		data_bytes[126];
	uint8_t		next_track;
	uint8_t		next_sector;
	uint16_t	checksum;
} sector_t;

typedef struct Tracks {
	sector_t	sector[40];
} track_t;

uint8_t fdc_data[TRKSIZE_VZ];
uint8_t	fm_track_data[TRKSIZE_FM];

int get_track(FILE *fp, int n)
{
	int size = TRKSIZE_VZ;
	int offset = TRKSIZE_VZ * n;
	int r = fseek(fp, offset, SEEK_SET);

	if (r==0)
	{
		printf("seek to: %d\r\n", offset);
	}

	r = fread(&fdc_data, size, 1, fp);
	printf("T:%02d, size=%d\r\n", n, size);

	for(int i=0; i<SEC_NUM; i++)
	{
		sector_t *sec = (sector_t *)&fdc_data[i* sizeof(sector_t)];
		sec_hdr_t *hdr = &sec->header;


		printf("Sector # %02x", i);
		printf("\r\ngap1: ");
		for(int j=0; j<sizeof(hdr->GAP1); j++)
		{
			printf("%02X ", hdr->GAP1[j]);
		}

		printf("\r\nidam_leading: ");
		for(int j=0; j<sizeof(hdr->IDAM_leading); j++)
		{
			printf("%02X ", hdr->IDAM_leading[j]);
		}

		printf("\r\nTR: %02X, SC: %02X, T+S: %02X", hdr->TR, hdr->SC, hdr->TS_sum);

		printf("\r\ngap2: ");
		for(int j=0; j<sizeof(hdr->GAP2); j++)
		{
			printf("%02X ", hdr->GAP2[j]);
		}


		printf("\r\nIDAM_closing: ");
		for(int j=0; j<sizeof(hdr->IDAM_closing); j++)
		{
			printf("%02X ", hdr->IDAM_closing[j]);
		}

		printf("\r\ndata:\r\n");
		for(int j=0; j<sizeof(sec->data_bytes); j++)
		{
			printf("%02X ", sec->data_bytes[j]);
		}

		if (n!=0)
		{
			printf("\r\nNext TR: %02X, SC: %02X", sec->next_track, sec->next_sector);
		}
		else
		{
			printf("%02x %02X", sec->next_track, sec->next_sector);
		}

		uint16_t checksum = 0;
		for(int j=0; j<sizeof(sec->data_bytes); j++)
		{
			checksum += sec->data_bytes[j];
		}
		checksum += sec->next_track;
		checksum += sec->next_sector;
		if (sec->checksum != checksum)
		{
			printf("\r\nCHECKSUM ERROR");
			exit(-1);
		}

		printf("\r\nVerified checksum=: %04X", checksum);

		printf("\r\n\r\n");
	}

	return 0;
}

int put_track()
{

}

int main ()
{
	FILE *fp = fopen("floppy1.dsk", "rb+");

	if (fp == NULL)
	{
		printf("Unable to open floppy1.dsk\r\n");
		exit(-1);
	}

	for(int i=0; i<TRK_NUM; i++)
	{
		get_track(fp, i);
	}


	printf("End of disk dump\r\n");
	fclose(fp);
	exit(0);
}
