/*
 * rave-eeprom-main.h
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

#ifndef __RAVE_EEPROM_MAIN_H
#define __RAVE_EEPROM_MAIN_H

#include <stdlib.h>

#define RAVE_EEPROM_MAIN_PATH "/dev/main-eeprom"

/* Page numbers */
typedef enum {
	RAVE_EEPROM_MAIN_PAGE_FORMAT                    = 0,
	RAVE_EEPROM_MAIN_PAGE_PARTNUM                   = 1,
	RAVE_EEPROM_MAIN_PAGE_CONFIG                    = 2,
	RAVE_EEPROM_MAIN_PAGE_SERIAL                    = 3,
	RAVE_EEPROM_MAIN_PAGE_FLAGS1                    = 4,
	RAVE_EEPROM_MAIN_PAGE_FLAGS2                    = 5,
	RAVE_EEPROM_MAIN_PAGE_BOARD_DOM                 = 6,
	RAVE_EEPROM_MAIN_PAGE_BOARD_PARTNUM             = 7,
	RAVE_EEPROM_MAIN_PAGE_BOARD_SERIAL              = 8,
	RAVE_EEPROM_MAIN_PAGE_BOARD_REV                 = 9,
	RAVE_EEPROM_MAIN_PAGE_BOARD_REV_UPDATE          = 10,
	RAVE_EEPROM_MAIN_PAGE_BOARD_DOM_UPDATE          = 11,
	RAVE_EEPROM_MAIN_PAGE_SDUMAINT                  = 24,
	RAVE_EEPROM_MAIN_PAGE_PRIMARY_ENC_TEST_MSG      = 54,
	RAVE_EEPROM_MAIN_PAGE_PRIMARY_PLAYER_KEY_INFO   = 63,
	RAVE_EEPROM_MAIN_PAGE_PRIMARY_PLAYER_KEY        = 64,
	RAVE_EEPROM_MAIN_PAGE_PLAYER_KEYS               = 64,
	RAVE_EEPROM_MAIN_PAGE_ENCRYPTED_TEST_MSG_SIZE   = 256,
	RAVE_EEPROM_MAIN_PAGE_SECONDARY_ENC_TEST_MSG    = 438,
	RAVE_EEPROM_MAIN_PAGE_SECONDARY_PLAYER_KEY_INFO = 447,
	RAVE_EEPROM_MAIN_PAGE_SECONDARY_PLAYER_KEY      = 448,
	RAVE_EEPROM_MAIN_PAGE_MAX
} rave_eeprom_main_page_t;

const char *rave_eeprom_main_page_to_string(rave_eeprom_main_page_t page_number);
typedef void (rave_eeprom_main_page_foreach_func)(rave_eeprom_main_page_t page_number, void *user_data);
void rave_eeprom_main_page_foreach(rave_eeprom_main_page_foreach_func callback, void *user_data);

/* MAIN EEPROM page 0 */
#define RAVE_EEPROM_MAIN_PAGE_FORMAT_MANUFACTURE_OFFSET 0x10

#endif
