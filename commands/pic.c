//*****************************************************************************
//
// Disclosure:
// Copyright(C) 2010 Systems and Software Enterprises, Inc. (dba IMS)
// ALL RIGHTS RESERVED.
// The contents of this medium may not be reproduced in whole or part without
// the written consent of IMS, except as authorized by section 117 of the U.S.
// Copyright law.
//*****************************************************************************


/*
 * Misc functions
 */
#include <common.h>
#include <command.h>
#include <errno.h>
#include <net.h>

// IMS_PATCH: Runtime support of 2 different environment devices (MMC/SF) ------------
#include <environment.h>

#include "pic.h"

#define EEPROM_BACKUP_PAGE  20

// Global flag to determine if the failed boot has already been incremented
int bootFailureIncremented = 0;

static struct console_device *pic_cdev = NULL;

void pic_uart_init(struct console_device *cdev, int speed)
{
	pic_cdev = cdev;

	pic_cdev->setbrg(pic_cdev, speed);
	if (pic_cdev->flush)
		pic_cdev->flush(pic_cdev);
}

void pic_putc(char c)
{
	pic_cdev->putc(pic_cdev, c);
}

int pic_getc(char chan, char *c)
{
	int timeout = 1000; /* 1 mS */
	while (timeout > 0) {
		if (pic_cdev->tstc(pic_cdev)) {
			*c = pic_cdev->getc(pic_cdev);
			return 1;
		}
		udelay(100);
		timeout -= 100;
	}
	return 0;
}

void pic_reset_comms(void)
{
	/* flush input */
	while (pic_cdev->tstc(pic_cdev))
		pic_cdev->getc(pic_cdev);
	pic_putc('\x03');
	pic_putc('\x03');
	pic_putc('\x03');
}

static int pic_escape_data(unsigned char *out, const unsigned char* in, int len)
{
	unsigned char c;
	unsigned char *out_temp = out;

	while (len--)
	{
		c = *in++;
		if (c == ETX || c == DLE || c == STX)
			*out_temp++ = DLE;
		*out_temp++ = c;
	}
	return out_temp - out;
}

// Calculates the simple checksum, and returns
static unsigned char pic_calc_checksum(const unsigned char * in, int len)
{
	unsigned char sum = 0;
	while(len--)
		sum += *in++;
	return 0x100 - sum;
}


int pic_ack_id = 0;
// Packs message into pic packet
// Returns total message length
static int pic_pack_msg(unsigned char *out, const unsigned char *in, char msg_type, int len)
{
	unsigned char checksum;
	unsigned char temp[256];
	unsigned char *out_temp = out;
	int i = 0;

	// Pack ack, data, and checksum into temp structure,
	// because we have to escape the whole thing
	temp[i++] = msg_type;
	temp[i++] = ++pic_ack_id;
	if ((in) && (len)) {
		memcpy(&temp[i], in, len);
		i += len;
	}

	checksum = pic_calc_checksum(temp, i);

	temp[i++] = checksum;

	*out_temp++ = STX;
	out_temp += pic_escape_data(out_temp, temp, i);
	*out_temp++ = ETX;

	return out_temp - out;
}

// Sends message to pic
void pic_send_msg(const unsigned char *msg, char msg_type,int len)
{
	unsigned char out[256];
	unsigned char *ptr;
	int packet_len;

	packet_len = pic_pack_msg(out, msg, msg_type, len);

	ptr = out;
	while(packet_len--)
		pic_putc(*ptr++);
}

