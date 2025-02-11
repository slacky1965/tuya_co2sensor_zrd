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
static uint8_t      factory_reset_cnt = 0;
static uint8_t      factory_reset_status = 0;
static ev_timer_event_t *factory_resetTimerEvt = NULL;
static ev_timer_event_t *check_answerTimerEvt = NULL;
static ev_timer_event_t *check_answerMcuTimerEvt = NULL;

cmd_queue_t cmd_queue = {0};

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

    cmd_queue_cell_t *current_queue = NULL;

    for (uint8_t i = 0; i < CMD_QUEUE_CELL_MAX; i++) {
        if (cmd_queue.cmd_queue[i].used) {
            if (cmd_queue.cmd_queue[i].confirm_need == confirm) {
                current_queue = &cmd_queue.cmd_queue[i];
            }
        }
    }

    return current_queue;
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

    return ret;
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

    factory_reset_status = 2;

    factory_resetTimerEvt = NULL;
    return -1;
}

void uart_cmd_handler() {

    size_t load_size = 0;
    uint8_t ch, complete = false;
    uint8_t answer_buff[DATA_MAX_LEN+24];
    pkt_tuya_t *pkt  = (pkt_tuya_t*)answer_buff;
    data_point_t *data_point = (data_point_t*)pkt->data;
    cmd_queue_cell_t *current_queue = NULL;

    if (first_start) {
        reset_cmd_queue();
        set_command(COMMAND01, seq_num, true);
        printf("set command first\r\n");
        data_point_model_init();
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
        if (!available_ring_buff()) {

            current_queue = get_first_queue(NEED_CONFIRM);

            if (current_queue) {

                if (send_command(&current_queue->pkt) == UART_TX_SUCCESS) {

                    /* trying to read for 1 seconds */
                    for(uint8_t i = 0; i < 100; i++ ) {
                        load_size = 0;
                        if (available_ring_buff() /*&& get_queue_len_ring_buff() >= 8*/) {
                            while (available_ring_buff() && load_size < (DATA_MAX_LEN + 8)) {
                                ch = read_byte_from_ring_buff();

                                if (load_size == 0) {
                                    if (ch != FLAG_START1) {
                                        continue;
                                    }
                                } else if (load_size == 1) {
                                    if (ch != FLAG_START2) {
                                        load_size = 0;
                                        continue;
                                    }
                                }

                                answer_buff[load_size++] = ch;

                                if (load_size == 2) {

                                    load_size += read_bytes_from_buff(answer_buff+load_size, 6);

                                    if (load_size == 8) {
                                        pkt->len = reverse16(pkt->len);
                                        load_size += read_bytes_from_buff(answer_buff+load_size, pkt->len+1);
                                        i = 100;
                                        complete = true;
                                        break;
                                    } else {
                                        load_size = 0;
                                        continue;
                                    }

                                }
                            }
                        }
#if (MODULE_WATCHDOG_ENABLE)
                        drv_wd_clear();
#endif
                        sleep_ms(10);
                    }

                    pkt_tuya_t *send_pkt = &current_queue->pkt;

                    if (complete) {

                        no_answer = false;

                        pkt->pkt_len = load_size;
                        uint8_t crc = checksum((uint8_t*)pkt, pkt->pkt_len-1);

    //                    printf("complete.inCRC: 0x%x, outCRC: 0x%x\r\n", crc, answer_buff[pkt->pkt_len-1]);

                        if (crc == answer_buff[pkt->pkt_len-1]) {

                            if (send_pkt->command == COMMAND04 && pkt->command == COMMAND06 /*&& pkt->seq_num == send_pkt->seq_num*/) {
                                current_queue->confirm_rec = true;
                                set_default_answer(COMMAND06, reverse16(pkt->seq_num));
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
    //                                            printf("tuya_manuf_names[%d][%d]: %s\r\n", ii, i, tuya_manuf_names[ii][i]);
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
                                        printf("Use modelId: %s\r\n", zb_modelId_arr[manuf_name]+1);
#endif

                                        zcl_setAttrVal(APP_ENDPOINT1, ZCL_CLUSTER_GEN_BASIC, ZCL_ATTRID_BASIC_MODEL_ID, zb_modelId_arr[manuf_name]);
                                        data_point_model = data_point_model_arr[manuf_name];

//                                        if (manuf_name == MANUF_NAME_5 || manuf_name == MANUF_NAME_6) {
//                                            set_command(COMMAND28, seq_num, true);
//                                        }

                                        break;
                                    case COMMAND02:
                                        current_queue->confirm_rec = true;
                                        break;
                                    case COMMAND00:
                                    case COMMAND03:
                                    case COMMAND04:
                                    case COMMAND05:
                                    case COMMAND06:
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
        }
        move_cmd_queue(current_queue);
    }

#if (MODULE_WATCHDOG_ENABLE)
    drv_wd_clear();
#endif

    if (available_ring_buff() /* && get_queue_len_ring_buff() >= 8*/) {
        load_size = 0;
        while (available_ring_buff() && load_size < (DATA_MAX_LEN + 8)) {
            ch = read_byte_from_ring_buff();

            if (load_size == 0) {
                if (ch != FLAG_START1) {
                    continue;
                }
            } else if (load_size == 1) {
                if (ch != FLAG_START2) {
                    load_size = 0;
                    continue;
                }
            }

            answer_buff[load_size++] = ch;

            if (load_size == 2) {

                load_size += read_bytes_from_buff(answer_buff+load_size, 6);

                if (load_size == 8) {
                    pkt->len = reverse16(pkt->len);
                    load_size += read_bytes_from_buff(answer_buff+load_size, pkt->len+1);
                    break;
                } else {
                    load_size = 0;
                    continue;
                }

            }
        }

        if (load_size == pkt->len + 9) {

            if (check_answerMcuTimerEvt) {
                TL_ZB_TIMER_CANCEL(&check_answerMcuTimerEvt);
            }
            check_answerMcuTimerEvt = TL_ZB_TIMER_SCHEDULE(check_answerMcuCb, NULL, TIMEOUT_1MIN30SEC);

            pkt->pkt_len = load_size;
            uint8_t crc = checksum((uint8_t*)pkt, pkt->pkt_len-1);
            if (crc == answer_buff[pkt->pkt_len-1]) {
                pkt->seq_num = reverse16(pkt->seq_num);

                if (pkt->command == COMMAND03) {
                    /* Reset Factory */
#if UART_PRINTF_MODE // && DEBUG_CMD
                    printf("command 0x03. Factory Reset\r\n");
#endif
                    if (factory_reset_cnt == 0 && factory_reset_status != 2) {
                        printf("FN1\r\n");
                        zb_resetDevice2FN();
                        factory_reset_cnt++;
                        factory_reset_status = 1;
                        factory_resetTimerEvt = TL_ZB_TIMER_SCHEDULE(factory_resetCb, NULL, TIMEOUT_3SEC);
                    } else {
                        printf("FN2\r\n");
                        if (factory_resetTimerEvt && factory_reset_status == 1) {
                            TL_ZB_TIMER_CANCEL(&factory_resetTimerEvt);
                        }
                        if (factory_reset_status == 1) {
                            factory_resetTimerEvt = TL_ZB_TIMER_SCHEDULE(factory_resetCb, NULL, TIMEOUT_3SEC);
                        }
                    }
                    set_command(pkt->command, pkt->seq_num, false);
//                    zb_factoryReset();
//                    TL_ZB_TIMER_SCHEDULE(delayedMcuResetCb, NULL, TIMEOUT_3SEC);
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
                } else if (pkt->command == COMMAND06) {
#if UART_PRINTF_MODE && DEBUG_CMD
                    printf("command 0x06. Report DP data\r\n");
#endif
                    if (pkt->len >= 5) {
                        /* data point used */
                        data_point->dp_len = reverse16(data_point->dp_len);

#if UART_PRINTF_MODE && DEBUG_DP
                        printf("data point id: 0x%x\r\n", data_point->dp_id);
#endif

                        set_default_answer(pkt->command, pkt->seq_num);

                        if (data_point->dp_id == data_point_model[DP_IDX_CO2].id &&
                                   data_point->dp_type == data_point_model[DP_IDX_CO2].type) {

#if UART_PRINTF_MODE && DEBUG_DP
                            printf("DP CO2\r\n");
#endif
                            uint32_t co2 = int32_from_str(data_point->data);

                            if (data_point_model[DP_IDX_CO2].local_cmd)
                                data_point_model[DP_IDX_CO2].local_cmd(&co2);
                        }
                    }
                }
            }
        }
    }
}

void set_status_net(status_net_t new_status) {

    if (new_status != status_net) {
        status_net = new_status;
//        set_command(COMMAND02, seq_num, true);
    }

    if (status_net == STATUS_NET_CONNECTED) {
        factory_reset_cnt = 0;
        factory_reset_status = 0;
    }

}

