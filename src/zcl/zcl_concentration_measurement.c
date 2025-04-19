#include "zcl_include.h"
#include "app_main.h"


#ifdef ZCL_CO2_MEASUREMENT

_CODE_ZCL_ status_t zcl_co2_measurement_register(u8 endpoint, u16 manuCode, u8 attrNum, const zclAttrInfo_t attrTbl[], cluster_forAppCb_t cb)
{
    return zcl_registerCluster(endpoint, ZCL_CLUSTER_MS_CO2_MEASUREMENT, manuCode, attrNum, attrTbl, NULL, cb);
}

#endif

#ifdef ZCL_FHYD_MEASUREMENT

_CODE_ZCL_ status_t zcl_fhyd_measurement_register(u8 endpoint, u16 manuCode, u8 attrNum, const zclAttrInfo_t attrTbl[], cluster_forAppCb_t cb)
{
    return zcl_registerCluster(endpoint, ZCL_CLUSTER_MS_FHYD_MEASUREMENT, manuCode, attrNum, attrTbl, NULL, cb);
}

#endif
