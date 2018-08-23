/*
 * rave-eeprom-update.c
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
#include <string.h>
#include <stdlib.h>
#include <linux/ctype.h>
#include "rave-eeprom.h"
#include "rave-eeprom-common.h"
#include "rave-eeprom-dds.h"

static int parse_seat_location(const char *str, uint8_t *number, char *letter)
{
	char c;
	int len, i, val;

	len = strlen(str);
	if (len < 2 || len > 4) {
		pr_err("invalid seat location: unexpected length\n");
		return -EINVAL;
	}

	if (!isalpha(str[len - 1])) {
		pr_err("invalid seat location: not an alpha char: 0x%02x", str[len - 1]);
		return -EINVAL;
	}
	c = str[len - 1];

	for (i = 0; i < (len - 1); i++) {
		if (!isdigit(str[i])) {
			pr_err("invalid seat location: not a digit: 0x%02x", str[i]);
			return -EINVAL;
		}
	}

	val = (unsigned int)simple_strtoul(str, NULL, 10);
	if (val > 0xff) {
		pr_err("invalid seat location: plane too long (row %u)", val);
		return -EINVAL;
	}

	*number = (uint8_t) val;
	*letter = c;
	return 0;
}

int rave_eeprom_update_dds_page_4(const char *seat_class,
                                  const char *display_type,
                                  const char *location_type,
                                  const char *seat_location)
{
	uint8_t page[RAVE_EEPROM_COMMON_PAGE_SIZE];
	int ret;
	int fd;
	rave_eeprom_dds_seat_class_t seat_class_val = RAVE_EEPROM_DDS_SEAT_CLASS_UNKNOWN;
	rave_eeprom_dds_display_type_t display_type_val = RAVE_EEPROM_DDS_DISPLAY_TYPE_UNKNOWN;
	rave_eeprom_dds_location_type_t location_type_val = RAVE_EEPROM_DDS_LOCATION_TYPE_UNKNOWN;
	int seat_location_number = -1;
	int seat_location_letter = -1;

	if (seat_class) {
		seat_class_val = rave_eeprom_dds_seat_class_from_string(seat_class);
		if (seat_class_val == RAVE_EEPROM_DDS_SEAT_CLASS_UNKNOWN) {
			pr_err("invalid seat class given: %s\n", seat_class);
			return -EINVAL;
		}
	}

	if (display_type) {
		display_type_val = rave_eeprom_dds_display_type_from_string(display_type);
		if (display_type_val == RAVE_EEPROM_DDS_DISPLAY_TYPE_UNKNOWN) {
			pr_err("invalid display type given: %s\n", display_type);
			return -EINVAL;
		}
	}

	if (location_type) {
		location_type_val = rave_eeprom_dds_location_type_from_string(location_type);
		if (location_type_val == RAVE_EEPROM_DDS_LOCATION_TYPE_UNKNOWN) {
			pr_err("invalid location type given: %s\n", location_type);
			return -EINVAL;
		}
	}

	if (seat_location) {
		uint8_t number;
		char	letter;

		if ((ret = parse_seat_location(seat_location, &number, &letter)) != 0)
			return ret;

		seat_location_number = number;
		seat_location_letter = letter;
	}

	fd = rave_eeprom_common_open(RAVE_EEPROM_DDS_PATH, O_RDWR);
	if (fd < 0) {
		pr_err("unable to open dds eeprom: %s\n", strerror(errno));
		return -errno;
	}

	if ((ret = rave_eeprom_common_read_page(fd, RAVE_EEPROM_DDS_PAGE_FLAGS1, page)) != 0)
		goto out;

	if (seat_class_val != RAVE_EEPROM_DDS_SEAT_CLASS_UNKNOWN)
		page[RAVE_EEPROM_DDS_PAGE_FLAGS1_SEAT_CLASS_OFFSET] = (uint8_t)seat_class_val;

	if (display_type_val != RAVE_EEPROM_DDS_DISPLAY_TYPE_UNKNOWN)
		page[RAVE_EEPROM_DDS_PAGE_FLAGS1_DISPLAY_TYPE_OFFSET] = (uint8_t)display_type_val;

	if (location_type_val != RAVE_EEPROM_DDS_LOCATION_TYPE_UNKNOWN)
		page[RAVE_EEPROM_DDS_PAGE_FLAGS1_LOCATION_TYPE_OFFSET] = (uint8_t)location_type_val;

	if (seat_location_number != -1)
		page[RAVE_EEPROM_DDS_PAGE_FLAGS1_SEAT_NUMBER_OFFSET] = (uint8_t)seat_location_number;

	if (seat_location_letter != -1)
		page[RAVE_EEPROM_DDS_PAGE_FLAGS1_SEAT_LETTER_OFFSET] = (uint8_t)seat_location_letter;

	rave_eeprom_common_page_crc_update(page);

	if ((ret = rave_eeprom_common_write_page(fd, RAVE_EEPROM_DDS_PAGE_FLAGS1, page)) != 0)
		goto out;

	ret = 0;

out:
	close(fd);
	return ret;
}

static int parse_on_off (const char *str)
{
	if (strcmp(str,"on") == 0)
		return 1;
	if (strcmp(str,"off") == 0)
		return 0;
	return -1;
}

int rave_eeprom_update_dds_page_6(const char *rju_usb_power,
                                  const char *user_usb_power)
{
	uint8_t page[RAVE_EEPROM_COMMON_PAGE_SIZE];
	int ret;
	int fd;
	int rju_usb_power_val = -1;
	int user_usb_power_val = -1;

	if (rju_usb_power) {
		rju_usb_power_val = parse_on_off (rju_usb_power);
		if (rju_usb_power_val == -1) {
			pr_err("invalid RJU USB power given: %s\n", rju_usb_power);
			return -EINVAL;
		}
	}

	if (user_usb_power) {
		user_usb_power_val = parse_on_off (user_usb_power);
		if (user_usb_power_val == -1) {
			pr_err("invalid user USB power given: %s\n", user_usb_power);
			return -EINVAL;
		}
	}

	fd = rave_eeprom_common_open(RAVE_EEPROM_DDS_PATH, O_RDWR);
	if (fd < 0) {
		pr_err("unable to open dds eeprom: %s\n", strerror(errno));
		return -errno;
	}

	if ((ret = rave_eeprom_common_read_page(fd, RAVE_EEPROM_DDS_PAGE_FLAGS3, page)) != 0)
		goto out;

	if (rju_usb_power_val == 0)
		page[RAVE_EEPROM_DDS_PAGE_FLAGS3_PWR_CFG_BITS_OFFSET] &= ~RAVE_EEPROM_DDS_USB_POWER_RJU;
	else if (rju_usb_power_val == 1)
		page[RAVE_EEPROM_DDS_PAGE_FLAGS3_PWR_CFG_BITS_OFFSET] |= RAVE_EEPROM_DDS_USB_POWER_RJU;

	if (user_usb_power_val == 0)
		page[RAVE_EEPROM_DDS_PAGE_FLAGS3_PWR_CFG_BITS_OFFSET] &= ~RAVE_EEPROM_DDS_USB_POWER_USER;
	else if (user_usb_power_val == 1)
		page[RAVE_EEPROM_DDS_PAGE_FLAGS3_PWR_CFG_BITS_OFFSET] |= RAVE_EEPROM_DDS_USB_POWER_USER;

	rave_eeprom_common_page_crc_update(page);

	if ((ret = rave_eeprom_common_write_page(fd, RAVE_EEPROM_DDS_PAGE_FLAGS3, page)) != 0)
		goto out;

	ret = 0;

out:
	close(fd);
	return ret;
}
