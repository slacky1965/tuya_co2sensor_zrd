#include "app_main.h"

/* data point for manufacturer id -
 * "mja3fuja"
 *
 * id, type, len, divisor, remote_commands_functionCb, local_commands_functionCb
*/

data_point_st_t data_point_model3[DP_IDX_MAXNUM] = {
        {DP_TYPE3_ID_16, DP_VAL,  4,    1,  NULL, local_cmd_co2_2},             // co2
        {DP_TYPE3_ID_12, DP_VAL,  4,    10, NULL, local_cmd_temperature_2},     // temperature
        {DP_TYPE3_ID_13, DP_VAL,  4,    10, NULL, local_cmd_humidity_2},        // humidity
        {DP_TYPE3_ID_15, DP_VAL,  4,    1,  NULL, local_cmd_voc_2},             // voc
        {DP_TYPE3_ID_02, DP_VAL,  4,    1,  NULL, local_cmd_formaldehyde_2},    // formaldehyde
};
