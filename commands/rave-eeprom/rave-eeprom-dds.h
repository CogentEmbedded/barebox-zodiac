/*
 * rave-eeprom-dds.h
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __RAVE_EEPROM_DDS_H
#define __RAVE_EEPROM_DDS_H

#include <stdlib.h>

#define RAVE_EEPROM_DDS_PAGE_SIZE 0x20

/* Page numbers */
typedef enum {
	RAVE_EEPROM_DDS_PAGE_FORMAT             = 0,
	RAVE_EEPROM_DDS_PAGE_PARTNUM            = 1,
	RAVE_EEPROM_DDS_PAGE_SERIAL             = 2,
	RAVE_EEPROM_DDS_PAGE_RDU_SERIAL         = 3,
	RAVE_EEPROM_DDS_PAGE_FLAGS1             = 4,
	RAVE_EEPROM_DDS_PAGE_FLAGS2             = 5,
	RAVE_EEPROM_DDS_PAGE_FLAGS3             = 6,
	RAVE_EEPROM_DDS_PAGE_ENGINEERING_TESTS  = 7,
	RAVE_EEPROM_DDS_PAGE_SDUMAINT           = 8,
	RAVE_EEPROM_DDS_PAGE_ASSEMBLY_PARTNUM   = 9,
	RAVE_EEPROM_DDS_PAGE_ASSEMBLY_SERIALNUM = 10,
	RAVE_EEPROM_DDS_PAGE_ASSEMBLY_DOM       = 11,
	RAVE_EEPROM_DDS_PAGE_BOARD_REV          = 12,
	RAVE_EEPROM_DDS_PAGE_BOARD_REV_UPDATE   = 13,
	RAVE_EEPROM_DDS_PAGE_BOARD_DOM_UPDATE   = 14,
} rave_eeprom_dds_page_t;

/* DDS EEPROM page 0 */
#define RAVE_EEPROM_DDS_PAGE_FORMAT_MANUFACTURE_OFFSET 0x10

/* DDS EEPROM page 4 */
#define RAVE_EEPROM_DDS_PAGE_FLAGS1_SEAT_CLASS_OFFSET    3
#define RAVE_EEPROM_DDS_PAGE_FLAGS1_DISPLAY_TYPE_OFFSET  4
#define RAVE_EEPROM_DDS_PAGE_FLAGS1_LOCATION_TYPE_OFFSET 5
#define RAVE_EEPROM_DDS_PAGE_FLAGS1_SEAT_NUMBER_OFFSET   6
#define RAVE_EEPROM_DDS_PAGE_FLAGS1_SEAT_LETTER_OFFSET   7

typedef enum {
	RAVE_EEPROM_DDS_SEAT_CLASS_UNKNOWN           = -1,
	RAVE_EEPROM_DDS_SEAT_CLASS_UNSET             = 0x00,
	RAVE_EEPROM_DDS_SEAT_CLASS_ECONOMY           = 0x01,
	RAVE_EEPROM_DDS_SEAT_CLASS_ECONOMY_PREMIERE  = 0x02,
	RAVE_EEPROM_DDS_SEAT_CLASS_BUSINESS          = 0x03,
	RAVE_EEPROM_DDS_SEAT_CLASS_BUSINESS_PREMIERE = 0x04,
	RAVE_EEPROM_DDS_SEAT_CLASS_FIRST             = 0x05,
	RAVE_EEPROM_DDS_SEAT_CLASS_FLIGHT_CREW_REST  = 0x06,
	RAVE_EEPROM_DDS_SEAT_CLASS_CABIN_CREW_REST   = 0x07,
	RAVE_EEPROM_DDS_SEAT_CLASS_MAX               = 0x08
} rave_eeprom_dds_seat_class_t;

const char *rave_eeprom_dds_seat_class_to_string (rave_eeprom_dds_seat_class_t type);
rave_eeprom_dds_seat_class_t rave_eeprom_dds_seat_class_from_string(const char *str);

