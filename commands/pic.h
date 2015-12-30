//*****************************************************************************
//
// Disclosure:
// Copyright(C) 2010 Systems and Software Enterprises, Inc. (dba IMS)
// ALL RIGHTS RESERVED.
// The contents of this medium may not be reproduced in whole or part without
// the written consent of IMS, except as authorized by section 117 of the U.S.
// Copyright law.
//*****************************************************************************

#ifndef CMD_PIC_H_
#define CMD_PIC_H_


#define SOH 0x01
#define STX 0x02
#define ETX 0x03
#define EOT 0x04
#define ACK 0x06
#define BSP 0x08
#define DLE 0x10
#define NAK 0x15
#define CAN 0x18
#define EOF 0x1A        /* ^Z for DOS officionados */


// Message ID values supported by the PIC
//   This enum is taken without modification from:
//   ravesvn\projects\application\sdu\RduPic\src\RduPicCommand.h

enum MessageId {
    CMD_STATUS           = 0xA0,
    CMD_SW_WDT           = 0xA1,
    CMD_PET_WDT          = 0xA2,
    CMD_DDS_EEPROM       = 0xA3,
    CMD_PIC_EEPROM       = 0xA4,
    CMD_RS_485           = 0xA5,
    CMD_LCD_BACKLIGHT    = 0xA6,
    CMD_PIC_RESET        = 0xA7,
    CMD_RESET_REASON     = 0xA8,
    CMD_PWR_LED          = 0xA9,
    CMD_SET_APP_BUTTON   = 0xAA,
    CMD_WIFI             = 0xAB,
    CMD_SEND_STATUS      = 0xAC,
    CMD_SET_PIC_CONSOLE  = 0xAD,
    CMD_SWITCH_EEPROM    = 0xAE,
    CMD_RESET_ETH_SWITCH = 0xAF,
    CMD_JUMP_TO_BTLDR    = 0xB0,
    CMD_BOOTLOADER       = 0xB1,
    CMD_REQ_IP_ADDR      = 0xB2,
    CMD_SUCCESSFUL_BOOT  = 0xB3,
    CMD_TIMING_TEST		 = 0xB4,
    CMD_UPDATE_BOOT_PROGRESS_CODE = 0xB5,
    CMD_REQ_COPPER_REV   = 0xB6,
    CMD_USB_BOOT_ENABLE	 = 0xB7,
    CMD_LCD_BOOT_ENABLE	 = 0xBE,

    RSP_STATUS           = 0xC0,
    RSP_SW_WDT           = 0xC1,
    RSP_PET_WDT          = 0xC2,
    RSP_DDS_EEPROM       = 0xC3,
    RSP_PIC_EEPROM       = 0xC4,
    RSP_RS_485           = 0xC5,
    RSP_LCD_BACKLIGHT    = 0xC6,
    RSP_PIC_RESET        = 0xC7,
    RSP_RESET_REASON     = 0xC8,
    RSP_PIC_PWR_LED      = 0xC9,
    RSP_SET_APP_BUTTON   = 0xCA,
    RSP_WIFI             = 0xCB,
    RSP_SEND_STATUS      = 0xCC,
    RSP_SET_PIC_CONSOLE  = 0xCD,
    RSP_SWITCH_EEPROM    = 0xCE,
    RSP_RESET_ETH_SWITCH = 0xCF,
    RSP_JUMP_TO_BTLDR    = 0xD0,
    RSP_BOOTLOADER       = 0xD1,
    RSP_REQ_IP_ADDR      = 0xD2,
    RSP_SUCCESSFUL_BOOT  = 0xD3,
    RSP_TIMING_TEST      = 0xD4,
    RSP_UPDATE_BOOT_PROGRESS_CODE = 0xD5,
    RSP_REQ_COPPER_REV  = 0xD6,
    RSP_USB_BOOT_ENABLE = 0xDE,
    RSP_LCD_BOOT_ENABLE = 0xDE,

    CMD_EVNT_BUTTON_PRESS = 0xE0,
    RSP_EVNT_BUTTON_PRESS = 0xE1,
    CMD_EVNT_ORIENTATION  = 0xE2,
    RSP_EVNT_ORIENTATION  = 0xE3,
    CMD_EVNT_DTMF_SIGNAL  = 0xE4,
    RSP_EVNT_DTMF_SIGNAL  = 0xE5,
    CMD_EVNT_STATUS       = 0xE6,
    RSP_EVNT_STATUS       = 0xE7,
    CMD_EVNT_WATCHDOG     = 0xE8,
    RSP_EVNT_WATCHDOG     = 0XE9
};

