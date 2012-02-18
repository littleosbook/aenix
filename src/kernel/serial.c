#include "serial.h"
#include "io.h"
#include "interrupt.h"
#include "common.h"
#include "log.h"
#include "pic.h"

/* ports */
#define DATA_PORT(port) port
#define INTERRUPT_ENABLE_PORT(port) port+1
#define FIFO_CONTROL_PORT(port) port+2
#define LINE_CONTROL_PORT(port) port+3
#define MODEM_CONTROL_PORT(port) port+4
#define LINE_STATUTS_PORT(port) port+5
#define DLAB_LOW_BYTE_PORT(port) port
#define DLAB_HIGH_BYTE_PORT(port) port + 1

/* constants */
#define ENABLE_DLAB 0x80
#define BAUD_RATE_DIVISOR 0x03 /* will give a baud rate of 115200 / 3 = 38400 */

static void serial_handle_interrupt_com1(cpu_state_t state, idt_info_t info,
                          stack_state_t exec)
{
    UNUSED_ARGUMENT(state);
    UNUSED_ARGUMENT(info);
    UNUSED_ARGUMENT(exec);
    log_info("serial_handle_interrupt_com1", "data on com1\n");
    pic_acknowledge();
}

static void serial_handle_interrupt_com2(cpu_state_t state, idt_info_t info,
                          stack_state_t exec)
{
    UNUSED_ARGUMENT(state);
    UNUSED_ARGUMENT(info);
    UNUSED_ARGUMENT(exec);
    log_info("serial_handle_interrupt_com2", "data on com2\n");
    pic_acknowledge();
}

void serial_init(uint16_t com)
{
    uint8_t config;
    /* disable innterupts */
    outb(INTERRUPT_ENABLE_PORT(com), 0x00);

    /* write the baud_rate_divisor */
    outb(LINE_CONTROL_PORT(com), ENABLE_DLAB);
    outb(DLAB_LOW_BYTE_PORT(com), BAUD_RATE_DIVISOR & 0x00FF);
    outb(DLAB_HIGH_BYTE_PORT(com), (BAUD_RATE_DIVISOR & 0xFF00) >> 8);

    /* write the config */

    /* name | value | size | desc
     * --------------------------
     * dl   |     3 |    2 | data length, 3 = 8 bits
     * sp   |     0 |    1 | # stop bits, 0 = 1, 1 = 1.5 or 2
     * p    |     0 |    3 | # parity bits, 0 = NONE
     * bc   |     0 |    1 | break control, 0 = disabled, 1 = enabled
     * dlab |     0 |    1 | if dlab is enabled or not
     */
    config = 0x00 | (1 << 1) | 0x01;
    outb(LINE_CONTROL_PORT(com), config);

    /* name | value | size | desc
     * --------------------------
     * on   |     1 |    1 | enable/disables the FIFO queues
     * rcvr |     1 |    1 | clear and reset the receiver FIFO queue
     * xmit |     1 |    1 | clear and reset the transmitter FIFO queue
     * dma  |     0 |    1 | 0 = single byta DMA, 1 = multi byte DMA
     * res  |     0 |    2 | reserved
     * lvl  |     3 |    2 | num bytes to trigger FIFO, 0x3 = 14 bytes
     */
    config = (1 << 7) | (1 << 6) | (1 << 2) | (1 << 1) | 0x01;
    outb(FIFO_CONTROL_PORT(com), config);

    /* name | value | size | desc
     * --------------------------
     * dtr  |     1 |    1 | DTR = Data Terminal Ready, 1 = on
     * rts  |     1 |    1 | RTS = Request To Send, 1 = on
     * out1 |     0 |    1 | controls output 1, 0 = off
     * out2 |     1 |    1 | controls output 2, 1 = on
     * diag |     0 |    1 | diagnotic mode, 0 = off
     * resv |     0 |    3 | reserved
     */
    config = (1 << 3) | (1 << 1) | 0x01;
    outb(MODEM_CONTROL_PORT(com), config);

    register_interrupt_handler(COM1_INT_IDX, serial_handle_interrupt_com1);
    register_interrupt_handler(COM2_INT_IDX, serial_handle_interrupt_com2);
}

static int is_transmit_fifo_empty(uint16_t com)
{
    /* 0x20 = bit 5: 1 if XMIT fifo is empty */
    return inb(LINE_STATUTS_PORT(com)) & 0x20;
}

void serial_write(uint16_t com, uint8_t data)
{
    while (!is_transmit_fifo_empty(com)) {
        /* wait and try again */
    }
    outb(DATA_PORT(com), data);
}

static int is_receiver_fifo_full(uint16_t com)
{
    /* 0x01 = bit 5: 1 if RCVR fifo is full */
    return inb(LINE_STATUTS_PORT(com)) & 0x01;
}

uint8_t serial_read(uint16_t com)
{
    while (!is_receiver_fifo_full(com)) {
        /* wait and try again */
    }

    return inb(DATA_PORT(com));
}