typedef enum {
	RAVE_EEPROM_DDS_DISPLAY_TYPE_UNKNOWN    = -1,
	RAVE_EEPROM_DDS_DISPLAY_TYPE_UNSET      = 0x00,
	RAVE_EEPROM_DDS_DISPLAY_TYPE_SEAT       = 0x01,
	RAVE_EEPROM_DDS_DISPLAY_TYPE_CREW_PANEL = 0x02,
	RAVE_EEPROM_DDS_DISPLAY_TYPE_BCO        = 0x03,
	RAVE_EEPROM_DDS_DISPLAY_TYPE_HOTSPARE   = 0x04,
	RAVE_EEPROM_DDS_DISPLAY_TYPE_MAX        = 0x05
} rave_eeprom_dds_display_type_t;

const char *rave_eeprom_dds_display_type_to_string (rave_eeprom_dds_display_type_t type);
rave_eeprom_dds_display_type_t rave_eeprom_dds_display_type_from_string(const char *str);

typedef enum {
	RAVE_EEPROM_DDS_LOCATION_TYPE_UNKNOWN       = -1,
	RAVE_EEPROM_DDS_LOCATION_TYPE_UNSET         = 0x00,
	RAVE_EEPROM_DDS_LOCATION_TYPE_STANDARD_SEAT = 0x01,
	RAVE_EEPROM_DDS_LOCATION_TYPE_CREW_REST     = 0x02,
	RAVE_EEPROM_DDS_LOCATION_TYPE_BULKHEAD      = 0x03,
	RAVE_EEPROM_DDS_LOCATION_TYPE_OVERHEAD      = 0x04,
	RAVE_EEPROM_DDS_LOCATION_TYPE_LASTROW       = 0x05,
	RAVE_EEPROM_DDS_LOCATION_TYPE_MISC          = 0x06,
	RAVE_EEPROM_DDS_LOCATION_TYPE_SCU           = 0x07,
	RAVE_EEPROM_DDS_LOCATION_TYPE_DISABLED      = 0x08,
	RAVE_EEPROM_DDS_LOCATION_TYPE_NIU           = 0x09,
	RAVE_EEPROM_DDS_LOCATION_TYPE_LIU           = 0x0A,
	RAVE_EEPROM_DDS_LOCATION_TYPE_ODB           = 0x0B,
	RAVE_EEPROM_DDS_LOCATION_TYPE_MAINTENANCE_PORT = 0x0C,
	RAVE_EEPROM_DDS_LOCATION_TYPE_CFU           = 0x0D,
	RAVE_EEPROM_DDS_LOCATION_TYPE_SPU           = 0x0E,
	RAVE_EEPROM_DDS_LOCATION_TYPE_SPB           = 0x0F,
	RAVE_EEPROM_DDS_LOCATION_TYPE_MAX           = 0x10
} rave_eeprom_dds_location_type_t;

const char *rave_eeprom_dds_location_type_to_string(rave_eeprom_dds_location_type_t type);
rave_eeprom_dds_location_type_t rave_eeprom_dds_location_type_from_string(const char *str);

/* DDS EEPROM Page 6 */
#define RAVE_EEPROM_DDS_PAGE_FLAGS3_PWR_CFG_BITS_OFFSET 0

typedef enum {
	RAVE_EEPROM_DDS_USB_POWER_NONE = 0,
	RAVE_EEPROM_DDS_USB_POWER_RJU  = 1 << 0,
	RAVE_EEPROM_DDS_USB_POWER_USER = 1 << 1,
} rave_eeprom_dds_usb_power_t;

int rave_eeprom_dds_open(int mode);
int rave_eeprom_dds_read_page(int fd, rave_eeprom_dds_page_t page_number, uint8_t *page);
int rave_eeprom_dds_write_page(int fd, rave_eeprom_dds_page_t page_number, uint8_t *page);

#endif