// Returns length of data received
int pic_recv_msg(unsigned char *out)
{
	// Do to some requests to the PIC taking upto 75 ms
	// the max timeout has been set to 225 ms to be safe.
	#define TIMEOUT_15 225
	char comm = 0;
	unsigned char * out_temp = out;
	char c;
	int time = TIMEOUT_15;
	int timeout = 0;
	int escaped = 0;
	int receiving = 0;
	unsigned char cksum = 0;
	int cnt = 0;

	while(1) {
		if (--time == 0) {
			timeout = 1;
			break;
		}

		if(!pic_getc(comm, &c)) {
			continue;
		}

		time = TIMEOUT_15;
		cnt++;

		if (!receiving) {
			if (c == STX) {
				receiving = 1;
			}
		}
		else {
			*out_temp++ = c;
			if (!escaped) {
				if (c == ETX) {
					out_temp--; // Don't want ETX in buffer
					break;
				}
				else if (c == DLE) {
					out_temp--; // backup! we don't want this char
					escaped = 1;
					continue;
				}
			} else {
				escaped = 0;
			}
			cksum += c;
		}
	}

	if (cksum != 0)	{
		printf("Checksum is %02X instead of zero -- packet dropped\n", cksum);
		return -1;
	}

	if (timeout) {
		printf("pic_recv_message timeout with %d chars received\n", out_temp - out);
		return -1;
	}

	return out_temp - out;
}



/********************************************************************
 * Function:        int GetEepromData(uint16_t pageNum, uint8_t *data)
 *
 * Input:           uint16_t pageNum: page number
 *                  uint8_t *data: buffer to place data
 *
 * Output:          0 on success, 1 on failure
 *
 * Overview:        Reads a page of data from the specified EEPROM
 *
 *******************************************************************/
int pic_GetRduEepromData(uint16_t pageNum, uint8_t *data)
{
	unsigned char dataOut[64]   = {0};
	unsigned char dataIn[64]    = {0};
	int recvLen = 0;
	int x = 0;
	int index = 0;

	// now send over the data
	dataOut[0] = 0x01; //Read data from the EEPROM
	dataOut[1] = (uint8_t)(pageNum & 0x00FF);
	dataOut[2] = (uint8_t)((pageNum >> 8) & 0x00FF);

	pic_reset_comms();

	// Send out the message
	pic_send_msg(dataOut, CMD_PIC_EEPROM, 3);

	// Receive the Message
	recvLen = pic_recv_msg(dataIn);

	if(recvLen <= 0 || dataIn[3] == 0)
		return 1;

	//data starts at byte 4 and runs through byte 35
	//now place the data in the buffer
	index = 4;
	for(x = 0; x < PIC_PAGE_SIZE; x++)
		data[x] = dataIn[index++];

	return 0;
}

/********************************************************************
 * Function:        int SendEepromData(uint16_t pageNum, uint8_t *data)
 *
 * Input:           uint16_t pageNum: page number
 *                  uint8_t *data: buffer to place data
 *
 * Output:          0 on success, 1 on failure
 *
 * Overview:        Writes a page of data to the specified EEPROM
 *
 *******************************************************************/
int pic_SendRduEepromData(uint16_t pageNum, uint8_t *data)
{
	unsigned char dataOut[64]   = {0};
	unsigned char dataIn[64]    = {0};
	int recvLen = 0;
	int x = 0;
	int index = 0;

	//now send over the data
	dataOut[0] = 0x00; //Write to the EEPROM
	dataOut[1] = (uint8_t)(pageNum & 0x00FF);
	dataOut[2] = (uint8_t)((pageNum >> 8) & 0x00FF);

	//data starts at byte 4 and runs through byte 35
	//now place the data in the buffer

	index = 3;

	for(x = 0; x < PIC_PAGE_SIZE; x++)
		dataOut[index++] = data[x];

	pic_reset_comms();

	//now send the data to the PIC to store to the RDU EEPROM
	pic_send_msg(dataOut, CMD_PIC_EEPROM, 35);

	// Receive the ACK
	recvLen = pic_recv_msg(dataIn);

	if(recvLen > 0 || dataIn[3] == 1) {
		return 0;
	} else {
		return 1;
	}
}

