#ifndef SRC_INCLUDE_APP_UART_H_
#define SRC_INCLUDE_APP_UART_H_


#define UART_DATA_LEN  512

#define RING_BUFF_SIZE                      2048              /* size ring buffer  */
#define RING_BUFF_MASK                      RING_BUFF_SIZE-1  /* mask ring buffer  */

#define UART_BAUDRATE_9600                  9600
#define UART_BAUDRATE_115200                115200

#define UART_MSG_STATUS_ERROR_START_CHAR    0xE0
#define UART_MSG_STATUS_MSG_OVERFLOW        0xE1
#define UART_MSG_STATUS_ERROR_END_CHAR      0xE2
#define UART_MSG_STATUS_BAD_MSG             0xE3
#define UART_MSG_STATUS_UART_EXCEPT         0xE4
#define UART_MSG_STATUS_CRC_ERROR           0xE5

typedef struct {
    uint32_t dma_len;        // dma len must be 4 byte
    uint8_t  data[UART_DATA_LEN];
} uart_data_t;

typedef enum {
    UART_TX_SUCCESS,
    UART_TX_BUFFERFULL,
    UART_TX_BUSY,
    UART_TX_FAILED,
    UART_TX_TOO_LONG,
} uartTx_err;

extern uint8_t uart_msg_err;
extern uint16_t uart_rx_callback_count;
extern uint16_t uart_rx_byte_count;
extern uint16_t uart_rx_last_len;
extern uint8_t uart_rx_last_first_byte;
extern uint16_t uart_rx_first_word;
extern uint16_t uart_rx_first_55aa_offset;
extern uint16_t uart_rx_sync_word;

void app_uart_init();
void set_uart_pins(uint32_t tx_pin, uint32_t rx_pin);
uint32_t get_uart_baudrate();
void set_uart_baudrate(uint32_t baudrate);
uint16_t get_uart_rx_callback_count();
uint16_t get_uart_rx_byte_count();
uint16_t get_uart_rx_last_len();
uint8_t get_uart_rx_last_first_byte();
uint16_t get_uart_rx_first_word();
uint16_t get_uart_rx_first_55aa_offset();
uint16_t get_uart_rx_sync_word();

uartTx_err app_uart_txMsg(uint8_t *data, uint8_t len);
//uint8_t checksum(uint8_t *data, uint16_t length);
//void print_pkt(uint8_t *data, uint32_t len, uint8_t dir);

size_t write_bytes_to_uart(uint8_t *data, size_t len);
uint8_t read_byte_from_ring_buff();
size_t read_bytes_from_buff(uint8_t *str, size_t len);
uint8_t available_ring_buff();
size_t get_queue_len_ring_buff();
void flush_ring_buff();


#endif /* SRC_INCLUDE_APP_UART_H_ */
