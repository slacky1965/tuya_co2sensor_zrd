#include "tl_common.h"

#include "app_main.h"

#define NOT_NEED_CONFIRM    false
#define NEED_CONFIRM        true

static uint8_t      pkt_buff[DATA_MAX_LEN*2];
bool                first_start = true;
static uint8_t      answer_count = 0;
static uint16_t     seq_num = 0;
static status_net_t status_net = STATUS_NET_UNKNOWN;
static uint8_t      no_answer = false;
static uint8_t      startup_net_sync_sent = false;
static uint8_t      factory_reset_cnt = 0;
static uint8_t      factory_reset_status = 0;
static ev_timer_event_t *factory_resetTimerEvt = NULL;
static ev_timer_event_t *check_answerTimerEvt = NULL;
static ev_timer_event_t *check_answerMcuTimerEvt = NULL;

cmd_queue_t cmd_queue = {0};

static uint8_t diag_last_command = 0;
static uint8_t diag_frame_ok = 0;
static uint8_t diag_crc_ok = 0;
static uint8_t diag_dp_seen = 0;
static uint8_t diag_dp_match = 0;
static uint8_t diag_last_dp_id = 0;
static uint8_t diag_tx_ok = 0;
static uint8_t diag_tx_fail = 0;
static uint8_t diag_ctrl_idx = 0xff;
static uint8_t diag_uart_tx_idx = 0xff;
static uint8_t diag_uart_rx_idx = 0xff;
static uint8_t diag_uart_baud_idx = 0xff;

static char hex_nibble(uint8_t value) {
    value &= 0x0f;
    return value < 10 ? ('0' + value) : ('a' + value - 10);
}

static void put_hex_byte(uint8_t *out, uint8_t value) {
    out[0] = hex_nibble(value >> 4);
    out[1] = hex_nibble(value);
}

static void update_diag_swbuild(void) {
    uint8_t *s = g_zcl_basicAttrs.swBuildId;

    s[0] = 15;
    s[1] = 'D';
    put_hex_byte(&s[2], diag_tx_ok);
    put_hex_byte(&s[4], (uint8_t)get_uart_rx_callback_count());
    put_hex_byte(&s[6], (uint8_t)get_uart_rx_byte_count());
    put_hex_byte(&s[8], diag_ctrl_idx);
    put_hex_byte(&s[10], diag_uart_tx_idx);
    put_hex_byte(&s[12], diag_uart_rx_idx);
    put_hex_byte(&s[14], diag_uart_baud_idx);
}

uint8_t checksum(uint8_t *data, uint16_t length) {

    uint8_t crc8 = 0;

    for(uint8_t i = 0; i < length; i++) {
        crc8 += data[i];
    }

    return crc8;
}

void add_cmd_queue(pkt_tuya_t *pkt, uint8_t confirm_need) {

//    printf("cmd_queue.cmd_num: %d\r\n", cmd_queue.cmd_num);

    for (uint8_t i = 0; i < CMD_QUEUE_CELL_MAX; i++) {
        if (!cmd_queue.cmd_queue[i].used) {
            memset(&cmd_queue.cmd_queue[i], 0, sizeof(cmd_queue_cell_t));
            cmd_queue.cmd_queue[i].used = true;
            cmd_queue.cmd_queue[i].confirm_need = confirm_need;
            if (confirm_need) cmd_queue.need_confirm = true;
            else cmd_queue.not_need_confirm = true;
            memcpy(&(cmd_queue.cmd_queue[i].pkt), pkt, sizeof(pkt_tuya_t));
            break;
        }
    }
}

static cmd_queue_cell_t *get_first_queue(uint8_t confirm) {

    for (uint8_t i = 0; i < CMD_QUEUE_CELL_MAX; i++) {
        if (cmd_queue.cmd_queue[i].used) {
            if (cmd_queue.cmd_queue[i].confirm_need == confirm) {
                return &cmd_queue.cmd_queue[i];
            }
        }
    }

    return NULL;
}

static void reset_cmd_queue() {
    memset(&cmd_queue, 0, sizeof(cmd_queue_t));
}

static void move_cmd_queue(cmd_queue_cell_t *current_queue) {

    if (current_queue) {
        if (current_queue->confirm_rec) {
            current_queue->used = 0;
            cmd_queue.need_confirm = false;
            cmd_queue.not_need_confirm = false;
            for(uint8_t i = 0; i < CMD_QUEUE_CELL_MAX; i++) {
                if (cmd_queue.cmd_queue[i].used) {
                    if (cmd_queue.cmd_queue[i].confirm_need) {
                        cmd_queue.need_confirm = true;
                    } else {
                        cmd_queue.not_need_confirm = true;
                    }
                }
            }
        }
    }

}

static uartTx_err send_command(pkt_tuya_t *pkt) {

    uartTx_err ret = UART_TX_FAILED;

    for(uint8_t i = 0; i < 100; i++) {
        ret = app_uart_txMsg((uint8_t*)pkt, pkt->pkt_len);
        if (ret == UART_TX_SUCCESS) break;
        sleep_ms(5);
    }

    if (ret == UART_TX_SUCCESS) {
        diag_tx_ok++;
    } else {
        diag_tx_fail++;
    }

    return ret;
}

