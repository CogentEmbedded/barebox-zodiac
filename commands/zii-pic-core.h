#ifndef _ZII_PIC_CORE_H_
#define _ZII_PIC_CORE_H_

#include <zii/pic.h>

/**********************************
***************clean this *********
**********************************/


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

typedef enum {
	msgErrorDetectionType_Checksum = 0,
	msgErrorDetectionType_Crc
} msgErrorDetectionType_E;

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

/**********************************
***************clean this *********
***************end*****************
**********************************/


typedef uint8_t		u8;
typedef uint16_t	u16;
typedef uint32_t	u32;

#define ZII_PIC_EEPROM_PAGE_SIZE	32
#define ZII_PIC_EEPROM_PAGE_COUNT	512
#define ZII_PIC_EEPROM_DDS_PAGE_COUNT	256

#define ZII_PIC_CMD_MAX_SIZE		64

/* sequential list of all commands on all HW variants */
enum zii_pic_cmd_id {
	ZII_PIC_CMD_GET_STATUS,

	/* Watchdog and reset */
	ZII_PIC_CMD_SW_WDT_SET,
	ZII_PIC_CMD_SW_WDT_GET,
	ZII_PIC_CMD_PET_WDT,
	ZII_PIC_CMD_RESET,
	ZII_PIC_CMD_HW_RECOVERY_RESET,
	ZII_PIC_CMD_GET_RESET_REASON,

	/* HWMON sensors */
	ZII_PIC_CMD_GET_28V_READING,
	ZII_PIC_CMD_GET_12V_READING,
	ZII_PIC_CMD_GET_5V_READING,
	ZII_PIC_CMD_GET_3V3_READING,
	ZII_PIC_CMD_GET_TEMPERATURE,

	/* Main EEPROM */
	ZII_PIC_CMD_EEPROM_READ,
	ZII_PIC_CMD_EEPROM_WRITE,

	/* Board specific variants */
	ZII_PIC_CMD_GET_FIRMWARE_VERSION,
	ZII_PIC_CMD_GET_BOOTLOADER_VERSION,

	ZII_PIC_CMD_DDS_EEPROM_READ,
	ZII_PIC_CMD_DDS_EEPROM_WRITE,

	ZII_PIC_CMD_COPPER_REV,
	ZII_PIC_CMD_LCD_DATA_STABLE,

	/* last one to get amount of supported commands */
	ZII_PIC_CMD_COUNT
};

#define N_MCU_CHECKSUM_NONE    0 /* Checksum is not calculated */
#define N_MCU_CHECKSUM_CRC16   1 /* CCITT CRC16 */
#define N_MCU_CHECKSUM_8B2C    2 /* 8-bit 2's complement */

/* HWMON API */
enum zii_pic_sensor {
	ZII_PIC_SENSOR_FIRST,

	ZII_PIC_SENSOR_28V = ZII_PIC_SENSOR_FIRST,
	ZII_PIC_SENSOR_12V,
	ZII_PIC_SENSOR_5V,
	ZII_PIC_SENSOR_3V3,

	ZII_PIC_SENSOR_TEMPERATURE,
	ZII_PIC_SENSOR_TEMPERATURE_2,

	ZII_PIC_SENSOR_BACKLIGHT_CURRENT,

	ZII_PIC_SENSORS_COUNT
};

enum pic_state {
	PIC_STATE_UNKNOWN,
	PIC_STATE_OPENED,
	PIC_STATE_CONFIGURED
};

struct pic_version {
	u8	hw;
	u16	major;
	u8	minor;
	u8	letter_1;
	u8	letter_2;
} __packed;

struct zii_pic_mfd;
struct zii_pic_eeprom {
	struct zii_pic_mfd		*pic;
	/* eeprom */
	struct cdev			cdev;
	struct file_operations		fops;
	int 				type;

	/* buffer to copy readed data */
	u8				*read_buf;
	/* skip first bytes from input buffer */
	int				read_skip;
	/* useful data size */
	int				read_size;
};

enum pic_eeprom_type {
	ZII_PIC_EEPROM_DDS,
	ZII_PIC_EEPROM_RDU,
	ZII_PIC_EEPROM_COUNT
};

/*
 * @cmd_seqn: PIC command sequence number
 */
struct pic_cmd_desc;
struct zii_pic_mfd {
	struct device			*dev;
	enum pic_hw_id			hw_id;
	u8				checksum_size;
	int				checksum_type;

	int				cmd_seqn;
	struct pic_cmd_desc		*cmd;
	struct console_device 		*cdev;

	struct zii_pic_eeprom		*eeprom[ZII_PIC_EEPROM_COUNT];

	enum pic_state			state;
	long				port_fd;
	struct tty_struct		*tty;

	u8				watchdog_timeout;
	u8				watchdog_enabled;
	u8				reset_reason;

	u8				orientation;
	bool				stowed;

	int				sensor_28v;
	int				sensor_12v;
	int				sensor_5v;
	int				sensor_3v3;
	int				temperature;
	int				temperature_2;
	int				backlight_current;
	int				ec;
	int				rdu_rev;
	int				dds_rev;
	int				temp;

	struct pic_version		bootloader_version;
	struct pic_version		firmware_version;
};

typedef int (*zii_pic_handler_t)(struct zii_pic_mfd *adev,
					u8 *data, u8 size);
struct pic_cmd_desc {
	u8			cmd_id;
	u8			data_len; /* without header (cmd id, ack) and CRC */
	zii_pic_handler_t	response_handler;
};

static inline int zii_pic_f88_to_int(u8 *data)
{
	return data[1] * 1000 + (data[0] * 1000 >> 8);
}

int zii_pic_mcu_cmd(struct zii_pic_mfd *adev,
		enum zii_pic_cmd_id id, const u8 * const data, u8 data_size);


#endif /* _ZII_PIC_CORE_H_ */