/* IMS: Add U-Boot commands to Get the IP Address and Netmask and Turn on the LCD Backlight using the Microchip PIC */
void do_pic_get_ip(void)
{
	unsigned char data[64];
	int len;
	uint32_t ip_extracted = 0;
	uint32_t netmask_extracted = 0;
	char *ip_addr;
	char *netmask;
	char *env_val;
/*
	// Check the platform to see if a PIC is attached
	if (!(system_type == SYSTEM_TYPE__RDU_B ||
		system_type == SYSTEM_TYPE__RDU_C) ) {

		// All of these platforms do not have pics
		// So set "ipsetup" to a empty string
		setenv ("ipsetup","");

		printf("*** This platform doesn't have a PIC, the IP address can't be retrieved in this manner ****\n");
		return;
	}
*/
	pic_reset_comms();

	data[0] = 0; // 0 = RDU, 1 = RJU
	data[1] = 0; // 0 = Self, 1 = Pri Eth, 2 = Sec Eth, 3 = Aux Eth, 4 = SPB/SPU

	pic_send_msg(data, CMD_REQ_IP_ADDR, 2);
	len = pic_recv_msg(data);

	if (len > 0) {
		ip_extracted      = data[5] << 24 | data[4] << 16 | data[3] << 8 | data[2];
		netmask_extracted = data[9] << 24 | data[8] << 16 | data[7] << 8 | data[6];
	}


	if ((data[5]==0) && (data[4]==0) && (data[3]==0) && (data[2]==0) ){
		setenv ("ipsetup","dhcp");
	}
	else {
		/* printf("%x\n",ip_extracted);
		printf("%x\n",netmask_extracted); */
		ip_addr = strdup(ip_to_string(ip_extracted));
		setenv ("ipaddr", ip_addr);
		netmask = strdup(ip_to_string(netmask_extracted));
		setenv ("netmask", netmask);
		env_val = asprintf("%s:::%s::eth0:", ip_addr, netmask);
		setenv ("ipsetup", env_val);
		free(env_val);
		free(netmask);
		free(ip_addr);
		/* saveenv(); */
	}

	return;
}

/* IMS: Add U-Boot commands to Turn on/off the LCD Backlight using the Microchip PIC */
int do_pic_set_lcd(int argc, char *argv[])
{
	unsigned char data[64];
	int len;
	long enable     = 1;	/* 0 = disable, 1 = enable */
	long brightness = 80; 	/* hard-coded to 80% */
	long delay      = 10000;	/* hard-coded to 10 sec */
/*
	// Check the platform to see if a PIC is attached
	if (!(system_type == SYSTEM_TYPE__RDU_B ||
		system_type == SYSTEM_TYPE__RDU_C) ) {

		// All of these platforms do not have pics
		// So just return
		printf("*** This platform doesn't have a PIC, thus the LCD Backlight command is not available ***\n");
		return 0;
	}
*/
	if (!pic_cdev)
		return -ENODEV;
	if (argc != 4)
		return COMMAND_ERROR_USAGE;

	enable      = simple_strtol(argv[1], NULL, 10);
	brightness  = simple_strtol(argv[2], NULL, 10);
	delay       = simple_strtol(argv[3], NULL, 10) * 1000;

	pic_reset_comms();

	brightness = brightness > 100 ? 100 : brightness;

	// first two bytes are taken care of by pic_send_mesg
	data[0] = ((enable ? 1 : 0) << 7) | brightness;
	data[1] = delay & 0xFF;
	data[2] = delay >> 8;

	pic_send_msg(data, CMD_LCD_BACKLIGHT, 3);
	len = pic_recv_msg(data);

	return 0;
}

int do_pic_en_lcd(int argc, char *argv[])
{
	unsigned char data[64];
	int len;

	if (!pic_cdev)
		return -ENODEV;
	if (argc != 1)
		return COMMAND_ERROR_USAGE;

	pic_reset_comms();

	pic_send_msg(NULL, CMD_LCD_BOOT_ENABLE, 0);
	len = pic_recv_msg(data);

	return 0;
}

int do_pic_en_usb(int argc, char *argv[])
{
	unsigned char data[64];
	int len;

	if (!pic_cdev)
		return -ENODEV;
	if (argc != 1)
		return COMMAND_ERROR_USAGE;

	pic_reset_comms();

	pic_send_msg(NULL, CMD_USB_BOOT_ENABLE, 0);
	len = pic_recv_msg(data);

	return 0;
}

