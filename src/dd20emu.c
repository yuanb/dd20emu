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

typedef struct Headers {
	uint8_t		GAP1[7];
	uint8_t		IDAM_leading[4];
	uint8_t		TR;
	uint8_t		SC;
	uint8_t		TS_sum;
	uint8_t		GAP2[6];
	uint8_t		IDAM_closing[4];
} header_t;

typedef struct Sectors {
	header_t	header;
	uint8_t		data_bytes[126];
	uint8_t		next_track;
	uint8_t 	next_sector;
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

	printf("Header dump:\r\n");
	for(int i=0; i<3; i++)
	{
		for(int j=0; j<8; j++)
		{
			printf("%02X ", fdc_data[i*8+j]);
		}
		printf("\r\n");
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

	for(int i=0; i<40; i++)
	{
		get_track(fp, i);
	}

	fclose(fp);
	exit(0);
}
