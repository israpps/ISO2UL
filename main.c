//#include "stdafx.h"
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>

#if defined(_WIN32) || defined(WIN32)
#include <io.h> //TODO: check if it's necesary for the code to work
#endif


#define UL_GAME_NAME_MAX       32
#define ISO_GAME_NAME_MAX      64
#define ISO_GAME_EXTENSION_MAX 4
#define GAME_STARTUP_MAX       12

#define ISO_GAME_FNAME_MAX (ISO_GAME_NAME_MAX + ISO_GAME_EXTENSION_MAX)

typedef struct
{
    char name[UL_GAME_NAME_MAX];    // it is not a string but character array, terminating NULL is not necessary
    char magic[3];                  // magic string "ul."
    char startup[GAME_STARTUP_MAX]; // it is not a string but character array, terminating NULL is not necessary
    uint8_t parts;                  // slice count
    uint8_t media;                  // Disc type
    uint8_t unknown[4];             // Always zero
    uint8_t Byte08;                 // Always 0x08
    uint8_t unknown2[10];           // Always zero
} USBExtreme_game_entry_t;

int fhin, fhout;
char *infile, outfile[0x400], outname[] = "%s\\ul.%08X.%s.%02X", temp[4];
char name[0x10], fname[] = "%s", sysfile[] = "SYSTEM.CNF;1", isohdr[] = "CD001";
char conffile[0x400], confname[] = "%s\\ul.cfg";
unsigned char copybuf[0x800000];
unsigned int crctab[0x400];

void ropen()
{
    fhin = _open(infile, _O_RDONLY | _O_BINARY);
    if (!fhin) {
        printf("\nERROR: Couldn't open file %s!\n", infile);
        exit(-1);
    }
}

void ropena()
{
    fhin = _open(infile, _O_RDWR | _O_CREAT | _S_IWRITE | _O_APPEND | _O_BINARY);
    if (!fhin) {
        printf("\nERROR: Couldn't open file %s!\n", infile);
        exit(-1);
    }
}

void rclose()
{
    _close(fhin);
}

void wopen()
{
    fhout = _open(outfile, _O_CREAT | _S_IWRITE | _O_WRONLY | _O_BINARY);
    if (!fhout) {
        printf("\nERROR: Couldn't open file %s!\n", outfile);
        exit(-1);
    }
}

void wclose()
{
    _close(fhout);
}

unsigned int crc32(char *string)
{
    int crc, table, count, byte;

    for (table = 0; table < 256; table++) {
        crc = table << 24;

        for (count = 8; count > 0; count--) {
            if (crc < 0)
                crc = crc << 1;
            else
                crc = (crc << 1) ^ 0x04C11DB7;
        }
        crctab[255 - table] = crc;
    }

    do {
        byte = string[count++];
        crc = crctab[byte ^ ((crc >> 24) & 0xFF)] ^ ((crc << 8) & 0xFFFFFF00);
    } while (string[count - 1] != 0);

    return crc;
}

