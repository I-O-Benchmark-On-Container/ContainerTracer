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
 * @file runner.h
 * @brief Declaration information of `runnner`
 * @author BlaCkinkGJ (ss5kijun@gmail.com)
 * @version 0.1
 * @date 2020-08-04
 */
#ifndef _RUNNER_H
#define _RUNNER_H

#include <linux/limits.h>
#include <sys/user.h>

#include <json.h>

#include <generic.h>

#define INTERVAL_RESULT_STRING_SIZE (PAGE_SIZE) /**< Expected 4KB */
#define TOTAL_RESULT_STRING_SIZE (PAGE_SIZE * PAGE_SIZE) /**< Expected 16MB */

#define BFQ_MIN_WEIGHT 1
#define BFQ_MAX_WEIGHT 1000

enum { RUNNER_FREE_ALL_MASK =
               0xFFFF, /**< This mask checks the flag want to deallocate all. */
       RUNNER_FREE_DRIVER_MASK =
               0x0001, /**< This mask checks the flag want to deallocate driver */
};

enum { RUNNER_FREE_ALL = 0xFFFF, /**< Deallocate the whole runner contents. */
};

/**
 * @brief This structure has user-inputted information.
 */
struct runner_config {
        char driver[PATH_MAX]; /**< This points that current driver of this runner. This uses in the `runner/generic.c` and relates `driver` field in user-inputted JSON. */
        struct generic_driver_op
                op; /**< This is an abstracted operation set that can be overridden by the driver. */
        struct json_object *
                setting; /**< This field has the driver dependant JSON object. This relates `setting` field in user-inputted JSON. */
};

int runner_init(const char *json_str);
int runner_run(void);
char *runner_get_interval_result(const char *key);
char *runner_get_total_result(const char *key);
void runner_put_result_string(char *buffer);
void runner_free(void);
void runner_config_free(struct runner_config *config, const int flags);
const struct runner_config *runner_get_global_config(void);

/**
 * @brief Check the validation of BFQ scheduler's weight.
 *
 * @param[in] weight BFQ scheduler's weight.
 *
 * @return 1 for valid BFQ weight, 0 for invalid BFQ weight.
 */
static inline int runner_is_valid_bfq_weight(unsigned int weight)
{
        return (weight <= BFQ_MAX_WEIGHT && weight >= BFQ_MIN_WEIGHT);
}

#endif