int do_pic_status(int argc, char *argv[])
{
	unsigned char data[64];
	int len;

	if (!pic_cdev)
		return -ENODEV;
	if (argc != 1)
		return COMMAND_ERROR_USAGE;

	pic_reset_comms();

	pic_send_msg(NULL, CMD_STATUS, 0);
	len = pic_recv_msg(data);

	if (len < 0)
		return len;

	printf("RSP message %02x (%d)\n", data[0], len);

	printf("BL: %d, %s, %d mA\n",
		data[28] & 0x7f,
		data[28] & 0x80 ? "en" : "dis",
		(data[30] << 8) | data[29]);

	return 0;
}

int do_pic_reset(int argc, char *argv[])
{
	unsigned char data[64];
	int len;

	if (!pic_cdev)
		return -ENODEV;
	if (argc != 1)
		return COMMAND_ERROR_USAGE;

	data[0] = 1;
	pic_send_msg(data, CMD_PIC_RESET, 1);

	len = pic_recv_msg(data);

	return 0;
}

/* IMS: Add U-Boot commands to Get the Reset reason from the Microchip PIC */
int do_pic_get_reset (int argc, char *argv[])
{
	unsigned char data[64];
	int len;
	uint32_t reason_extracted = 0;
/*
	// Check the platform to see if a PIC is attached
	if (!(system_type == SYSTEM_TYPE__RDU_B ||
		system_type == SYSTEM_TYPE__RDU_C) ) {

		// All of these platforms do not have pics
		// So just return
		printf("*** This platform doesn't have a PIC, thus the PIC RESET command is not available ***\n");
		return 0;
	}
*/

	if (!pic_cdev)
		return -ENODEV;
	if (argc != 1)
		return COMMAND_ERROR_USAGE;

	pic_reset_comms();

	pic_send_msg(data, CMD_RESET_REASON, 2);
	len = pic_recv_msg(data);

	if (len > 0)
		reason_extracted = data[2];


	switch (reason_extracted) {
		case 0x0:
			setenv ("reason","Normal_Power_off");
			printf ("Reset Reason: Normal Power off\n");
			break;

		case 0x1:
			setenv ("reason","PIC_HW_Watchdog");
			printf ("Reset Reason: PIC HW Watchdog\n");
			break;

		case 0x2:
			setenv ("reason","PIC_SW_Watchdog");
			printf ("Reset Reason: PIC SW Watchdog\n");
			break;

		case 0x3:
			setenv ("reason","Input_Voltage_out_of_range");
			printf ("Reset Reason: Input Voltage out of range\n");
			break;

		case 0x4:
			setenv ("reason","Host_Requested");
			printf ("Reset Reason: Host Requested\n");
			break;

		case 0x5:
			setenv ("reason","Temperature_out_of_range");
			printf ("Reset Reason: Temperature out of range\n");
			break;

		case 0x6:
			setenv ("reason","User_Requested");
			printf ("Reset Reason: User Requested\n");
			break;

		case 0x7:
			setenv ("reason","Illegal_Configuration_Word");
			printf ("Reset Reason: Illegal Configuration Word\n");
			break;

		case 0x8:
			setenv ("reason","Illegal_Instruction");
			printf ("Reset Reason: Illegal Instruction\n");
			break;

		case 0x9:
			setenv ("reason","Illegal_Trap");
			printf ("Reset Reason: Illegal Trap\n");
			break;

		case 0xa:
			setenv ("reason","Unknown_Reset_Reason");
			printf ("Reset Reason: Unknown\n");
			break;

		default: /* Can't happen? */
			printf ("Reset Reason: Unknown 0x%x\n", reason_extracted);
			break;
	}

	return 0;
}

BAREBOX_CMD_HELP_START(pic_setlcd)
	BAREBOX_CMD_HELP_TEXT("Turn on the LCD Backlight via the Microchip PIC")
	BAREBOX_CMD_HELP_TEXT("")
	BAREBOX_CMD_HELP_TEXT("Options:")
	BAREBOX_CMD_HELP_OPT ("[en]", "enable or disable")
	BAREBOX_CMD_HELP_OPT ("[level]", "level")
	BAREBOX_CMD_HELP_OPT ("[time]", "time")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(pic_setlcd)
	.cmd		= do_pic_set_lcd,
	BAREBOX_CMD_DESC("Turn on the LCD Backlight via the Microchip PIC")
	BAREBOX_CMD_OPTS("[en] [level] [time]")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
	BAREBOX_CMD_HELP(cmd_pic_setlcd_help)
