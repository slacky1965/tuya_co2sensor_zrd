#ifndef SRC_INCLUDE_APP_CO2SENSOR_H_
#define SRC_INCLUDE_APP_CO2SENSOR_H_

#define ZB_MODELID_ARR_NUM          16
#define ZB_MODELID_SIZE             18
#define ZB_MODELID_FULL_SIZE        ZB_MODELID_SIZE+2


extern data_point_st_t data_point_model1[DP_IDX_MAXNUM];
extern uint8_t remote_cmd_pkt_buff[DATA_MAX_LEN+12];
extern uint8_t zb_modelId_arr[ZB_MODELID_ARR_NUM][ZB_MODELID_FULL_SIZE];

/*
 * common functions local_cmd
 */
void local_cmd_co2(void *args);

/*
 *  local_cmd for signarure
 *  "ogkdpgy2"
 *
 *  model1 - name_1
 */
#define local_cmd_co2_1        local_cmd_co2


#endif /* SRC_INCLUDE_APP_CO2SENSOR_H_ */