int main(int argc, char **argv)
{
    unsigned int crc, count, slice, part, fileres, l, i, blksz, pad;
    __int64 offs1, offs2, offs3;

    if (argc < 5 || argc > 5) {
        printf("\n"
               "USAGE: %s IMAGEFILE ROOTPATH \"TITLE\" [CD/DVD]\n"
               "\n"
               "Example: %s \"I:\\games\\foo.ISO\" \"D:\\my\\desktop\" \"My very 1st demo\" CD\n",
               argv[0], argv[0]);
        exit(1);
    }

    if (strlen((const char *)argv[3]) > 0x1F) {
        printf("\nERROR: Title \"%s\" is too long!\n", argv[3]);
        exit(-1);
    }

    strncpy(temp, argv[4], 3);
    temp[0] = tolower(temp[0]);
    temp[1] = tolower(temp[1]);
    temp[2] = tolower(temp[2]);
    temp[3] = 0;

    blksz = 2048;
    pad = 0;
    part = 0;
    infile = argv[1];
    crc = crc32(argv[3]);

    ropen();
    if (_read(fhin, copybuf, 0x100000) != 0) {
        if (strncmp(isohdr, (const char *)copybuf + 0x9319, 5) == 0) {
            blksz = 2352;
            pad = 0x18;
        }
        count = ((copybuf[16 * blksz + pad + 0x9e]) + ((copybuf[16 * blksz + pad + 0x9f]) << 8)) * blksz;
        l = 0;
        do {
            l++;
        } while ((strcmp(sysfile, (const char *)copybuf + count + l) != 0) && (l < 11760));
        count = count + l - 0x1F;
        offs1 = copybuf[count];
        offs2 = copybuf[count + 1];
        offs3 = copybuf[count + 2];
        offs1 = (offs1 + (offs2 << 8) + (offs3 << 16)) * blksz + pad;
        printf("\n");
        _lseeki64(fhin, offs1, 0);
        _read(fhin, copybuf, 0x800);
        rclose();

        count = (unsigned int)memchr(copybuf, ';', 0x800);
        l = (unsigned int)copybuf;
        copybuf[count - l] = 0;
        // count=(unsigned int)memchr(copybuf,'\\',0x800);
        
        /*printf("%d is the value of count\n", count);
        printf("copybuf MEMDUMP ---{\n");
		int x;
        for(x=0; x<0x800;x++)
        {
            if (copybuf[x] == NULL)
                break;
            printf("%c", copybuf[x]);
        }
        printf("}\n");*/

        // snprintf(name, sizeof(name), fname, copybuf+count+1);
        memcpy(name, memchr(copybuf, '\\', 0x800) + 1, sizeof(name));
        printf("main ELF filename is [%s]\n", name);
        if (blksz == 2048) {
            ropen();
            do {
                slice = 0;
                fileres = _read(fhin, copybuf, 0x800000);
                if (fileres > 0) {
                    sprintf(outfile, outname, argv[2], crc, name, part);
                    printf("Writing: %s\n", outfile);
                    wopen();
                    _write(fhout, copybuf, fileres);
                    slice++;
                    if (fileres == 0x800000) {
                        do {
                            fileres = _read(fhin, copybuf, 0x800000);
                            if (fileres > 0) {
                                _write(fhout, copybuf, fileres);
                                slice++;
                            }
                        } while ((slice < 128) && (fileres > 0));
                    }
                    wclose();
                    part++;
                }
            } while ((fileres > 0));
            rclose();

            sprintf(conffile, confname, argv[2]);
            infile = conffile;
            memset(copybuf, 0, 0x100);
            ropena();
            printf("Updating %s\n", conffile);
            strcpy((char *)copybuf, argv[3]);
            strncpy((char *)copybuf + 0x20, outfile + 3, 3);
            strcpy((char *)copybuf + 0x23, name);
            copybuf[0x2F] = part;
            copybuf[0x30] = 0x12;
            if (temp[0] == 0x64)
                copybuf[0x30] = 0x14;
            copybuf[0x35] = 0x08;
            _write(fhin, copybuf, 0x40);
            rclose();

        } else {
            ropen();
            do {
                slice = 0;
                fileres = _read(fhin, copybuf, 0x7FFAA0);
                if (fileres > 0) {
                    sprintf(outfile, outname, argv[2], crc, name, part);
                    printf("Writing: %s\n", outfile);

                    for (l = 0; l < 3566; l++) {
                        for (i = 0; i < 2048; i++) {
                            copybuf[l * 2048 + i] = copybuf[l * 2352 + i + pad];
                        }
                    }

                    wopen();
                    _write(fhout, copybuf, fileres / 2352 * 2048);
                    slice++;
                    if (fileres == 0x7FFAA0) {
                        do {
                            fileres = _read(fhin, copybuf, 0x7FFAA0);
                            if (fileres > 0) {
                                for (l = 0; l < 3566; l++) {
                                    for (i = 0; i < 2048; i++) {
                                        copybuf[l * 2048 + i] = copybuf[l * 2352 + i + pad];
                                    }
                                }
                                _write(fhout, copybuf, fileres / 2352 * 2048);
                                slice++;
                            }
                        } while ((slice < 128) && (fileres > 0));
                    }
                    wclose();
                    part++;
                }
            } while ((fileres > 0));
            rclose();
            // TODO: traverse existing UL.CFG and make sure that an entry with same name and ELF is not there already
            sprintf(conffile, confname, argv[2]);
            infile = conffile;
            memset(copybuf, 0, 0x100);
            ropena();
            printf("Updating %s\n", conffile);
            strcpy((char *)copybuf, argv[3]);
            strncpy((char *)copybuf + 0x20, outfile + 3, 3);
            strcpy((char *)copybuf + 0x23, name);
            copybuf[0x2F] = part;
            copybuf[0x30] = 0x12;
            if (temp[0] == 0x64)
                copybuf[0x30] = 0x14;
            copybuf[0x35] = 0x08;
            _write(fhin, copybuf, 0x40);
            rclose();
        }
    } else {
        rclose();
    }
}