BAREBOX_CMD_END

BAREBOX_CMD_START(pic_enlcd)
	.cmd		= do_pic_en_lcd,
	BAREBOX_CMD_DESC("Enable LCD via the Microchip PIC")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
BAREBOX_CMD_END

BAREBOX_CMD_START(pic_enusb)
	.cmd		= do_pic_en_usb,
	BAREBOX_CMD_DESC("Enable USB via the Microchip PIC")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
BAREBOX_CMD_END

BAREBOX_CMD_START(pic_status)
	.cmd		= do_pic_status,
	BAREBOX_CMD_DESC("Get PIC status")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
BAREBOX_CMD_END

BAREBOX_CMD_START(pic_getreset)
	.cmd		= do_pic_get_reset,
	BAREBOX_CMD_DESC("Read the Reset Reason from the Microchip PIC")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
BAREBOX_CMD_END

BAREBOX_CMD_START(pic_reset)
	.cmd		= do_pic_reset,
	BAREBOX_CMD_DESC("Reset Microchip PIC and whole system")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
BAREBOX_CMD_END

/* IMS: Add U-Boot commands to Pet the Watchdog Timer using the Microchip PIC */
int do_pic_pet_wdt (int argc, char *argv[])
{
	unsigned char data[64];
	int len;
/*
	// Check the platform to see if a PIC is attached
	if (!(system_type == SYSTEM_TYPE__RDU_B ||
		system_type == SYSTEM_TYPE__RDU_C) ) {

		// All of these platforms do not have pics
		// So just return
		printf("*** This platform doesn't have a PIC, thus the PET Watchdog command is not available ***\n");
		return 0;
	}
*/
	if (!pic_cdev)
		return -ENODEV;
	if (argc != 1)
		return COMMAND_ERROR_USAGE;

	/* printf("do_pic_get_ip function called.\n"); */

	pic_reset_comms();

	data[0] = 0; // 0 = RDU, 1 = RJU
	data[1] = 0; // 0 = Self, 1 = Pri Eth, 2 = Sec Eth, 3 = Aux Eth, 4 = SPB/SPU

	pic_send_msg(data, CMD_PET_WDT, 2);
	len = pic_recv_msg(data);

	if (len <= 0)
		printf("ERROR: Unable to pet the Watchdog Timer\n!");

	return 0;
}



/* IMS: Add U-Boot command to Enable and Set the Watchdog Timer using the Microchip PIC */
int do_pic_set_wdt (int argc, char *argv[])
{
	unsigned char data[64];
	int len;
	long enable = 1;	/* 0 = disable, 1 = enable, 2 = enable in debug mode */
	long timeout = 60;	/* hard-coded to 60 sec */
/*
	// Check the platform to see if a PIC is attached
	if (!(system_type == SYSTEM_TYPE__RDU_B ||
		system_type == SYSTEM_TYPE__RDU_C) ) {

		// All of these platforms do not have pics
		// So just return
		printf("*** This platform doesn't have a PIC, thus the SET Watchdog command is not available ***\n");
		return 0;
	}
*/
	if (!pic_cdev)
		return -ENODEV;
	if ((argc != 2) && (argc != 3))
		return COMMAND_ERROR_USAGE;

	enable = simple_strtol(argv[1], NULL, 10);

	if (enable) {
		if (argc != 3)
			return COMMAND_ERROR_USAGE;

		timeout = simple_strtol(argv[2], NULL, 10);
		if (timeout < 60) {
			printf("Minimum WDT Timeout is 60 seconds\n");
			return 1;
		}
		if (timeout > 300) {
			printf("Maximum WDT Timeout is 300 seconds\n");
			return 1;
		}
	}

	/* printf("do_pic_set_wdt function called.\n");
	printf("Enable set to %d\n",enable);
	printf("Timeout set to %d seconds\n",timeout); */

	pic_reset_comms();

	// first two bytes are taken care of by pic_send_mesg
	data[0] = enable;
	data[1] = timeout & 0xFF;
	data[2] = timeout >> 8;

	pic_send_msg(data, CMD_SW_WDT, 3);
	len = pic_recv_msg(data);

	if (len <= 0) {
		printf("Error setting Watchdog Timer\n");
	}

	return 0;
}

