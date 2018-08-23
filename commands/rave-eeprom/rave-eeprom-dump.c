/*
 * rave-eeprom-dump.c
 *
 * Copyright (c) 2018 Zodiac Inflight Innovations.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <malloc.h>
#include <fs.h>
#include <errno.h>
#include <linux/ctype.h>
#include "rave-eeprom.h"
#include "rave-eeprom-common.h"
#include "rave-eeprom-dds.h"
#include "rave-eeprom-main.h"

#define MIN(x,y) (x < y ? x : y)

#define FIELD_NAME_LENGTH 37

static void print_field_name(const char *field_name)
{
	char buffer[FIELD_NAME_LENGTH + 1];

	snprintf(buffer, sizeof(buffer), "  %s:", field_name);
	printf("%-*.*s", FIELD_NAME_LENGTH, FIELD_NAME_LENGTH, buffer);
}

static int dump_page_string(int fd, bool fail_on_crc_errors,
                            unsigned int page_number,
                            const char *field_name, int offset)
{
	uint8_t page[RAVE_EEPROM_COMMON_PAGE_SIZE];
	int i, len;

	if (rave_eeprom_common_read_page(fd, page_number, page) != 0) {
		print_field_name(field_name);
		printf("n/a (error reading page)\n");
		return 0; /* don't halt printing other fields */
	}

	if (!rave_eeprom_common_page_check_initialized(page)) {
		print_field_name(field_name);
		printf("n/a (page not initialized)\n");
		return 0; /* don't halt printing other fields */
	}

	if (!rave_eeprom_common_page_crc_validate(page)) {
		pr_err("invalid crc detected in page %u\n", page_number);
		if (fail_on_crc_errors)
			return -1; /* fatal */
	}

	print_field_name(field_name);
	len = MIN(RAVE_EEPROM_COMMON_PAGE_SIZE - offset - 2, page[offset]);
	for (i = offset + 1; i < len; i++) {
		if (!isprint(page[i])) {
			printf("n/a (invalid string contents)\n");
			return 0; /* don't halt printing other fields */
		}
	}
	printf("%.*s\n", len, (char *)&page[offset + 1]);
	return 0;
}

static int dump_dds_strings(int fd, bool fail_on_crc_errors)
{
	if (dump_page_string(fd, fail_on_crc_errors, RAVE_EEPROM_DDS_PAGE_ASSEMBLY_PARTNUM, "part number", 0) < 0)
		return -1;
	if (dump_page_string(fd, fail_on_crc_errors, RAVE_EEPROM_DDS_PAGE_ASSEMBLY_SERIALNUM, "serial number", 0) < 0)
		return -1;
	if (dump_page_string(fd, fail_on_crc_errors, RAVE_EEPROM_DDS_PAGE_ASSEMBLY_DOM, "date of manufacture", 0) < 0)
		return -1;
	if (dump_page_string(fd, fail_on_crc_errors, RAVE_EEPROM_DDS_PAGE_PARTNUM, "board part number", 0) < 0)
		return -1;
	if (dump_page_string(fd, fail_on_crc_errors, RAVE_EEPROM_DDS_PAGE_BOARD_REV, "board part revision", 0) < 0)
		return -1;
	if (dump_page_string(fd, fail_on_crc_errors, RAVE_EEPROM_DDS_PAGE_SERIAL, "board serial number", 0) < 0)
		return -1;
	if (dump_page_string(fd, fail_on_crc_errors, RAVE_EEPROM_DDS_PAGE_FORMAT, "board date of manufacture", RAVE_EEPROM_DDS_PAGE_FORMAT_MANUFACTURE_OFFSET) < 0)
		return -1;
	if (dump_page_string(fd, fail_on_crc_errors, RAVE_EEPROM_DDS_PAGE_BOARD_REV_UPDATE, "board updated revision", 0) < 0)
		return -1;
	if (dump_page_string(fd, fail_on_crc_errors, RAVE_EEPROM_DDS_PAGE_BOARD_DOM_UPDATE, "board updated date of manufacture", 0) < 0)
		return -1;
	return 0;
}

