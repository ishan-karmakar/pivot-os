struc dap_t
    .size: resb 1
    .rsv: resb 1
    .num_sectors: resw 1
    .buffer: resd 1
    .low_lba: resd 1
    .high_lba: resd 1
endstruc

load_sectors:
    mov si, dap
    mov ah, 0x42
    mov dl, 0x80
    mov bx, disk_read_err
    int 0x13
    jc error
    ret

error:
    call print
    jmp $

print:
    pusha
.start:
    mov al, [bx]
    cmp al, 0
    je .done
    call printc
    add bx, 1
    jmp .start
.done:
    popa
    ret

printh:
    pusha
    mov cx, 0
.start:
    cmp cx, 4
    je .end

    mov ax, dx
    and ax, 0xF
    add al, 0x30
    cmp al, 0x39
    jle .step2
    add al, 7
.step2:
    mov bx, HEX_OUT + 5
    sub bx, cx
    mov [bx], al
    ror dx, 4

    add cx, 1
    jmp .start
.end:
    mov bx, HEX_OUT
    call print
    popa
    ret

disk_read_err: db `Disk read error\0`
HEX_OUT: db '0x0000',0 ; reserve memory for our new string
dap: istruc dap_t
    at dap_t.size, db 16
    at dap_t.rsv, db 0
    at dap_t.high_lba, dd 0
iend