BAREBOX_CMD_HELP_START(pic_setwdt)
	BAREBOX_CMD_HELP_TEXT("Set the Watchdog Timer (WDT) on the Microchip PIC")
	BAREBOX_CMD_HELP_TEXT("")
	BAREBOX_CMD_HELP_TEXT("1 60  Enable Watchdog with Timeout value of 60 seconds; lowest value")
	BAREBOX_CMD_HELP_TEXT("1 300 Enable Watchdog with Timeout value of 300 seconds; highest value")
	BAREBOX_CMD_HELP_TEXT("0 60  Disable Watchdog and ignore Timeout parameter")
	BAREBOX_CMD_HELP_TEXT("Options:")
	BAREBOX_CMD_HELP_OPT ("[mode]", "0 = disable, 1 = enable, 2 = enable in debug mode")
	BAREBOX_CMD_HELP_OPT ("[timeout]", "Timeout Value in second, range 60-300")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(pic_setwdt)
	.cmd		= do_pic_set_wdt,
	BAREBOX_CMD_DESC("Set the Watchdog Timer (WDT) on the Microchip PIC")
	BAREBOX_CMD_OPTS("[mode] [timeout]")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
	BAREBOX_CMD_HELP(cmd_pic_setwdt_help)
BAREBOX_CMD_END

BAREBOX_CMD_START(pic_petwdt)
	.cmd		= do_pic_pet_wdt,
	BAREBOX_CMD_DESC("Pet the Watchdog Timer (WDT) on the Microchip PIC")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
BAREBOX_CMD_END

/********************************************************************
 * Function:        void GetRDUBootDevice()
 *
 * Input:           U-Boot Cmd Sequence
 *
 * Output:          None
 *
 * Overview:        Prints out the current boot device
 *
 *******************************************************************/
int do_pic_get_boot_device (int argc, char *argv[])
{
	unsigned char data[64];
	int len;
/*
	// Check the platform to see if a PIC is attached
	if (!(system_type == SYSTEM_TYPE__RDU_B ||
		system_type == SYSTEM_TYPE__RDU_C) ) {

		// All of these platforms do not have pics
		// So just return
		printf("*** This platform doesn't have a PIC, thus the Get Boot Device command is not available ***\n");
		return 0;
	}
*/
	if (!pic_cdev)
		return -ENODEV;
	if (argc != 1)
		return COMMAND_ERROR_USAGE;

	/* printf("do_pic_get_boot_device function called.\n"); */

	pic_reset_comms();

	data[0] = 0x01; //read flag
	data[1] = 0x04; //EEPROM Page Number LSB
	data[2] = 0x00; //EEPROM Page Number MSB

	//send out the message to read page 4
	pic_send_msg(data, CMD_PIC_EEPROM, 3);

	//read page 4 so we can change byte 0x03
	len = pic_recv_msg(data);

	if (len <= 0)
		printf("Error getting RDU Boot Device\n");

	if(data[3] != 1)
		return -1;

	if(data[7] == 0)
		printf("Current Boot Device: SD\n");
	else if(data[7] == 1)
		printf("Current Boot Device: eMMC\n");
	else if(data[7] == 2)
		printf("Current Boot Device: NOR\n");
	else
		printf("Current Boot Device: INVALID\n");

	return 0;
}

BAREBOX_CMD_START(pic_getboot)
	.cmd		= do_pic_get_boot_device,
	BAREBOX_CMD_DESC("Get the Boot device from the Microchip PIC")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
BAREBOX_CMD_END