static uint8_t read_tuya_frame(uint8_t *answer_buff, size_t *load_size, uint8_t wait_ticks) {

    pkt_tuya_t *pkt = (pkt_tuya_t*)answer_buff;
    uint8_t ch;
    uint8_t header_wait = wait_ticks;
    uint8_t payload_wait = wait_ticks;

    if (header_wait < 10) {
        header_wait = 10;
    }

    if (payload_wait < 100) {
        payload_wait = 100;
    }

    *load_size = 0;

    while (available_ring_buff() && *load_size < (DATA_MAX_LEN + 8)) {
        ch = read_byte_from_ring_buff();

        if (*load_size == 0) {
            if (ch != FLAG_START1) {
                continue;
            }
        } else if (*load_size == 1) {
            if (ch != FLAG_START2) {
                *load_size = 0;
                continue;
            }
        }

        answer_buff[(*load_size)++] = ch;

        if (*load_size == 2) {
            while (get_queue_len_ring_buff() < 6 && header_wait--) {
#if (MODULE_WATCHDOG_ENABLE)
                drv_wd_clear();
#endif
                sleep_ms(10);
            }

            if (get_queue_len_ring_buff() < 6) {
                return false;
            }

            *load_size += read_bytes_from_buff(answer_buff + *load_size, 6);
            if (*load_size != 8) {
                *load_size = 0;
                return false;
            }

            pkt->len = reverse16(pkt->len);
            if (pkt->len > DATA_MAX_LEN) {
                flush_ring_buff();
                *load_size = 0;
                return false;
            }

            while (get_queue_len_ring_buff() < (size_t)(pkt->len + 1) && payload_wait--) {
#if (MODULE_WATCHDOG_ENABLE)
                drv_wd_clear();
#endif
                sleep_ms(10);
            }

            if (get_queue_len_ring_buff() < (size_t)(pkt->len + 1)) {
                return false;
            }

            *load_size += read_bytes_from_buff(answer_buff + *load_size, pkt->len + 1);
            return (*load_size == (size_t)(pkt->len + 9));
        }
    }

    return false;
}

static void float_control_pin(uint32_t pin) {
    drv_gpio_func_set(pin);
    drv_gpio_write(pin, 0);
    drv_gpio_output_en(pin, false);
    drv_gpio_input_en(pin, false);
}

static void drive_control_pin(uint32_t pin, uint8_t value) {
    drv_gpio_func_set(pin);
    drv_gpio_write(pin, value);
    drv_gpio_output_en(pin, true);
    drv_gpio_input_en(pin, false);
}

static uint8_t control_profile_uses_pa0(uint8_t ctrl_idx) {
    return ctrl_idx >= 1 && ctrl_idx <= 6;
}

static void wait_for_mcu_boot(uint16_t ticks) {
    while (ticks--) {
#if (MODULE_WATCHDOG_ENABLE)
        drv_wd_clear();
#endif
        sleep_ms(10);
    }
}

static void apply_control_profile(uint8_t ctrl_idx) {
    float_control_pin(GPIO_PA0);
    float_control_pin(GPIO_PB4);
    sleep_ms(20);

    switch (ctrl_idx) {
        case 0:
            break;
        case 1:
            drive_control_pin(GPIO_PA0, 1);
            drive_control_pin(GPIO_PB4, 1);
            break;
        case 2:
            drive_control_pin(GPIO_PA0, 0);
            drive_control_pin(GPIO_PB4, 1);
            break;
        case 3:
            drive_control_pin(GPIO_PA0, 1);
            drive_control_pin(GPIO_PB4, 0);
            break;
        case 4:
            drive_control_pin(GPIO_PA0, 0);
            drive_control_pin(GPIO_PB4, 0);
            break;
        case 5:
            drive_control_pin(GPIO_PA0, 1);
            break;
        case 6:
            drive_control_pin(GPIO_PA0, 0);
            break;
        case 7:
            drive_control_pin(GPIO_PB4, 1);
            break;
        case 8:
            drive_control_pin(GPIO_PB4, 0);
            break;
        default:
            break;
    }
}

static void soft_uart_prepare_tx(uint32_t pin) {
    drv_gpio_func_set(pin);
    drv_gpio_write(pin, 1);
    drv_gpio_output_en(pin, true);
    drv_gpio_input_en(pin, false);
}

static void soft_uart_prepare_rx(uint32_t pin) {
    drv_gpio_func_set(pin);
    drv_gpio_write(pin, 0);
    drv_gpio_output_en(pin, false);
    drv_gpio_input_en(pin, true);
}

static uint16_t soft_uart_bit_us(uint32_t baudrate) {
    return baudrate == UART_BAUDRATE_115200 ? 9 : 104;
}

static void soft_uart_send_byte(uint32_t tx_pin, uint8_t value, uint16_t bit_us) {
    drv_gpio_write(tx_pin, 0);
    sleep_us(bit_us);

    for (uint8_t i = 0; i < 8; i++) {
        drv_gpio_write(tx_pin, value & 0x01);
        value >>= 1;
        sleep_us(bit_us);
    }

    drv_gpio_write(tx_pin, 1);
    sleep_us(bit_us);
}

