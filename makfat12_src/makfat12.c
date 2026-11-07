/*
 * makfat12 - write a PC-98 1232kB FAT12 filesystem
 *
 * Geometry is fixed to the NEC 2HD 1232kB floppy:
 *   77 cylinders, 2 heads, 8 sectors/track, 1024 bytes/sector
 *
 * Physical format of the media is assumed to be already done.
 * This program writes the boot sector, two FAT copies and an
 * empty root directory (a "quick format").
 *
 * Usage: makfat12 [device]
 *   default device is /dev/fd0
 */
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef _M_I86
typedef unsigned long uint32;
#else
typedef unsigned int uint32;
#endif
typedef unsigned short uint16;
typedef unsigned char uint8;

#define SECSIZE     1024
#define CYL         77
#define HEADS       2
#define SPT         8
#define TOTALSEC    (CYL * HEADS * SPT) /* 1232 */
#define SPC         1
#define RESERVED    1
#define NFATS       2
#define ROOTENT     192
#define MEDIA       0xFE
#define SPF         2
#define ROOTSECS    ((ROOTENT * 32) / SECSIZE)  /* 6 */

static uint8 sec[SECSIZE];
static int ofd;

static void put16(uint8 *p, uint16 v)
{
    p[0] = (uint8)(v & 0xFF);
    p[1] = (uint8)((v >> 8) & 0xFF);
}

static void put32(uint8 *p, uint32 v)
{
    p[0] = (uint8)(v & 0xFF);
    p[1] = (uint8)((v >> 8) & 0xFF);
    p[2] = (uint8)((v >> 16) & 0xFF);
    p[3] = (uint8)((v >> 24) & 0xFF);
}

static int write_lba(uint16 lba)
{
    long off;
    int n;

    off = (long)lba * SECSIZE;
    if (lseek(ofd, off, 0) != off) {
        printf("lseek sector %d failed\n", lba);
        return 0;
    }
    n = write(ofd, sec, SECSIZE);
    if (n != SECSIZE) {
        printf("write sector %d failed (n=%d)\n", lba, n);
        return 0;
    }
    return 1;
}

static void make_boot(void)
{
    memset(sec, 0, SECSIZE);

    /* JMP 003E / NOP */
    sec[0] = 0xEB;
    sec[1] = 0x3C;
    sec[2] = 0x90;

    /* OEM name */
    memcpy(sec + 3, "MAKFAT12", 8);

    put16(sec + 11, SECSIZE);       /* bytes per sector */
    sec[13] = SPC;                  /* sectors per cluster */
    put16(sec + 14, RESERVED);      /* reserved sectors */
    sec[16] = NFATS;                /* number of FATs */
    put16(sec + 17, ROOTENT);       /* root entries */
    put16(sec + 19, TOTALSEC);      /* total sectors (small) */
    sec[21] = MEDIA;                /* media descriptor */
    put16(sec + 22, SPF);           /* sectors per FAT */
    put16(sec + 24, SPT);           /* sectors per track */
    put16(sec + 26, HEADS);         /* heads */
    put32(sec + 28, 0L);            /* hidden sectors */
    put32(sec + 32, 0L);            /* total sectors (large) */

    sec[36] = 0x00;                 /* drive number (floppy) */
    sec[37] = 0x00;
    sec[38] = 0x29;                 /* extended boot signature */
    sec[39] = 0x12;                 /* volume serial */
    sec[40] = 0x34;
    sec[41] = 0x56;
    sec[42] = 0x78;
    memcpy(sec + 43, "MAKFAT12   ", 11);
    memcpy(sec + 54, "FAT12   ", 8);

    /* tiny stub: cli / hlt / jmp hlt */
    sec[0x3E] = 0xFA;
    sec[0x3F] = 0xF4;
    sec[0x40] = 0xEB;
    sec[0x41] = 0xFD;

    sec[510] = 0x55;
    sec[511] = 0xAA;
}

static void make_fat_sector(int idx)
{
    memset(sec, 0, SECSIZE);
    if (idx == 0) {
        /* cluster 0 = media|0xF00, cluster 1 = EOC */
        sec[0] = MEDIA;
        sec[1] = 0xFF;
        sec[2] = 0xFF;
    }
}

static void make_root_sector(int idx)
{
    memset(sec, 0, SECSIZE);
    if (idx == 0) {
        /* volume label directory entry */
        memcpy(sec, "MAKFAT12   ", 11);
        sec[11] = 0x08;     /* ATTR_VOLUME */
    }
}

int main(int ac, char **av)
{
    char *dev;
    uint16 lba;
    uint16 i, f;
    uint16 data_sec;
    uint16 clusters;
    int n;

    dev = "/dev/fd0";
    if (ac >= 2)
        dev = av[1];

    data_sec = TOTALSEC - RESERVED - (NFATS * SPF) - ROOTSECS;
    clusters = data_sec / SPC;

    printf("PC-98 1232kB FAT12\n");
    printf("  C/H/S %d/%d/%d  sector %d bytes\n", CYL, HEADS, SPT, SECSIZE);
    printf("  total sectors %d  media 0xFE\n", TOTALSEC);
    printf("  reserved %d  fats %d  spf %d\n", RESERVED, NFATS, SPF);
    printf("  root entries %d  root sectors %d\n", ROOTENT, ROOTSECS);
    printf("  data sectors %d  clusters %d\n", data_sec, clusters);
    printf("  device %s\n", dev);

    ofd = open(dev, O_RDWR);
    if (ofd < 0) {
        printf("cannot open %s\n", dev);
        return 1;
    }

    lba = 0;
    make_boot();
    if (!write_lba(lba))
        goto fail;
    lba++;
    printf("wrote boot sector\n");

    for (f = 0; f < NFATS; f++) {
        for (i = 0; i < SPF; i++) {
            make_fat_sector(i);
            if (!write_lba(lba))
                goto fail;
            lba++;
        }
        printf("wrote FAT %d\n", f + 1);
    }

    for (i = 0; i < ROOTSECS; i++) {
        make_root_sector(i);
        if (!write_lba(lba))
            goto fail;
        lba++;
    }
    printf("wrote root directory\n");

    /* read boot sector back and check a few BPB fields */
    if (lseek(ofd, 0L, 0) != 0L) {
        printf("lseek 0 for verify failed\n");
        goto fail;
    }
    memset(sec, 0, SECSIZE);
    n = read(ofd, sec, SECSIZE);
    if (n != SECSIZE) {
        printf("verify read failed (n=%d)\n", n);
        goto fail;
    }
    if (sec[11] != 0x00 || sec[12] != 0x04) {
        printf("verify: bytes/sector mismatch\n");
        goto fail;
    }
    if (sec[21] != MEDIA) {
        printf("verify: media byte mismatch\n");
        goto fail;
    }
    if (sec[510] != 0x55 || sec[511] != 0xAA) {
        printf("verify: boot signature mismatch\n");
        goto fail;
    }
    printf("verify boot sector OK\n");

    close(ofd);
    sync();
    printf("done, %d sectors written\n", lba);
    return 0;

fail:
    close(ofd);
    return 1;
}