/********************************************************************
 * Function:        void SetRDUBootDevice(uint8_t theDevice)
 *
 * Input:           long int which can be from rduBootDevice enum
 *
 * Output:          None
 *
 * Overview:        sends a command to the RDU to set its boot device
 *                  SD (external flash card) or EMMC (on-board flash)
 *
 *******************************************************************/
int do_pic_set_boot_device (int argc, char *argv[]) {
	unsigned char data[64];
	int len;
	uint8_t tempData[32] = {0};
	int x = 0;
	int index = 0;
	int theDevice = 0;
/*
	// Check the platform to see if a PIC is attached
	if (!(system_type == SYSTEM_TYPE__RDU_B ||
		system_type == SYSTEM_TYPE__RDU_C) ) {

		// All of these platforms do not have pics
		// So just return
		printf("*** This platform doesn't have a PIC, thus the Set Boot Device command is not available ***\n");
		return 0;
	}
*/
	if (!pic_cdev)
		return -ENODEV;
	if (argc != 2)
		return COMMAND_ERROR_USAGE;

	theDevice = simple_strtol(argv[1], NULL, 10);
	if ((theDevice < 0) ||
		(theDevice > 2))
		return -EINVAL;

	/* printf("do_pic_set_boot_device function called.\n"); */

	pic_reset_comms();

	data[0] = 0x01; //read flag
	data[1] = 0x04; //EEPROM Page Number LSB
	data[2] = 0x00; //EEPROM Page Number MSB

	//send out the message to read page 4
	/* sendBytes((unsigned char*)&PacketToDevice, 5, NORMAL_MSG); */
	pic_send_msg(data, CMD_PIC_EEPROM, 3);

	//read page 4 so we can change byte 0x03
	/* receiveBytes((unsigned char*)&PacketFromDevice, 1, NORMAL_MSG); */
	len = pic_recv_msg(data);

	if(data[3] != 1)
		return -1;

	index = 4;
	for(x = 0; x < 32; x++)
		tempData[x] = data[index++];

	//change byte 0x03 to the specified boot device
	tempData[3] = theDevice;

	data[0] = 0x00; //write flag
	data[1] = 0x04; //EEPROM Page Number LSB
	data[2] = 0x00; //EEPROM Page Number MSB

	index = 3;
	for(x = 0; x < 32; x++)
		data[index++] = tempData[x];

	//send out the message to read page 4
	/* sendBytes((unsigned char*)&PacketToDevice, 37, NORMAL_MSG); */
	pic_send_msg(data, CMD_PIC_EEPROM, 35);

	//read page 4 so we can change byte 0x03
	len = pic_recv_msg(data);
	if (len <= 0) {
		printf("Error setting RDU Boot Device\n");
	}

	return 0;
}

BAREBOX_CMD_HELP_START(pic_setboot)
	BAREBOX_CMD_HELP_TEXT("Set the Boot device from the Microchip PIC")
	BAREBOX_CMD_HELP_TEXT("")
	BAREBOX_CMD_HELP_TEXT("Options:")
	BAREBOX_CMD_HELP_OPT ("[dev]", "0 = SD, 1 = eMMC, 2 = NOR")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(pic_setboot)
	.cmd		= do_pic_set_boot_device,
	BAREBOX_CMD_DESC("Set the Boot device from the Microchip PIC")
	BAREBOX_CMD_OPTS("[dev]")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
	BAREBOX_CMD_HELP(cmd_pic_setboot_help)
BAREBOX_CMD_END

/********************************************************************
 * Function:        int set_pic_boot_progress(uint16_t progress)
 *
 * Input:           uint16_t progress: progress number
 *
 * Output:          0 on success, 1 on failure
 *
 * Overview:        Sends a message to the PIC telling it the
 * 					current boot progress state
 *
 *******************************************************************/
int pic_set_boot_progress(uint16_t progress) {
	uint8_t data[32] = {0};
	int len = 0;

	// first byte is LSB
	// second byte is MSB
	data[0] = progress & 0x00FF;
	data[1] = (progress >> 8) & 0x00FF;

	//send out the message
	pic_send_msg(data, CMD_UPDATE_BOOT_PROGRESS_CODE, 2);

	// Reveive the response
	len = pic_recv_msg(data);
	if(len <= 0) {
		// error setting boot progress
		return 1;
	}

	return 0;
}