static void soft_uart_send(uint32_t tx_pin, uint8_t *data, uint8_t len, uint16_t bit_us) {
    soft_uart_prepare_tx(tx_pin);
    sleep_us(bit_us * 3);

    for (uint8_t i = 0; i < len; i++) {
        soft_uart_send_byte(tx_pin, data[i], bit_us);
    }
}

static uint8_t soft_uart_read_byte(uint32_t rx_pin, uint8_t *value, uint16_t bit_us, uint32_t timeout_us) {
    while (timeout_us) {
        if (!drv_gpio_read(rx_pin)) {
            break;
        }
        sleep_us(10);
        timeout_us = timeout_us > 10 ? timeout_us - 10 : 0;
    }

    if (drv_gpio_read(rx_pin)) {
        return false;
    }

    sleep_us(bit_us + (bit_us / 2));

    uint8_t result = 0;
    for (uint8_t i = 0; i < 8; i++) {
        if (drv_gpio_read(rx_pin)) {
            result |= BIT(i);
        }
        sleep_us(bit_us);
    }

    *value = result;
    return true;
}

static uint8_t soft_uart_read_frame(uint32_t rx_pin, uint8_t *answer_buff, size_t *load_size, uint16_t bit_us) {
    uint8_t ch;

    *load_size = 0;

    for (uint8_t i = 0; i < 32; i++) {
        if (!soft_uart_read_byte(rx_pin, &ch, bit_us, 100000)) {
            return false;
        }

        if (ch == FLAG_START1) {
            answer_buff[(*load_size)++] = ch;
            break;
        }
    }

    if (*load_size != 1) {
        return false;
    }

    if (!soft_uart_read_byte(rx_pin, &ch, bit_us, 30000) || ch != FLAG_START2) {
        *load_size = 0;
        return false;
    }

    answer_buff[(*load_size)++] = ch;

    for (uint8_t i = 0; i < 6; i++) {
        if (!soft_uart_read_byte(rx_pin, &answer_buff[(*load_size)++], bit_us, 30000)) {
            *load_size = 0;
            return false;
        }
    }

    pkt_tuya_t *pkt = (pkt_tuya_t*)answer_buff;
    pkt->len = reverse16(pkt->len);
    if (pkt->len > DATA_MAX_LEN) {
        *load_size = 0;
        return false;
    }

    for (uint16_t i = 0; i < pkt->len + 1; i++) {
        if (!soft_uart_read_byte(rx_pin, &answer_buff[(*load_size)++], bit_us, 30000)) {
            *load_size = 0;
            return false;
        }
    }

    return (*load_size == (size_t)(pkt->len + 9));
}

static uint8_t probe_soft_gpio_uart(pkt_tuya_t *probe_pkt) {
    const uint32_t tx_pins[] = {GPIO_PB1, GPIO_PA2, GPIO_PC2, GPIO_PD0, GPIO_PD3, GPIO_PD7};
    const uint32_t rx_pins[] = {
        GPIO_PA0, GPIO_PA1, GPIO_PA2, GPIO_PA3, GPIO_PA4,
        GPIO_PB0, GPIO_PB1, GPIO_PB2, GPIO_PB3, GPIO_PB4, GPIO_PB5, GPIO_PB6, GPIO_PB7,
        GPIO_PC0, GPIO_PC1, GPIO_PC2, GPIO_PC3, GPIO_PC4, GPIO_PC5,
        GPIO_PD0, GPIO_PD1, GPIO_PD2, GPIO_PD3, GPIO_PD4, GPIO_PD5, GPIO_PD6, GPIO_PD7
    };
    const uint32_t baudrates[] = {UART_BAUDRATE_9600, UART_BAUDRATE_115200};
    const uint8_t ctrl_profiles[] = {1, 0, 2, 3, 4, 5, 6, 7, 8};
    uint8_t answer_buff[DATA_MAX_LEN + 24];
    size_t load_size = 0;
    pkt_tuya_t *answer_pkt = (pkt_tuya_t*)answer_buff;

    for (uint8_t ctrl_i = 0; ctrl_i < ARRAY_SIZE(ctrl_profiles); ctrl_i++) {
        uint8_t ctrl_idx = ctrl_profiles[ctrl_i];
        uint8_t pa0_is_control = control_profile_uses_pa0(ctrl_idx);

        apply_control_profile(ctrl_idx);
        wait_for_mcu_boot(500);

        for (uint8_t baud_idx = 0; baud_idx < ARRAY_SIZE(baudrates); baud_idx++) {
            uint16_t bit_us = soft_uart_bit_us(baudrates[baud_idx]);

            for (uint8_t tx_idx = 0; tx_idx < ARRAY_SIZE(tx_pins); tx_idx++) {
                soft_uart_prepare_tx(tx_pins[tx_idx]);

                for (uint8_t rx_idx = 0; rx_idx < ARRAY_SIZE(rx_pins); rx_idx++) {
                    if (tx_pins[tx_idx] == rx_pins[rx_idx]) {
                        continue;
                    }
                    if (pa0_is_control && rx_pins[rx_idx] == GPIO_PA0) {
                        continue;
                    }

                    soft_uart_prepare_rx(rx_pins[rx_idx]);
                    soft_uart_send(tx_pins[tx_idx], (uint8_t*)probe_pkt, probe_pkt->pkt_len, bit_us);
                    diag_tx_ok++;

                    if (soft_uart_read_frame(rx_pins[rx_idx], answer_buff, &load_size, bit_us)) {
                        answer_pkt->pkt_len = load_size;
                        uint8_t crc = checksum((uint8_t*)answer_pkt, answer_pkt->pkt_len - 1);

                        if (crc == answer_buff[answer_pkt->pkt_len - 1] && answer_pkt->command == COMMAND01) {
                            diag_ctrl_idx = 0x80 | ctrl_idx;
                            diag_uart_tx_idx = tx_idx;
                            diag_uart_rx_idx = rx_idx;
                            diag_uart_baud_idx = baud_idx;
                            diag_frame_ok++;
                            diag_crc_ok++;
                            update_diag_swbuild();
                            return true;
                        }
                    }

#if (MODULE_WATCHDOG_ENABLE)
                    drv_wd_clear();
#endif
                }
            }
        }
    }

    update_diag_swbuild();
    return false;
}

