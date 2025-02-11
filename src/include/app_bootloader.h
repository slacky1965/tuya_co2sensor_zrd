#ifndef SRC_INCLUDE_APP_BOOTLOADER_H_
#define SRC_INCLUDE_APP_BOOTLOADER_H_

#define BOOTLOADER_ADDR         0x0
#define IMAGE_OTA_ADDR1         0x70000
#define IMAGE_OTA_ADDR2         FLASH_ADDR_OF_OTA_IMAGE
#define BOOTLOAD_MARKER         ZCL_BASIC_MFG_NAME
#define BOOTLOAD_MARKER_ADDR    0x7ff0
#define BOOTLOAD_MARKER_SECTOR  0x7000

#ifndef BOOT_SIZE
#define BOOT_SIZE               20580
#endif

#define TL_MAGIC                "KNLT"
#define TL_SIG                  0x025D

typedef struct {
    uint16_t reset;
    uint32_t file_version;
    uint16_t sig;
    uint8_t  magic[4];
    uint32_t addr_bin_code;
    uint16_t irq;
    uint16_t manuf_code;
    uint16_t image_type;
    uint16_t dummy1;
    uint32_t bin_size;
    uint32_t dummy2;
} tl_header_t;

void bootloader_check();

#endif /* SRC_INCLUDE_APP_BOOTLOADER_H_ */
