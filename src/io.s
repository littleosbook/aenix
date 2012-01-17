global outb         ; for sending a byte to a port
global inb          ; for reading a byte from a port
global inw          ; for reading a word from port

section .text:

outb:
    mov dx, [esp+4] ; the port to send data to
    mov al, [esp+8] ; the data to send
    out dx, al      ; send the contents of al to the port in cx
    ret

inb:
    mov dx, [esp+4] ; the port to read from
    in  al, dx      ; reads a byte from dx into al
    ret             ; returns the read byte

inw:
    mov dx, [esp+4] ; the port to read from
    in  ax, dx      ; read a word from dx into ax
    ret             ; returns the read word
