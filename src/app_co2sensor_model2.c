#include "app_main.h"

/* data point for manufacturer id -
 * "yvx5lh6k"
 *
 * id, type, len, divisor, remote_commands_functionCb, local_commands_functionCb
*/

data_point_st_t data_point_model2[DP_IDX_MAXNUM] = {
        {DP_TYPE2_ID_02, DP_VAL,  4,    1,  NULL, local_cmd_co2_2},                                // co2
        {DP_TYPE2_ID_12, DP_VAL,  4,    10, NULL, local_cmd_temperature_2},                        // temperature
        {DP_TYPE2_ID_13, DP_VAL,  4,    10, NULL, local_cmd_humidity_2},                           // humidity
        {DP_TYPE2_ID_15, DP_VAL,  4,    1,  NULL, local_cmd_voc_2},                                // voc
        {DP_TYPE2_ID_16, DP_VAL,  4,    1,  NULL, local_cmd_formaldehyde_2},                        // formaldehyde
};


void local_cmd_temperature_2(void *args) {

    int16_t *temp = (int16_t*)args;
    uint16_t divisor = 1;

    if (data_point_model[DP_IDX_TEMP].divisor == 10) {
        divisor = 10;
    } else if (data_point_model[DP_IDX_TEMP].divisor == 100) {
        divisor = 100;
    }

    *temp *= divisor;

    zcl_setAttrVal(APP_ENDPOINT1, ZCL_CLUSTER_MS_TEMPERATURE_MEASUREMENT, ZCL_TEMPERATURE_MEASUREMENT_ATTRID_MEASUREDVALUE, (uint8_t*)temp);

#if UART_PRINTF_MODE && DEBUG_CMD
            printf("Temperature: %d\r\n", *temp);
#endif

}


void local_cmd_humidity_2(void *args) {

    uint16_t *hum = (uint16_t*)args;
    uint16_t divisor = 1;

    if (data_point_model[DP_IDX_HUM].divisor == 10) {
        divisor = 10;
    } else if (data_point_model[DP_IDX_HUM].divisor == 100) {
        divisor = 100;
    }

    *hum *= divisor;

    zcl_setAttrVal(APP_ENDPOINT1, ZCL_CLUSTER_MS_RELATIVE_HUMIDITY, ZCL_ATTRID_HUMIDITY_MEASUREDVALUE, (uint8_t*)hum);

#if UART_PRINTF_MODE && DEBUG_CMD
            printf("Humidity: %d\r\n", *hum);
#endif

}

void local_cmd_voc_2(void *args) {

    uint32_t *voc = (uint32_t*)args;
    uint16_t divisor = 1;

    if (data_point_model[DP_IDX_VOC].divisor == 10) {
        divisor = 10;
    } else if (data_point_model[DP_IDX_VOC].divisor == 100) {
        divisor = 100;
    }

    *voc *= divisor;

    float attrVoc = (float)*voc/1000000;

    zcl_setAttrVal(APP_ENDPOINT1, ZCL_CLUSTER_GEN_ANALOG_INPUT_BASIC, ZCL_ANALOG_INPUT_ATTRID_PRESENT_VALUE, (uint8_t*)&attrVoc);

//    uint8_t *b = (uint8_t*)&attrVoc;

//    printf("VOC PPM hex: 0x%02x", b[3]);
//    printf("%02x", b[2]);
//    printf("%02x", b[1]);
//    printf("%02x\r\n", b[0]);

#if UART_PRINTF_MODE && DEBUG_CMD
            printf("VOC PPM: %d\r\n", *voc);
#endif

}


void local_cmd_formaldehyde_2(void *args) {

    int32_t *fhyd = (int32_t*)args;
    uint16_t divisor = 1;

    if (data_point_model[DP_IDX_CO2].divisor == 10) {
        divisor = 10;
    } else if (data_point_model[DP_IDX_CO2].divisor == 100) {
        divisor = 100;
    }

    *fhyd *= divisor;

    float attrFhyd = (float)*fhyd/100000000;

    zcl_setAttrVal(APP_ENDPOINT1, ZCL_CLUSTER_MS_FHYD_MEASUREMENT, ZCL_FHYD_MEASUREMENT_ATTRID_MEASUREDVALUE, (uint8_t*)&attrFhyd);

#if UART_PRINTF_MODE && DEBUG_CMD
            printf("PPM: %d\r\n", *fhyd);
#endif

}