/********************************************************************
 * Function:        int pic_incrementNumFailedBoots(void)
 *
 * Input:           None
 *
 * Output:          0 on success, 1 on failure
 *
 * Overview:        Increments the number failed boots
 *
 *******************************************************************/
int pic_incrementNumFailedBoots(void)
{
    unsigned char data[64]  = {0};
    uint8_t numBoots        = 0;
/*
	// Check the platform to see if a PIC is attached
	if(!(system_type == SYSTEM_TYPE__RDU_B ||
		system_type == SYSTEM_TYPE__RDU_C) ) {

		// All of these platforms do not have pics
		// So just return
		printf("*** This platform doesn't have a PIC, thus the Get Boot Device command is not available ***\n");
		return 0;
	}
*/

    if(bootFailureIncremented == 1) {
        return 0;
    }

    bootFailureIncremented = 1;

    // Read out Page 5 of the RDU EEPROM
    // It contains the UINT16 for number of boot failures
    if(pic_GetRduEepromData(5, data) != 0) {
        // Error, was unable to read data from the RDU EEPROM
        return 1;
    }

    //UINT8 from the EEPROM
    numBoots = data[4];

    // Check to see if the board has been initalized
    // If  not set the failed count to 0;
    if(numBoots == 0xFF) {
        numBoots = 0;
    }

    // Increment the number of failed boots
    numBoots++;

    // If the number of failed boots is about to wrap
    // around don't save the new value as
    // we want to see that a insane amount of
    // bad boots has already happened
    if(numBoots >= 0xFE) {
        // Just return success
        return 0;
    }

    // Split the UINT16 back into bytes to be written to the EEPROM
    data[4] = numBoots;

    return pic_SendRduEepromData(5, data);
}



/********************************************************************
 * Function:        int pic_getNumFailedBoots(uint8_t *boots)
 *
 * Input:           uint8_t*: variable to return number of failed boots
 *
 * Output:          0 on success, 1 on failure
 *
 * Overview:        Returns the number of failed boots
 *
 *******************************************************************/
int pic_getNumFailedBoots(uint8_t *boots) {
	unsigned char data[64]  = {0};
/*
	// Check the platform to see if a PIC is attached
	if( !(system_type == SYSTEM_TYPE__RDU_B ||
		 system_type == SYSTEM_TYPE__RDU_C) ) {

		// All of these platforms do not have pics
		// So just return
		printf("*** This platform doesn't have a PIC, thus the Get Boot Device command is not available ***\n");
		return 1;
	}
*/
	// Read out Page 5 of the RDU EEPROM
	// It contains the UINT16 for number of boot failures
	if(pic_GetRduEepromData(5, data) != 0) {
		// Error, was unable to read data from the RDU EEPROM
		printf("[ERROR] Unable to read from the RDU EEPROM\n");
		return 1;
	}

	//UINT8 from the EEPROM
	if(data[4] ==  0xFF) {
		*boots = 0;
	}
	else {
		*boots = data[4];
	}

	return 0;
}



/********************************************************************
 * Function:        int pic_getNumFailedBoots(int argc, char *argv[])
 *
 * Input:           Command line arguments
 *
 * Output:          0 on success, 1 on failure
 *
 * Overview:        Prints out the number failed boots
 *
 *******************************************************************/
int do_pic_getNumFailedBoots(int argc, char *argv[])
{
	int retVal        = 0;
	uint8_t numBoots  = 0;

	retVal = pic_getNumFailedBoots(&numBoots);
	if(retVal == 0) {
		printf("Number of failed boot attempts due to eMMC errors: %d\n", numBoots);
	}

	return 0;
}

BAREBOX_CMD_START(pic_getNumFailedBoots)
	.cmd		= do_pic_getNumFailedBoots,
	BAREBOX_CMD_DESC("Get the number of failed boot attempts due to eMMC errors")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
BAREBOX_CMD_END
