/*
 * rave-eeprom.h
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

#ifndef __RAVE_EEPROM_H
#define __RAVE_EEPROM_H

int rave_eeprom_dump(int dump_main,
		     int dump_dds,
		     bool fail_on_crc_errors);

int rave_eeprom_check(int check_main,
                      int check_dds);

int rave_eeprom_update_dds_page_4(const char *seat_class,
                                  const char *display_type,
                                  const char *location_type,
                                  const char *seat_location);

int rave_eeprom_update_dds_page_6(const char *rju_usb_power,
                                  const char *user_usb_power);

#endif
