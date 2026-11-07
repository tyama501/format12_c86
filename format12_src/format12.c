/*
 * format12 - PC-98 floppy physical format (c86 / ELKS)
 *
 * Ported from gcc-ia16 sources. INT 1Bh AH=5Dh is called from
 * int1b5d.s using the c86 small-model stack convention.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern int int1B5D(int buf_offset, int buf_segment, int h_num, int c_num,
                   int sector_num, int data_num, int dev_addr);
extern unsigned int getss(void);

typedef struct {
    char c;
    char h;
    char r;
    char n;
} chs_buf_t;

void clearkbuf(void)
{
    int ck;

    while (((ck = getchar()) != '\n') && (ck != EOF))
        ;
}

int main(void)
{
    unsigned int buf_offset;
    unsigned int buf_segment;
    int s_num;
    int h_num;
    int c_num;
    int s_num_end;
    int h_num_end;
    int c_num_end;
    int sector_num;
    int data_num;
    int dev_addr;
    char dev_name[9];
    int c_drive;
    int c_format;
    char format_s[7];
    int c_yn;
    unsigned char ah;
    int bios_ax;
#ifdef DEBUG
    int debug_i;
#endif
    chs_buf_t chs_buf[15];

    /* Stack object: offset is a near pointer, segment is SS. */
    buf_offset = (unsigned int)chs_buf;
    buf_segment = getss();
    printf("CHS Buffer Segment : %x, Offset : %x\n", buf_segment, buf_offset);

    printf("Which drive fd0 : 0, fd1 : 1, fd2 : 2, fd3 : 3 ?\n");
    c_drive = getchar();
    if (c_drive == '0') {
        dev_addr = 0x90;
        strcpy(dev_name, "/dev/fd0");
    } else if (c_drive == '1') {
        dev_addr = 0x91;
        strcpy(dev_name, "/dev/fd1");
    } else if (c_drive == '2') {
        dev_addr = 0x92;
        strcpy(dev_name, "/dev/fd2");
    } else if (c_drive == '3') {
        dev_addr = 0x93;
        strcpy(dev_name, "/dev/fd3");
    } else {
        printf("Select 0 - 3\n");
        return 1;
    }

    clearkbuf();

    printf("Select Format FD1200 : 0, FD1232 : 1 ?\n");
    c_format = getchar();
    if (c_format == '1') {
        strcpy(format_s, "FD1232");
        s_num_end = 8;
        h_num_end = 2;
        c_num_end = 77;
        sector_num = 3;
        data_num = 32;
    } else {
        strcpy(format_s, "FD1200");
        s_num_end = 15;
        h_num_end = 2;
        c_num_end = 80;
        sector_num = 2;
        data_num = 60;
    }

    clearkbuf();

    printf("Format %s, Device 0x%x ? y/n\n", format_s, dev_addr);
    c_yn = getchar();
    if (c_yn != 'y') {
        printf("Quit\n");
        return 0;
    }

    for (c_num = 0; c_num < c_num_end; c_num++) {
        for (h_num = 0; h_num < h_num_end; h_num++) {
            for (s_num = 0; s_num < s_num_end; s_num++) {
                chs_buf[s_num].c = (char)c_num;
                chs_buf[s_num].h = (char)h_num;
                chs_buf[s_num].r = (char)(s_num + 1);
                chs_buf[s_num].n = (char)sector_num;
            }
#ifdef DEBUG
            for (debug_i = 0; debug_i < s_num_end; debug_i++) {
                printf("chs_buf.c : %d, h : %d, r : %d, n : %d\n",
                       chs_buf[debug_i].c, chs_buf[debug_i].h,
                       chs_buf[debug_i].r, chs_buf[debug_i].n);
            }
#endif
            printf("C : %d, H : %d\n", c_num, h_num);
            bios_ax = int1B5D(buf_offset, buf_segment, h_num, c_num,
                              sector_num, data_num, dev_addr);
            ah = (unsigned char)((bios_ax >> 8) & 0xFF);
            if (ah) {
                printf("Error %x\n", ah);
                return 1;
            }
        }
    }

    printf("Making FAT\n");
    {
        char *av[4];

        /* libc86 has no system()/wait(); overlay this process. */
        if (c_format == '1') {
            av[0] = "makfat12";
            av[1] = dev_name;
            av[2] = 0;
            execv("/bin/makfat12", av);
        } else {
            av[0] = "mkfat";
            av[1] = dev_name;
            av[2] = "1200";
            av[3] = 0;
            execv("/bin/mkfat", av);
        }
        printf("Error mkfat\n");
        return 1;
    }
}