static uint8_t probe_uart_path(void) {

    const uint32_t tx_pins[] = {UART_TX_PB1, UART_TX_PA2, UART_TX_PC2, UART_TX_PD0, UART_TX_PD3, UART_TX_PD7};
    const uint32_t rx_pins[] = {UART_RX_PB7, UART_RX_PA0, UART_RX_PB0, UART_RX_PC3, UART_RX_PC5, UART_RX_PD6};
    const uint32_t baudrates[] = {UART_BAUDRATE_9600, UART_BAUDRATE_115200};
    const uint8_t ctrl_profiles[] = {1, 0, 2, 3, 4, 5, 6, 7, 8};
    uint8_t answer_buff[DATA_MAX_LEN + 24];
    size_t load_size = 0;
    pkt_tuya_t probe_pkt;
    pkt_tuya_t *answer_pkt = (pkt_tuya_t*)answer_buff;

    set_header_pkt((uint8_t*)&probe_pkt, sizeof(probe_pkt), seq_num, COMMAND01);
    probe_pkt.len = 0;
    probe_pkt.pkt_len++;
    probe_pkt.pkt_len++;
    probe_pkt.data[0] = checksum((uint8_t*)&probe_pkt, probe_pkt.pkt_len++);

    for (uint8_t ctrl_i = 0; ctrl_i < ARRAY_SIZE(ctrl_profiles); ctrl_i++) {
        uint8_t ctrl_idx = ctrl_profiles[ctrl_i];
        uint8_t pa0_is_control = control_profile_uses_pa0(ctrl_idx);

        apply_control_profile(ctrl_idx);
        wait_for_mcu_boot(500);

        for (uint8_t baud_idx = 0; baud_idx < ARRAY_SIZE(baudrates); baud_idx++) {
            set_uart_baudrate(baudrates[baud_idx]);

            for (uint8_t tx_idx = 0; tx_idx < ARRAY_SIZE(tx_pins); tx_idx++) {
                for (uint8_t rx_idx = 0; rx_idx < ARRAY_SIZE(rx_pins); rx_idx++) {
                    if (pa0_is_control && rx_pins[rx_idx] == UART_RX_PA0) {
                        continue;
                    }

                    set_uart_pins(tx_pins[tx_idx], rx_pins[rx_idx]);
                    app_uart_init();
                    sleep_ms(20);
                    flush_ring_buff();

                    if (send_command(&probe_pkt) != UART_TX_SUCCESS) {
                        continue;
                    }

                    for(uint8_t i = 0; i < 30; i++ ) {
                        if (read_tuya_frame(answer_buff, &load_size, 2)) {
                            answer_pkt->pkt_len = load_size;
                            uint8_t crc = checksum((uint8_t*)answer_pkt, answer_pkt->pkt_len - 1);

                            if (crc == answer_buff[answer_pkt->pkt_len - 1] && answer_pkt->command == COMMAND01) {
                                diag_ctrl_idx = ctrl_idx;
                                diag_uart_tx_idx = tx_idx;
                                diag_uart_rx_idx = rx_idx;
                                diag_uart_baud_idx = baud_idx;
                                diag_frame_ok++;
                                diag_crc_ok++;
                                update_diag_swbuild();
                                return true;
                            }
                        }
#if (MODULE_WATCHDOG_ENABLE)
                        drv_wd_clear();
#endif
                        sleep_ms(10);
                    }
                }
            }
        }
    }

    if (probe_soft_gpio_uart(&probe_pkt)) {
        return true;
    }

    set_uart_baudrate(UART_BAUDRATE_9600);
    set_uart_pins(GPIO_UART_TX, GPIO_UART_RX);
    app_uart_init();
    update_diag_swbuild();
    return false;
}

static void init_known_mja3fuja_uart_path(void) {
    apply_control_profile(1);
    wait_for_mcu_boot(500);
    set_uart_baudrate(UART_BAUDRATE_9600);
    set_uart_pins(UART_TX_PC2, UART_RX_PC3);
    app_uart_init();

    diag_ctrl_idx = 1;
    diag_uart_tx_idx = 2;
    diag_uart_rx_idx = 3;
    diag_uart_baud_idx = 0;
    update_diag_swbuild();
}

