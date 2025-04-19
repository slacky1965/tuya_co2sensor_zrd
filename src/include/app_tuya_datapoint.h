#ifndef SRC_INCLUDE_APP_TUYA_DATAPOINT_H_
#define SRC_INCLUDE_APP_TUYA_DATAPOINT_H_

#define DP_TYPE_ID_00 0x00

/* data point for manufacturer id -
 * "ogkdpgy20"
 *
 * type1 (model1)
*/
typedef enum {
    DP_TYPE1_ID_02    = 0x02,     // ppm
} data_point_id_type1_t;

/* data point for manufacturer id -
 * "yvx5lh6k"
 *
 * type2 (model2)
*/
typedef enum {
    DP_TYPE2_ID_02    = 0x02,     // CO2 ppm
    DP_TYPE2_ID_12    = 0x12,     // Temperature
    DP_TYPE2_ID_13    = 0x13,     // Humidity %
    DP_TYPE2_ID_15    = 0x15,     // VOC
    DP_TYPE2_ID_16    = 0x16,     // Formaldehyde
} data_point_id_type2_t;

typedef enum {
    MANUF_NAME_1 = 0,
    MANUF_NAME_2,
    MANUF_NAME_MAX
} manuf_name_t;

typedef enum {
    DP_IDX_CO2  = 0,
    DP_IDX_TEMP,
    DP_IDX_HUM,
    DP_IDX_VOC,
    DP_IDX_FHYD,
    DP_IDX_MAXNUM
} data_point_idx_t;

/* Data Point
 *  Type    Data type   Data length     Description
    0x00    Raw         Custom          Raw data type in hex format, with customizable bytes.
    0x01    Boolean     1               Boolean data type. Valid values include 0x00 and 0x01.
    0x02    Value       4               Value data type, for example, 1, 23, and 104.
    0x03    String      Custom          Custom string.
    0x04    Enum        1               Enum data type, ranging from 0 to 255.
    0x05    Bitmap      1/2/4           Bitmap data type, used to report error codes.
 */

typedef enum {
    DP_RAW = 0,
    DP_BOOL,
    DP_VAL,
    DP_STR,
    DP_ENUM,
    DP_BITMAP
} dp_type_t;

typedef struct __attribute__((packed)) {
    uint8_t  dp_id;
    uint8_t  dp_type;
    uint16_t dp_len;
    uint8_t  data[DATA_MAX_LEN-8];
} data_point_t;

typedef void (*remote_cmd_t)(void *args);
typedef void (*local_cmd_t)(void *args);

typedef struct {
    uint8_t         id;
    uint8_t         type;
    uint16_t        len;
    uint16_t        divisor;
    remote_cmd_t    remote_cmd;
    local_cmd_t     local_cmd;
} data_point_st_t;

extern uint8_t manuf_name;
extern data_point_st_t *data_point_model;
extern const char8_t **tuya_manuf_names[];

void data_point_model_init();
data_point_st_t *data_point_model_arr[DP_IDX_MAXNUM];

#endif /* SRC_INCLUDE_APP_TUYA_DATAPOINT_H_ */