static int dump_dds_flags1(int fd, bool fail_on_crc_errors)
{
	uint8_t page[RAVE_EEPROM_COMMON_PAGE_SIZE];
	bool page_read;
	bool page_initialized = false;

	page_read = (rave_eeprom_common_read_page(fd, RAVE_EEPROM_DDS_PAGE_FLAGS1, page) == 0);

	if (page_read) {
		page_initialized = rave_eeprom_common_page_check_initialized(page);
		if (page_initialized && !rave_eeprom_common_page_crc_validate(page)) {
			pr_err("invalid crc detected in page %u\n", RAVE_EEPROM_DDS_PAGE_FLAGS1);
			if (fail_on_crc_errors)
				return -1; /* fatal */
		}
	}

	print_field_name("seat class");
	if (!page_read)
		printf("n/a (error reading page)\n");
	else if (!page_initialized)
		printf("n/a (page not initialized)\n");
	else if (page[RAVE_EEPROM_DDS_PAGE_FLAGS1_SEAT_CLASS_OFFSET] < RAVE_EEPROM_DDS_SEAT_CLASS_MAX)
		printf("%s\n", rave_eeprom_dds_seat_class_to_string(page[RAVE_EEPROM_DDS_PAGE_FLAGS1_SEAT_CLASS_OFFSET]));
	else
		printf("unknown (0x%02x)\n", page[RAVE_EEPROM_DDS_PAGE_FLAGS1_SEAT_CLASS_OFFSET]);

	print_field_name("display type");
	if (!page_read)
		printf("n/a (error reading page)\n");
	else if (!page_initialized)
		printf("n/a (page not initialized)\n");
	else if (page[RAVE_EEPROM_DDS_PAGE_FLAGS1_DISPLAY_TYPE_OFFSET] < RAVE_EEPROM_DDS_DISPLAY_TYPE_MAX)
		printf("%s\n", rave_eeprom_dds_display_type_to_string(page[RAVE_EEPROM_DDS_PAGE_FLAGS1_DISPLAY_TYPE_OFFSET]));
	else
		printf("unknown (0x%02x)\n", page[RAVE_EEPROM_DDS_PAGE_FLAGS1_DISPLAY_TYPE_OFFSET]);

	print_field_name("location type");
	if (!page_read)
		printf("n/a (error reading page)\n");
	else if (!page_initialized)
		printf("n/a (page not initialized)\n");
	else if (page[RAVE_EEPROM_DDS_PAGE_FLAGS1_LOCATION_TYPE_OFFSET] < RAVE_EEPROM_DDS_LOCATION_TYPE_MAX)
		printf("%s\n", rave_eeprom_dds_location_type_to_string(page[RAVE_EEPROM_DDS_PAGE_FLAGS1_LOCATION_TYPE_OFFSET]));
	else
		printf("unknown (0x%02x)\n", page[RAVE_EEPROM_DDS_PAGE_FLAGS1_LOCATION_TYPE_OFFSET]);

	print_field_name("seat location");
	if (!page_read)
		printf("n/a (error reading page)\n");
	else if (!page_initialized)
		printf("n/a (page not initialized)\n");
	else if (page[RAVE_EEPROM_DDS_PAGE_FLAGS1_SEAT_NUMBER_OFFSET] != 0 && isalpha(page[RAVE_EEPROM_DDS_PAGE_FLAGS1_SEAT_LETTER_OFFSET]))
		printf("%u%c\n",
		       page[RAVE_EEPROM_DDS_PAGE_FLAGS1_SEAT_NUMBER_OFFSET],
		       page[RAVE_EEPROM_DDS_PAGE_FLAGS1_SEAT_LETTER_OFFSET]);
	else
		printf("unknown (0x%02x 0x%02x)\n",
		       page[RAVE_EEPROM_DDS_PAGE_FLAGS1_SEAT_NUMBER_OFFSET],
		       page[RAVE_EEPROM_DDS_PAGE_FLAGS1_SEAT_LETTER_OFFSET]);

	return 0;
}

static int dump_dds_flags3(int fd, bool fail_on_crc_errors)
{
	uint8_t page[RAVE_EEPROM_COMMON_PAGE_SIZE];
	bool page_read;
	bool page_initialized = false;

	page_read = (rave_eeprom_common_read_page(fd, RAVE_EEPROM_DDS_PAGE_FLAGS3, page) == 0);
	if (page_read) {
		page_initialized = rave_eeprom_common_page_check_initialized(page);
		if (page_initialized && !rave_eeprom_common_page_crc_validate(page)) {
			pr_err("invalid crc detected in page %u\n", RAVE_EEPROM_DDS_PAGE_FLAGS3);
			if (fail_on_crc_errors)
				return -1; /* fatal */
		}
	}

	print_field_name("rju usb power");
	if (!page_read)
		printf("n/a (error reading page)\n");
	else if (!page_initialized)
		printf("n/a (page not initialized)\n");
	else
		printf("%s\n", (page[RAVE_EEPROM_DDS_PAGE_FLAGS3_PWR_CFG_BITS_OFFSET] & RAVE_EEPROM_DDS_USB_POWER_RJU)  ? "on" : "off");

	print_field_name("user usb power");
	if (!page_read)
		printf("n/a (error reading page)\n");
	else if (!page_initialized)
		printf("n/a (page not initialized)\n");
	else
		printf("%s\n", (page[RAVE_EEPROM_DDS_PAGE_FLAGS3_PWR_CFG_BITS_OFFSET] & RAVE_EEPROM_DDS_USB_POWER_USER) ? "on" : "off");

	return 0;
}