uint16_t get_seq_num() {

    return seq_num;
}

void set_seq_num(uint16_t f_seq_num) {

    seq_num = f_seq_num;
}

void set_header_pkt(uint8_t *f_pkt_buff, uint8_t len, uint16_t f_seq_num, uint8_t command) {

    memset(f_pkt_buff, 0, len);
    pkt_tuya_t *out_pkt = (pkt_tuya_t*)f_pkt_buff;
    out_pkt->pkt_len = 0;
    out_pkt->f_start1 = FLAG_START1;
    out_pkt->pkt_len++;
    out_pkt->f_start2 = FLAG_START2;
    out_pkt->pkt_len++;
    out_pkt->pkt_vesion = SP_VERSION;
    out_pkt->pkt_len++;
    out_pkt->seq_num = reverse16(f_seq_num);
    out_pkt->pkt_len++;
    out_pkt->pkt_len++;
    out_pkt->command = command;
    out_pkt->pkt_len++;
}

static void set_command(command_t command, uint16_t f_seq_num, bool inc_seq_num) {

    pkt_tuya_t *out_pkt = (pkt_tuya_t*)pkt_buff;

    if (inc_seq_num) {
        f_seq_num++;
        if (f_seq_num > 0xFFF0) f_seq_num = 0;
        seq_num = f_seq_num;
    }

    set_header_pkt(pkt_buff, sizeof(pkt_buff), f_seq_num, command);

    switch(command) {
        case COMMAND00:
            break;
        case COMMAND01:
            out_pkt->len = 0;
            out_pkt->pkt_len++;
            out_pkt->pkt_len++;
            out_pkt->data[0] = checksum((uint8_t*)out_pkt, out_pkt->pkt_len++);
            add_cmd_queue(out_pkt, true);
            break;
        case COMMAND02:
            out_pkt->len = reverse16(1);
            out_pkt->pkt_len++;
            out_pkt->pkt_len++;
            out_pkt->data[0] = status_net;
            out_pkt->pkt_len++;
            out_pkt->data[1] = checksum((uint8_t*)out_pkt, out_pkt->pkt_len++);
            add_cmd_queue(out_pkt, true);
            break;
        case COMMAND03:
            out_pkt->len = 0;
            out_pkt->pkt_len++;
            out_pkt->pkt_len++;
            out_pkt->data[0] = checksum((uint8_t*)out_pkt, out_pkt->pkt_len++);
            add_cmd_queue(out_pkt, false);
            break;
        case COMMAND04:
            break;
        case COMMAND05:
            break;
        case COMMAND06:
            break;
        case COMMAND20:
            out_pkt->len = reverse16(1);
            out_pkt->pkt_len++;
            out_pkt->pkt_len++;
            if (zb_getLocalShortAddr() >= 0xFFF8)
                status_net = STATUS_NET_FREE;
            out_pkt->data[0] = status_net;
            out_pkt->pkt_len++;
            out_pkt->data[1] = checksum((uint8_t*)out_pkt, out_pkt->pkt_len++);
            add_cmd_queue(out_pkt, false);
            break;
        case COMMAND28:
            out_pkt->len = 0;
            out_pkt->pkt_len++;
            out_pkt->pkt_len++;
            out_pkt->data[0] = checksum((uint8_t*)out_pkt, out_pkt->pkt_len++);
            add_cmd_queue(out_pkt, false);
            break;
        case COMMANDXX:
//            printf("COMMMANDXX\r\n");
            out_pkt->command = 0x04;
            out_pkt->len = reverse16(5);
            out_pkt->pkt_len++;
            out_pkt->pkt_len++;
            out_pkt->data[0] = 0x03;
            out_pkt->pkt_len++;
            out_pkt->data[1] = 0x04;
            out_pkt->pkt_len++;
            out_pkt->data[2] = 0x00;
            out_pkt->pkt_len++;
            out_pkt->data[3] = 0x01;
            out_pkt->pkt_len++;
            out_pkt->data[4] = 0x01;
            out_pkt->pkt_len++;
            out_pkt->data[5] = checksum((uint8_t*)out_pkt, out_pkt->pkt_len++);
            add_cmd_queue(out_pkt, true);
            break;
        default:
            break;
    }
}

static void set_default_answer(command_t command, uint16_t f_seq_num) {

    pkt_tuya_t *out_pkt = (pkt_tuya_t*)pkt_buff;
    set_header_pkt(pkt_buff, sizeof(pkt_buff), f_seq_num, command);
    out_pkt->len = reverse16(1);
    out_pkt->pkt_len++;
    out_pkt->pkt_len++;
    out_pkt->data[0] = 0x01;
    out_pkt->pkt_len++;
    out_pkt->data[1] = checksum((uint8_t*)out_pkt, out_pkt->pkt_len++);
    add_cmd_queue(out_pkt, false);

}

