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
 * @file generic.c
 * @brief This makes runner to determine the driver in runtime.
 * @author BlaCkinkGJ (ss5kijun@gmail.com)
 * @version 0.1
 * @date 2020-08-05
 * @note reference link: https://github.com/mit-pdos/xv6-public/blob/master/syscall.c
 */

#include <generic.h>
#include <driver/tr-driver.h>
#include <driver/docker-driver.h>

const char *driver_name_tbl[] = {
        [TRACE_REPLAY_DRIVER] = "trace-replay",
        [DOCKER_DRIVER] = "docker",
        NULL,
}; /**< Associated table of each driver. */

static int (*driver_init_tbl[])(void *object) = {
        [TRACE_REPLAY_DRIVER] = tr_init,
        [DOCKER_DRIVER] = docker_init,
}; /**< Associated initialization table of each driver. This uses in `generic_driver_init()` */

/**
 * @brief Get the index of the name based on an associated table.
 *
 * @param[in] name The name of driver.
 *
 * @return index number which related to name
 * @exception -EINVAL if doesn't exist the name in associated table.
 */
int get_generic_driver_index(const char *name)
{
        int index = 0;
        while (driver_name_tbl[index] != NULL) {
                if (!strcmp(driver_name_tbl[index], name)) {
                        return index;
                }
                index++;
        }
        return -EINVAL;
}

/**
 * @brief This is a generic initialization function that performs initialization of driver corresponding to the driver name.
 *
 * @param[in] name driver's name.
 * @param[in] object The object which wants to initialize. Usually, global_config in this place.
 *
 * @return 0 for success to init, other value for fail to init.
 * @exception -EINVAL if doesn't exist the name in associated table.
 */
int generic_driver_init(const char *name, void *object)
{
        int num = get_generic_driver_index(name);
        if (num < 0) {
                return -EINVAL;
        }

        return (driver_init_tbl[num])(object);
}