static int run_dump_dds(bool fail_on_crc_errors)
{
	int fd;
	int ret = -EFAULT;

	fd = rave_eeprom_common_open(RAVE_EEPROM_DDS_PATH, O_RDONLY);
	if (fd < 0) {
		pr_err("unable to open dds eeprom: %s\n", strerror(errno));
		return -errno;
	}

	printf("pic dds eeprom info:\n");
	if (dump_dds_strings(fd, fail_on_crc_errors) < 0)
		pr_err("error printing dds strings\n");
	else if (dump_dds_flags1(fd, fail_on_crc_errors) < 0)
		pr_err("error printing dds flags1\n");
	else if (dump_dds_flags3(fd, fail_on_crc_errors) < 0)
		pr_err("error printing dds flags3\n");
	else
		ret = 0;

	close(fd);
	return ret;
}

static int dump_main_strings(int fd, bool fail_on_crc_errors)
{
	if (dump_page_string(fd, fail_on_crc_errors, RAVE_EEPROM_MAIN_PAGE_PARTNUM, "part number", 0) < 0)
		return -1;
	if (dump_page_string(fd, fail_on_crc_errors, RAVE_EEPROM_MAIN_PAGE_SERIAL, "serial number", 0) < 0)
		return -1;
	if (dump_page_string(fd, fail_on_crc_errors, RAVE_EEPROM_MAIN_PAGE_FORMAT, "date of manufacture", RAVE_EEPROM_MAIN_PAGE_FORMAT_MANUFACTURE_OFFSET) < 0)
		return -1;
	if (dump_page_string(fd, fail_on_crc_errors, RAVE_EEPROM_MAIN_PAGE_BOARD_PARTNUM, "board part number", 0) < 0)
		return -1;
	if (dump_page_string(fd, fail_on_crc_errors, RAVE_EEPROM_MAIN_PAGE_BOARD_REV, "board part revision", 0) < 0)
		return -1;
	if (dump_page_string(fd, fail_on_crc_errors, RAVE_EEPROM_MAIN_PAGE_BOARD_SERIAL, "board serial number", 0) < 0)
		return -1;
	if (dump_page_string(fd, fail_on_crc_errors, RAVE_EEPROM_MAIN_PAGE_BOARD_DOM, "board date of manufacture", 0) < 0)
		return -1;
	if (dump_page_string(fd, fail_on_crc_errors, RAVE_EEPROM_MAIN_PAGE_BOARD_REV_UPDATE, "board updated revision", 0) < 0)
		return -1;
	if (dump_page_string(fd, fail_on_crc_errors, RAVE_EEPROM_MAIN_PAGE_BOARD_DOM_UPDATE, "board updated date of manufacture", 0) < 0)
		return -1;
	return 0;
}

static int run_dump_main(bool fail_on_crc_errors)
{
	int fd;
	int ret = -EFAULT;

	fd = rave_eeprom_common_open(RAVE_EEPROM_MAIN_PATH, O_RDONLY);
	if (fd < 0) {
		pr_err("unable to open main eeprom: %s\n", strerror(errno));
		return -errno;
	}

	printf("pic main eeprom info:\n");
	if (dump_main_strings(fd, fail_on_crc_errors) < 0)
		pr_err("error printing strings\n");
	else
		ret = 0;

	close(fd);
	return ret;
}

int rave_eeprom_dump(int dump_main, int dump_dds, bool fail_on_crc_errors)
{
	int ret;

	if (!dump_dds && !dump_main) {
		dump_dds = 1;
		dump_main = 1;
	}

	if (dump_main && ((ret = run_dump_main(fail_on_crc_errors)) != 0)) {
		pr_err("failed dumping main eeprom contents\n");
		return ret;
	}

	if (dump_dds && ((ret = run_dump_dds(fail_on_crc_errors)) != 0)) {
		pr_err("failed dumping dds eeprom contents\n");
		return ret;
	}

	return 0;
}
