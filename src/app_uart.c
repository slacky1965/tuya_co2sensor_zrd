#include "tl_common.h"

#include "app_main.h"

uart_data_t rec_buff = {0,  {0, } };
uint8_t  ring_buff[RING_BUFF_SIZE];
uint16_t ring_head, ring_tail;
uint8_t uart_msg_err;

static uint32_t uart_baudrate = UART_BAUDRATE_9600;

#if UART_PRINTF_MODE && DEBUG_PKT

static void print_pkt(uint8_t *data, uint32_t len) {

    uint8_t ch;

    for (int i = 0; i < len; i++) {
        ch = data[i];
        if (ch < 0x10) {
            printf("0%x", ch);
        } else {
            printf("%x", ch);
        }
    }
    printf("\r\n");
}

#endif

static void print_pkt_inp(uint8_t *data, uint32_t len) {

#if UART_PRINTF_MODE && DEBUG_PKT

#if DEBUG_PKT_FILTER

    static uint8_t command = 0;
    static uint32_t time_pkt = 0;
    pkt_tuya_t *pkt = (pkt_tuya_t*)data;

    if (command != pkt->command) {
        command = pkt->command;
        time_pkt = clock_time();
        print_time();
        printf("inp_pkt ==> 0x");
        print_pkt(data, len);
    } else {
        if (clock_time_exceed(time_pkt, TIMEOUT_TICK_50MS)) {
            print_time();
            printf("inp_pkt ==> 0x");
            print_pkt(data, len);
        }
        time_pkt = clock_time();
    }
#else
    printf("inp_pkt ==> 0x");
    print_pkt(data, len);
#endif
#endif
}

static void print_pkt_out(uint8_t *data, uint32_t len) {

#if UART_PRINTF_MODE && DEBUG_PKT

#if DEBUG_PKT_FILTER

    static uint8_t command = 0;
    static uint32_t time_pkt = 0;
    pkt_tuya_t *pkt = (pkt_tuya_t*)data;


    if (command != pkt->command) {
        command = pkt->command;
        time_pkt = clock_time();
        print_time();
        printf("out_pkt <== 0x");
        print_pkt(data, len);
    } else {
        if (clock_time_exceed(time_pkt, TIMEOUT_TICK_50MS)) {
            print_time();
            printf("out_pkt <== 0x");
            print_pkt(data, len);
        }
        time_pkt = clock_time();
    }
#else
    printf("out_pkt <== 0x");
    print_pkt(data, len);
#endif
#endif
}

uint8_t available_ring_buff() {
    if (ring_head != ring_tail) {
        return true;
    }
    return false;
}

size_t get_queue_len_ring_buff() {
   return (ring_head - ring_tail) & (RING_BUFF_MASK);
}

static size_t get_freespace_ring_buff() {
    return (sizeof(ring_buff)/sizeof(ring_buff[0]) - get_queue_len_ring_buff());
}

void flush_ring_buff() {
    ring_head = ring_tail = 0;
    memset(ring_buff, 0, RING_BUFF_SIZE);
}

uint8_t read_byte_from_ring_buff() {
    uint8_t ch = ring_buff[ring_tail++];
    ring_tail &= RING_BUFF_MASK;
    return ch;

}

size_t read_bytes_from_buff(uint8_t *str, size_t len) {

    size_t i = 0;

    while (i < len) {
        /* check for empty buffer */
        if (get_queue_len_ring_buff()) {
            str[i++] = read_byte_from_ring_buff();
        } else {
            break;
        }
    }

//    for (i = 0; i < len; i++) {
//        str[i] = read_byte_from_ring_buff();
//    }

//    printf("read_bytes_from_buff(). len: %d, i: %d\r\n", len, i);

    return i;
}

static size_t write_bytes_to_ring_buff(uint8_t *data, size_t len) {

    size_t free_space = get_freespace_ring_buff();
    size_t put_len;

//    printf("free_space: %d, len: %d\r\n", free_space, len);

    if (free_space >= len) put_len = len;
    else put_len = free_space;

    for (int i = 0; i < put_len; i++) {
        ring_buff[ring_head++] = data[i];
        ring_head &= RING_BUFF_MASK;
    }

    return put_len;
}

static void app_uartRecvCb() {

    uint8_t st = SUCCESS;

    if(rec_buff.dma_len == 0) {
        st = UART_MSG_STATUS_UART_EXCEPT;
    }

    if(rec_buff.dma_len > UART_DATA_LEN) {
        st = UART_MSG_STATUS_MSG_OVERFLOW;
    }

    if (st == SUCCESS) {

        print_pkt_inp(rec_buff.data, rec_buff.dma_len);

        write_bytes_to_ring_buff(rec_buff.data, rec_buff.dma_len);
        sleep_ms(10);
    }

//    printf("st: 0x%x, rec_buff.dma_len: %d\r\n", st, rec_buff.dma_len);

//    rec_buff.dma_len = 0;

    uart_msg_err = st;
}

uartTx_err app_uart_txMsg(uint8_t *data, uint8_t len) {

    print_pkt_out(data, len);

    if (drv_uart_tx_start(data, len)) return UART_TX_SUCCESS;

    return UART_TX_FAILED;
}

void app_uart_init() {

    flush_ring_buff();
    drv_uart_pin_set(GPIO_UART_TX, GPIO_UART_RX);

    drv_uart_init(uart_baudrate, (uint8_t*)&rec_buff, sizeof(uart_data_t), app_uartRecvCb);

//    printf("uart_baudrate: %d\r\n", uart_baudrate);
}

uint32_t get_uart_baudrate() {
    return uart_baudrate;
}

void set_uart_baudrate(uint32_t baudrate) {
    uart_baudrate = baudrate;
}
