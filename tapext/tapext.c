#include <stdio.h>
#include <stdlib.h>

#define ROM_S	0x2E
#define ROM_M	0x40
#define ROM_L	0x54

#define SPLIT_L	((ROM_S+ROM_M)/2)
#define SPLIT_H	((ROM_M+ROM_L)/2)

#define PILOTLEN 40

#define LEADIN   0x02


int fetchbyte(FILE *fpin)
{
	int byte;

	if ((byte=fgetc(fpin)) == EOF)
		exit(4);
	else return(byte);
}


void findstart(FILE *fpin)
{
	int pilot, byte;
	int found;
	
	for (found = 0; found == 0; )
	{
		for (pilot = 0; pilot < PILOTLEN; pilot++)
		{
			byte = fetchbyte(fpin);
			if (byte > SPLIT_L)
				pilot = 0;
		}

		while ((byte = fetchbyte(fpin)) < SPLIT_L);

		if (byte < SPLIT_H) continue;
		byte = fetchbyte(fpin);
		if ((byte < SPLIT_L) || (byte >  SPLIT_H)) continue;
		found = 1;
	}
	return;
}


int freadbit(FILE *fpin)
{
	int puls1, puls2;

	puls1 = fetchbyte(fpin);
	puls2 = fetchbyte(fpin);
	if ((puls1 < SPLIT_L) && (puls2 > SPLIT_L) &&  (puls2 < SPLIT_H))
		return (0);
	else if ((puls1 > SPLIT_L) &&  (puls1 < SPLIT_H) && (puls2 < SPLIT_L))
		return (1);
	else return (-1); /* illegal pulse combination */
}


int checksync(FILE *fpin)
{
	int byte;

	byte = fetchbyte(fpin);
	if (byte < SPLIT_H) return (0);
	byte = fetchbyte(fpin);
	if ((byte < SPLIT_L) || (byte >  SPLIT_H)) return (0);
	return (1);
}


int freadbyte(FILE *fpin, int *byte, int *checksum)
{
	int bit, input, check;

	check = 1; /* initial value for checksum */

	for (bit = 0, *byte = 0; bit < 8; bit++)
	{
		(*byte) /= 2;
		input = freadbit(fpin);
		if (input < 0) return (0); /* error reading byte */
		
		(*byte) += input ? 0x80 : 0x00;
		check ^= input;
	}
	input = freadbit(fpin);
	if (check != input)
		*checksum = 0;	/* checksum doesn't match */
	else
		return (1);	/* checkum OK */
}

void usage(void)
{
	fprintf(stderr, "usage: tapext <.tap file>\n");
	exit(1);
}
	
void main(int argc, char **argv)
{
	FILE *fpin, *fpout;
	unsigned long startadr, stopadr, adr;
	unsigned int byte;
	int fileno, check;
	int sync;
	int skip;
	char outname[80];
	unsigned long start, stop;
	int found;
	int c;
	int isheader, isfirst;
	int id;
	char filename[80];
	int i;
	int num;

	if (argc < 2)
		usage();
	if ((fpin=fopen(argv[1],"rb")) == NULL)
	{
		fprintf(stderr, "Couldn't open input file %s\n", argv[1]);
		exit(2);
	}


	num = 0;
	found = 0;
	fileno = 1;
	isheader = 1;
	while(1)
	{
		c= 1;
		findstart(fpin);
		printf("foundsync");

		for  (sync = 0x09; sync >= 0x01; sync--)
		{
			if (!freadbyte(fpin, &byte, &c)) break;
			printf("%02x ", byte);
			if ((byte & 0x7f) != sync) break;
			if (!checksync(fpin)) break;
		}
		if (sync != 0x00) continue;

		if (byte == 0x81)
			isfirst = 1; 
		else
			isfirst = 0;


		if (isheader)
		{
			if (!freadbyte(fpin, &byte, &c)) continue;
			if (!checksync(fpin)) continue;
			id = byte;
			printf("ID: %02x\n", id);
			if (!freadbyte(fpin, &byte, &c)) continue;
			if (!checksync(fpin)) continue;
			start = byte;
			if (!freadbyte(fpin, &byte, &c)) continue;
			if (!checksync(fpin)) continue;
			start += (byte * 256);
			if (!freadbyte(fpin, &byte, &c)) continue;
			if (!checksync(fpin)) continue;
			stop = byte;
			if (!freadbyte(fpin, &byte, &c)) continue;
			if (!checksync(fpin)) continue;
			stop += (byte * 256);
			for (i = 0; i < 16; i++)
			{
				if (!freadbyte(fpin, &byte, &c)) break;
				if (!checksync(fpin)) break;
				if (isalnum(byte))
					filename[i] = byte;
				else
					filename[i] = '_';
			}
			filename[16] = '\0';
			printf("start: %04x\n", start);
			printf("stop:  %04x\n", stop);
			printf("name:  %s\n", filename);
		}
		else /* body of file */
		{
			filename[7] = '\0'; /* restrict to DOS 8 char filenames */
			sprintf(outname,"%s%d.prg",filename, num++);
			if ((fpout=fopen(outname,"wb")) == NULL)
			{
				fprintf(stderr, "Couldn't open output file %s\n", outname);
				exit(3);
			}
			printf("writing file  %s, ", outname);

			fputc((start & 0xff), fpout);
			fputc((start / 0x100), fpout);
			for (adr = start; adr < stop; adr++)
			{
				if (!freadbyte(fpin, &byte, &c)) break;
				if (!checksync(fpin)) break;
				fputc(byte, fpout);
			}
			fclose(fpout);
			printf("checksum %s\n", c ? "OK" : "BAD!");
		}

		if (isfirst == 0)
			isheader ^= 1;

		fclose(fpout);
	}
	fclose(fpin);
	exit(0);
}
