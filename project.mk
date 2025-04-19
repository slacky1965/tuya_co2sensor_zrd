# Add inputs and outputs from these tool invocations to the build variables
OUT_DIR += \
/$(SRC_PATH) \
/$(SRC_PATH)/zcl \
/$(SRC_PATH)/common
 
OBJS += \
$(OUT_PATH)/$(SRC_PATH)/common/main.o \
$(OUT_PATH)/$(SRC_PATH)/zcl/zcl_concentration_measurement.o \
$(OUT_PATH)/$(SRC_PATH)/zcl/zcl_relative_humidity_measurement.o \
$(OUT_PATH)/$(SRC_PATH)/zcl/zcl_analog_input.o \
$(OUT_PATH)/$(SRC_PATH)/app_tuya_datapoint.o \
$(OUT_PATH)/$(SRC_PATH)/app_uart_tuya.o \
$(OUT_PATH)/$(SRC_PATH)/app_uart.o \
$(OUT_PATH)/$(SRC_PATH)/app_co2sensor.o \
$(OUT_PATH)/$(SRC_PATH)/app_co2sensor_model1.o \
$(OUT_PATH)/$(SRC_PATH)/app_co2sensor_model2.o \
$(OUT_PATH)/$(SRC_PATH)/app_endpoint_cfg.o \
$(OUT_PATH)/$(SRC_PATH)/app_utility.o \
$(OUT_PATH)/$(SRC_PATH)/app_bootloader.o \
$(OUT_PATH)/$(SRC_PATH)/app_main.o \
$(OUT_PATH)/$(SRC_PATH)/zb_appCb.o \
$(OUT_PATH)/$(SRC_PATH)/zcl_appCb.o

# Each subdirectory must supply rules for building sources it contributes
$(OUT_PATH)/$(SRC_PATH)/%.o: $(SRC_PATH)/%.c 
	@echo 'Building file: $<'
	@$(CC) $(GCC_FLAGS) $(INCLUDE_PATHS) -c -o"$@" "$<"