enum bootProgress {
	BOOT_PROGRESS_UBOOT_DRIVERS_LOADED	= 0x0001,
	BOOT_PROGRESS_UBOOT_FULLY_LOADED	= 0x0002,
	BOOT_PROGRESS_UBOOT_LOADED_KERNEL 	= 0x0003,
	BOOT_PROGRESS_UBOOT_JUMP_TO_KERNEL 	= 0x0004
};

#define PIC_PAGE_SIZE                   32

#define PIC_EEPROM_LAST_READABLE_PAGE   6
#define PIC_EEPROM_LAST_PAGE            511

#define PIC_EEPROM_CRC_PAGE__FIRST      0
#define PIC_EEPROM_CRC_PAGE__LAST       3

#define PIC_EEPROM_MX51_EEPROM_BACKUP_START    20
#define PIC_EEPROM_MX51_EEPROM_BACKUP_END      23

#define PIC_EEPROM_MX51_SPINOR_BACKUP_START    128
#define PIC_EEPROM_MX51_SPINOR_BACKUP_END      160

#define PIC_MFG_DATE_PAGE               0
#define PIC_MFG_DATE_LEN_OFFSET         16
#define PIC_MFG_DATE_OFFSET             17
#define PIC_MFG_DATE_MAX_LEN            8

#define PIC_PART_NUM_PAGE               1
#define PIC_PART_NUM_LEN_OFFSET         0
#define PIC_PART_NUM_OFFSET             1
#define PIC_PART_NUM_MAX_LEN            15

#define PIC_SERIAL_NUM_PAGE             3
#define PIC_SERIAL_NUM_LEN_OFFSET       0
#define PIC_SERIAL_NUM_OFFSET           1
#define PIC_SERIAL_NUM_MAX_LEN          15

#define PIC_VOLTAGE_STAT_PAGE           5
#define PIC_VOLTAGE_STAT_OFFSET         0
#define PIC_VOLTAGE_STATE_SIZE          1
#define VOLTAGE_3_3_MASK                0x01
#define VOLTAGE_5_MASK                  0x02
#define VOLTAGE_12_MASK                 0x04
#define VOLTAGE_28_MASK                 0x08

#define PIC_SW_RST_REASON_PAGE          5
#define PIC_SW_RST_REASON_OFFSET        1
#define PIC_SW_RST_REASON_SIZE          1

#define PIC_BOOT_SRC_PAGE               4
#define PIC_BOOT_SRC_OFFSET             3
#define PIC_BOOT_SRC_SIZE               1

#define PIC_LIMIT_WD_RST_TIME_PAGE      4
#define PIC_LIMIT_WD_RST_TIME_OFFSET    4
#define PIC_LIMIT_WD_RST_TIME_SIZE      2

#define PIC_DEFAULT_PWR_LED_PAGE        4
#define PIC_DEFAULT_PWR_LED_OFFSET      6
#define PIC_DEFUALT_PWR_LED_SIZE        1

#define PIC_DEFAULT_RING_LED_PAGE       4
#define PIC_DEFAULT_RING_LED_OFFSET     7
#define PIC_DEFUALT_RING_LED_SIZE       1

#define PIC_MX51_NEVER_BOOTED_PAGE      4
#define PIC_MX51_NEVER_BOOTED_OFFSET    11
#define PIC_MX51_NEVER_BOOTED_SIZE      1

#define PIC_MAX_LCD_BRIGHT_PAGE         4
#define PIC_MAX_LCD_BRIGHT_OFFSET       9
#define PIC_MAX_LCD_BRIGHT_SIZE         1

#define PIC_MIN_LCD_BRIGHT_PAGE         4
#define PIC_MIN_LCD_BRIGHT_OFFSET       8
#define PIC_MIN_LCD_BRIGHT_SIZE         1

#define PIC_INIT_WD_TO_PAGE             4
#define PIC_INIT_WD_TO_OFFSET           1
#define PIC_INIT_WD_TO_SIZE             2

#define PIC_LCD_BL_PWM_FREQ_PAGE        4
#define PIC_LCD_BL_PWM_FREQ_OFFSET      12
#define PIC_LCD_BL_PWM_FREQ_SIZE        2

#define PIC_MAX_FAILED_BOOTS_PAGE       4
#define PIC_MAX_FAILED_BOOTS_OFFSET     14
#define PIC_MAX_FAILED_BOOTS_SIZE       2

#define PIC_CUR_FAILED_BOOTS_PAGE       4
#define PIC_CUR_FAILED_BOOTS_OFFSET     16
#define PIC_CUR_FAILED_BOOTS_SIZE       2

#define PIC_BOOT_STATE_PAGE             4
#define PIC_BOOT_STATE_OFFSET           18
#define PIC_BOOT_STATE_SIZE             2

#define SPI_NOR_SHADOW_SIZE_BYTES       1024

#endif /* CMD_PIC_H_ */
