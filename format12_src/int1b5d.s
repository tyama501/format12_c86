; int1B5D.s - PC-98 DISK BIOS INT 1Bh AH=5Dh (format track)
; Assembled with ELKS as86 for the c86 toolchain.
;
; c86 small model, near call, arguments pushed right-to-left
; (same layout as gcc-ia16 near cdecl):
;   4[bp]  buf_offset     ES:BP format buffer (C,H,R,N x NSEC)
;   6[bp]  buf_segment
;   8[bp]  h_num          DH
;  10[bp]  c_num          CL
;  12[bp]  sector_num     CH  (N: 2=512, 3=1024)
;  14[bp]  data_num       BX  (buffer length in bytes)
;  16[bp]  dev_addr       AL  (90h-93h)
; Return: AX = BIOS AX (AH=status)
;
        use16   86
        .text

        .global _int1B5D
_int1B5D:
        push    bp
        mov     bp, sp
        push    es
        push    si
        push    di

        mov     ah, #0x5D
        mov     al, 16[bp]
        mov     bx, 14[bp]
        mov     ch, 12[bp]
        mov     cl, 10[bp]
        mov     dh, 8[bp]
        mov     si, 6[bp]
        mov     es, si
        mov     bp, 4[bp]
        mov     dl, #0xE5

        int     0x1B

        pop     di
        pop     si
        pop     es
        pop     bp
        ret

; unsigned int getss(void) - SS of the caller (stack-resident CHS buffer)
        .global _getss
_getss:
        mov     ax, ss
        ret