static void handle_data_point(data_point_t *data_point) {

    diag_last_dp_id = data_point->dp_id;
    diag_dp_seen++;

#if UART_PRINTF_MODE && DEBUG_DP
    printf("data point id: 0x%x\r\n", data_point->dp_id);
#endif

    if (data_point->dp_id == data_point_model[DP_IDX_CO2].id &&
               data_point->dp_type == data_point_model[DP_IDX_CO2].type) {

        diag_dp_match++;
#if UART_PRINTF_MODE && DEBUG_DP
        printf("DP CO2\r\n");
#endif
        uint32_t co2 = int32_from_str(data_point->data);

        if (data_point_model[DP_IDX_CO2].local_cmd)
            data_point_model[DP_IDX_CO2].local_cmd(&co2);

    } else if (data_point->dp_id == data_point_model[DP_IDX_TEMP].id &&
                data_point->dp_type == data_point_model[DP_IDX_TEMP].type) {

        diag_dp_match++;
#if UART_PRINTF_MODE && DEBUG_DP
        printf("DP Temperature\r\n");
#endif
        int16_t temp = int32_from_str(data_point->data);

        if (data_point_model[DP_IDX_TEMP].local_cmd)
            data_point_model[DP_IDX_TEMP].local_cmd(&temp);

    } else if (data_point->dp_id == data_point_model[DP_IDX_HUM].id &&
                data_point->dp_type == data_point_model[DP_IDX_HUM].type) {

        diag_dp_match++;
#if UART_PRINTF_MODE && DEBUG_DP
        printf("DP Humidity\r\n");
#endif
        uint16_t hum = int32_from_str(data_point->data);

        if (data_point_model[DP_IDX_HUM].local_cmd)
            data_point_model[DP_IDX_HUM].local_cmd(&hum);
    } else if (data_point->dp_id == data_point_model[DP_IDX_FHYD].id &&
            data_point->dp_type == data_point_model[DP_IDX_FHYD].type) {

        diag_dp_match++;
#if UART_PRINTF_MODE && DEBUG_DP
        printf("DP Formaldehyde\r\n");
#endif
        uint32_t fhyd = int32_from_str(data_point->data);

        if (data_point_model[DP_IDX_FHYD].local_cmd)
            data_point_model[DP_IDX_FHYD].local_cmd(&fhyd);
    } else if (data_point->dp_id == data_point_model[DP_IDX_VOC].id &&
            data_point->dp_type == data_point_model[DP_IDX_VOC].type) {

        diag_dp_match++;
#if UART_PRINTF_MODE && DEBUG_DP
        printf("DP VOC\r\n");
#endif
        uint32_t voc = int32_from_str(data_point->data);

        if (data_point_model[DP_IDX_VOC].local_cmd)
            data_point_model[DP_IDX_VOC].local_cmd(&voc);
    }
}

static void handle_data_points(pkt_tuya_t *pkt) {

    uint16_t offset = 0;

    while ((offset + 4) <= pkt->len) {
        data_point_t *data_point = (data_point_t *)(pkt->data + offset);
        uint16_t dp_len = reverse16(data_point->dp_len);

        if ((offset + 4 + dp_len) > pkt->len) {
            break;
        }

        data_point->dp_len = dp_len;
        handle_data_point(data_point);
        offset += 4 + dp_len;
    }
}

static int32_t check_answerCb(void *arg) {

    if (no_answer) {
#if UART_PRINTF_MODE
        printf("no answer, uart reinit\r\n");
#endif
        app_uart_init();
        no_answer = false;

    }

    check_answerTimerEvt = NULL;
    return -1;
}

static int32_t check_answerMcuCb(void *arg) {

#if UART_PRINTF_MODE
    printf("no answer from MCU, uart reinit\r\n");
#endif

    app_uart_init();

    return 0;
}



static int32_t factory_resetCb(void *arg) {

    zb_resetDevice2FN();

    printf("reset factory 2 from timer\r\n");
    factory_reset_status = 2;

    factory_resetTimerEvt = NULL;
    return -1;
}

static int32_t factory_reset_statusCb(void *arg) {

    printf("clear reset factory status from timer\r\n");
    factory_reset_status = 0;

    return -1;
}

