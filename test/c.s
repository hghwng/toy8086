org 0x100

mov ax, #$b800
mov ds, ax

mov byte ptr [$02], #$48 ; 'H'
mov byte ptr [$04], #$65 ; 'e'
mov byte ptr [$06], #$6c ; 'l'
mov byte ptr [$08], #$6c ; 'l'
mov byte ptr [$0a], #$6f ; 'o'
mov byte ptr [$0c], #$2c ; ','
mov byte ptr [$0e], #$57 ; 'W'
mov byte ptr [$10], #$6f ; 'o'
mov byte ptr [$12], #$72 ; 'r'
mov byte ptr [$14], #$6c ; 'l'
mov byte ptr [$16], #$64 ; 'd'
mov byte ptr [$18], #$21 ; '!'

mov cx, #12  ; number of characters.
mov di, #3 ; start from byte after 'h'

c:  mov byte ptr [di], #%11101100   ; light red(1110) on yellow(1100)
    add di, #2 ; skip over next ascii code in vga memory.
    loop c

hlt
