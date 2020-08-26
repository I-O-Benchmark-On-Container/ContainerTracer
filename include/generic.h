/**
 * @copyright "Container Tracer" which executes the container performance mesurements
 * Copyright (C) 2020 BlaCkinkGJ
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * @file generic.h
 * @brief This header maintain the driver's mapping information. It's definition in `runner/generic.c`.
 * @author BlaCkinkGJ (ss5kijun@gmail.com)
 * @version 0.1
 * @date 2020-08-05
 */
#ifndef _GENERIC_H
#define _GENERIC_H

#define MAX_DRIVERS 10

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <linux/limits.h>

#include <json.h>

#include <log.h>

enum { TRACE_REPLAY_DRIVER = 0, /**< tr-driver index */
       DOCKER_DRIVER = 1, /**< docker-driver index */
};

/**
 * @brief This structure contains the driver's command set.
 */
struct generic_driver_op {
        int (*runner)(void); /**< Run the benchmark program. */
        int (*get_interval)(const char *key,
                            char *buffer); /**< Get execution-time result. */
        int (*get_total)(const char *key,
                         char *buffer); /**< Get end-time result. */
        void (*free)(
                void); /**< Deallocation of driver's dynamic allocated resources. */
};

int get_generic_driver_index(const char *name);
int generic_driver_init(const char *name, void *object);

/**
 * @brief Remove the word <ch> in string.
 *
 * @param str Inputted string.
 * @param ch The word which I want to remove.
 *
 * @note This function can degrade the total performance of the program due to linear searching.
 */
static inline void generic_strip_string(char *str, char ch)
{
        int i = 0, j = 0;
        while (i < PATH_MAX && str[j] != '\0') {
                if (str[i] == ch) {
                        i++;
                        continue;
                }
                str[j++] = str[i++];
        }
}

#endif