void uart_cmd_handler() {

    size_t load_size = 0;
    uint8_t complete = false;
    uint8_t answer_buff[DATA_MAX_LEN+24];
    pkt_tuya_t *pkt  = (pkt_tuya_t*)answer_buff;
    cmd_queue_cell_t *current_queue = NULL;

    if (first_start) {
        reset_cmd_queue();
        data_point_model_init();
        init_known_mja3fuja_uart_path();
        if (zb_getLocalShortAddr() < 0xFFF8) {
            status_net = STATUS_NET_CONNECTED;
        }
        set_command(COMMAND01, seq_num, true);
        //printf("set command first\r\n");
        check_answerMcuTimerEvt = TL_ZB_TIMER_SCHEDULE(check_answerMcuCb, NULL, TIMEOUT_1MIN30SEC);

        first_start = false;

        //only for test!!!
//        set_command(COMMANDXX, seq_num, true);
    }

    if (cmd_queue.not_need_confirm) {

        current_queue = get_first_queue(NOT_NEED_CONFIRM);

        if (current_queue) {
            if (send_command(&current_queue->pkt) == UART_TX_SUCCESS) {
                current_queue->confirm_rec = true;
                move_cmd_queue(current_queue);
            } else {
                return;
            }
        }
    }

    if (cmd_queue.need_confirm) {

        current_queue = get_first_queue(NEED_CONFIRM);

        if (current_queue) {

            if (send_command(&current_queue->pkt) == UART_TX_SUCCESS) {

                /* trying to read for 1 seconds */
                for(uint8_t i = 0; i < 100; i++ ) {
                    if (read_tuya_frame(answer_buff, &load_size, 1)) {
                        complete = true;
                        break;
                    }
#if (MODULE_WATCHDOG_ENABLE)
                    drv_wd_clear();
#endif
                    sleep_ms(10);
                }

                pkt_tuya_t *send_pkt = &current_queue->pkt;

                if (complete) {

                    no_answer = false;
                    diag_last_command = pkt->command;
                    diag_frame_ok++;

                    pkt->pkt_len = load_size;
                    uint8_t crc = checksum((uint8_t*)pkt, pkt->pkt_len-1);

    //                    printf("complete.inCRC: 0x%x, outCRC: 0x%x\r\n", crc, answer_buff[pkt->pkt_len-1]);

                    if (crc == answer_buff[pkt->pkt_len-1]) {

                        diag_crc_ok++;
                        if (pkt->command == COMMAND05 || pkt->command == COMMAND06 || pkt->command == COMMAND2C) {
                            current_queue->confirm_rec = true;
                            set_default_answer(pkt->command, reverse16(pkt->seq_num));
                            handle_data_points(pkt);
//                                if (data_point->dp_id == data_point_model[DP_IDX_SETPOINT].id ||
//                                    data_point->dp_id == data_point_model[DP_IDX_ONOFF].id ||
//                                    data_point->dp_id == data_point_model[DP_IDX_SCHEDULE].id) {
//                                    set_default_answer(COMMAND06, reverse16(pkt->seq_num));
//                                }
                        } else if (send_pkt->command == COMMAND28) {
                            current_queue->confirm_rec = true;
                        } else if (pkt->command == send_pkt->command /*&& pkt->seq_num == send_pkt->seq_num*/) {
    //                            printf("command: 0%x\r\n", pkt->command);
                            switch(pkt->command) {
                                case COMMAND01:

                                    current_queue->confirm_rec = true;

                                    uint8_t *p = pkt->data;
                                    uint16_t len = pkt->len;

                                    while(*p != ':' && len != 0) {
                                        p++;
                                        len--;
                                    }
                                    p++;
                                    p++;

                                    uint8_t *ptr = p;
                                    while(*p != '"' && len != 0) {
                                        p++;
                                        len--;
                                    }

                                    *p = 0;

                                    manuf_name = MANUF_NAME_MAX;

                                    for (uint8_t ii = 0; ii < MANUF_NAME_MAX; ii++) {
                                        for (uint8_t i = 0; i < 255; i++) {
                                            if (tuya_manuf_names[ii][i] == NULL) break;
//                                                printf("tuya_manuf_names[%d][%d]: %s\r\n", ii, i, tuya_manuf_names[ii][i]);
                                            if (strcmp(tuya_manuf_names[ii][i], (char8_t*)ptr) == 0) {
                                                manuf_name = ii;
                                                ii = MANUF_NAME_MAX;
                                                break;
                                            }
                                        }
                                    }

                                    if (manuf_name == MANUF_NAME_MAX) {
#if UART_PRINTF_MODE
                                        printf("Known Tuya signature not found. Use default\r\n");
#endif
                                        manuf_name = MANUF_NAME_1;

                                    } else {
#if UART_PRINTF_MODE
                                        printf("Tuya signature found: \"%s\"\r\n", ptr);
#endif
                                    }

#if 0
                                    /* Only for test */
    //                                    manuf_name = MANUF_NAME_1;
    //                                    manuf_name = MANUF_NAME_2;
    //                                    manuf_name = MANUF_NAME_3;
    //                                    manuf_name = MANUF_NAME_4;
    //                                    manuf_name = MANUF_NAME_5;

#endif

#if UART_PRINTF_MODE
                                    printf("Use modelId: %s\r\n", zb_modelId_arr[manuf_model_id_arr[manuf_name]]+1);
#endif

                                    zcl_setAttrVal(APP_ENDPOINT1, ZCL_CLUSTER_GEN_BASIC, ZCL_ATTRID_BASIC_MODEL_ID, zb_modelId_arr[manuf_model_id_arr[manuf_name]]);
                                    data_point_model = data_point_model_arr[manuf_name];
                                    set_command(COMMAND02, seq_num, true);
                                    startup_net_sync_sent = true;

                                    break;
                                case COMMAND02:
                                    current_queue->confirm_rec = true;
                                    if (status_net == STATUS_NET_CONNECTED) {
                                        set_command(COMMAND28, seq_num, true);
                                    }
                                    break;
                                case COMMAND00:
                                case COMMAND03:
                                case COMMAND04:
                                case COMMAND05:
                                case COMMAND06:
                                case COMMAND2C:
                                    break;
                                case COMMAND28:
                                    current_queue->confirm_rec = true;
                                    break;
                                default:
                                    break;
                            }
                        }

                    } else {
#if UART_PRINTF_MODE
                        printf("Error CRC. inCRC: 0x%x, outCRC: 0x%x\r\n", crc, answer_buff[pkt->pkt_len-1]);
#endif
                    }

                } else {
#if UART_PRINTF_MODE
                    printf("no complete\r\n");
#endif
                    current_queue->confirm_rec = false;
                    if (answer_count++ == 5) {
                        answer_count = 0;
                        if (send_pkt->command == COMMAND02 && status_net == STATUS_NET_CONNECTED) {
                            set_command(COMMAND28, seq_num, true);
                        }
                        current_queue->confirm_rec = true;
                    }

                    if (!no_answer) {
                        no_answer = true;

                        if (check_answerTimerEvt) {
                            TL_ZB_TIMER_CANCEL(&check_answerTimerEvt);
                        }
                        check_answerTimerEvt = TL_ZB_TIMER_SCHEDULE(check_answerCb, NULL, TIMEOUT_15SEC);
                    }


                    if (send_pkt->command == COMMAND01) {
                        uint32_t baudrate = get_uart_baudrate();
                        if (baudrate == UART_BAUDRATE_9600) {
                            set_uart_baudrate(UART_BAUDRATE_115200);
                        } else {
                            set_uart_baudrate(UART_BAUDRATE_9600);
                        }
                        app_uart_init();
                    }
                }
            } else {
                return;
            }
        }
        move_cmd_queue(current_queue);
    }

#if (MODULE_WATCHDOG_ENABLE)
    drv_wd_clear();
#endif

    if (available_ring_buff() /* && get_queue_len_ring_buff() >= 8*/) {
        read_tuya_frame(answer_buff, &load_size, 10);

        if (load_size == pkt->len + 9) {

            if (check_answerMcuTimerEvt) {
                TL_ZB_TIMER_CANCEL(&check_answerMcuTimerEvt);
            }
            check_answerMcuTimerEvt = TL_ZB_TIMER_SCHEDULE(check_answerMcuCb, NULL, TIMEOUT_1MIN30SEC);

            pkt->pkt_len = load_size;
            uint8_t crc = checksum((uint8_t*)pkt, pkt->pkt_len-1);
            diag_last_command = pkt->command;
            diag_frame_ok++;
            if (crc == answer_buff[pkt->pkt_len-1]) {
                diag_crc_ok++;
                pkt->seq_num = reverse16(pkt->seq_num);

                if (pkt->command == COMMAND03) {
                    /* Reset Factory */
#if UART_PRINTF_MODE // && DEBUG_CMD
                    printf("command 0x03. Factory Reset\r\n");
#endif
                    if (zb_getLocalShortAddr() >= 0xFFF8) {
                        if (!factory_reset_status) {
                            factory_reset_status = 2;
                            printf("reset factory 2. zb_getLocalShortAddr(): 0x%x\r\n", zb_getLocalShortAddr());
                            zb_resetDevice2FN();
                            TL_ZB_TIMER_SCHEDULE(factory_reset_statusCb, NULL, TIMEOUT_15SEC);
                        }
                    } else if (!factory_reset_status) {
                        factory_reset_status = 1;
                        if (factory_resetTimerEvt) {
                            TL_ZB_TIMER_CANCEL(&factory_resetTimerEvt);
                        } else {
                            printf("reset factory 1. zb_getLocalShortAddr(): 0x%x\r\n", zb_getLocalShortAddr());
                            zb_resetDevice2FN();
                        }
                        factory_resetTimerEvt = TL_ZB_TIMER_SCHEDULE(factory_resetCb, NULL, TIMEOUT_3SEC);
                    }

                    set_command(pkt->command, pkt->seq_num, false);
//                } else if (pkt->command == COMMAND02) {
//                    printf("input COMMAND02\r\n");

//                } else if (pkt->command == COMMAND24) {
//#if UART_PRINTF_MODE
//                    printf("command 0x24. Sync Time\r\n");
//#endif
//                    if (get_time_sent()) {
//                        set_command(pkt->command, pkt->seq_num, false);
//                    } else {
//                        set_default_answer(COMMAND06, pkt->seq_num);
//                    }

                } else if (pkt->command == COMMAND20) {
                    set_command(pkt->command, pkt->seq_num, false);
                } else if (pkt->command == COMMAND05 || pkt->command == COMMAND06 || pkt->command == COMMAND2C) {
#if UART_PRINTF_MODE && DEBUG_CMD
                    printf("command 0x05/0x06/0x2C. Report DP data\r\n");
#endif
                    if (pkt->len >= 5) {
                        set_default_answer(pkt->command, pkt->seq_num);
                        handle_data_points(pkt);
                    }
                }
            }
        }
    }

    update_diag_swbuild();
}

void set_status_net(status_net_t new_status) {

    if (new_status != status_net) {
        status_net = new_status;
        if (!first_start && !startup_net_sync_sent && status_net == STATUS_NET_CONNECTED) {
            set_command(COMMAND02, seq_num, true);
            startup_net_sync_sent = true;
        }
    }

    if (status_net == STATUS_NET_CONNECTED) {
        factory_reset_cnt = 0;
        factory_reset_status = 0;
    }

}

