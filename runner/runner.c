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
 * @file runner.c
 * @brief Definition of `runner.h` declaration contents.
 * @author BlaCkinkGJ (ss5kijun@gmail.com)
 * @version 0.1
 * @date 2020-08-04
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <linux/limits.h>

#include <jemalloc/jemalloc.h>
#include <json.h>

#include <generic.h>
#include <runner.h>
#include <log.h>

static struct runner_config *global_config =
        NULL; /**< Global configuration contents of runner */

/**
 * @brief Deallocate the global_runner's contents.
 *
 * @param[in] config pointer of `runner_config` structure.
 * @param[in] flags Flag of deallocation range.
 */
void runner_config_free(struct runner_config *config, const int flags)
{
        if (NULL == config) {
                pr_info(WARNING, "invalid config location: %p\n", config);
                return;
        }

        if (RUNNER_FREE_ALL != (flags & RUNNER_FREE_ALL_MASK)) {
                pr_info(WARNING,
                        "Nothing occurred in this functions... (flags: 0x%X)\n",
                        flags);
                return;
        }

        if ((flags & RUNNER_FREE_DRIVER_MASK) && (NULL != config->op.free)) {
                (config->op.free)();
        }

        if (NULL != config->setting) {
                json_object_put(config->setting);
                config->setting = NULL;
                pr_info(INFO,
                        "setting deallcated success: expected (nil) ==> %p\n",
                        config->setting);
        }
        config->driver[0] = '\0';

        free(config);
}

/**
 * @brief Read the JSON string and config the runner.
 *
 * @param[in] json_str JSON string which is used in runner config.
 *
 * @return 0 for success to init, error number for fail to init.
 *
 * @exception Error number returns if there exists any invalid value in JSON string.
 */
int runner_init(const char *json_str)
{
        struct json_object *json_obj, *tmp;
        int ret = 0;
        assert(NULL != json_str);

        global_config =
                (struct runner_config *)malloc(sizeof(struct runner_config));
        assert(NULL != global_config);

        memset(global_config, 0, sizeof(struct runner_config));

        json_obj = json_tokener_parse(json_str);
        if (!json_obj) {
                pr_info(ERROR, "Parse failed: \"%s\"\n", json_str);
                ret = -EINVAL;
                goto exception;
        }

        if (!json_object_object_get_ex(json_obj, "driver", &tmp)) {
                pr_info(ERROR, "Not exist error (key: %s)\n", "driver");
                ret = -EACCES;
                goto exception;
        };
        snprintf(global_config->driver, sizeof(global_config->driver), "%s",
                 json_object_to_json_string_ext(tmp, JSON_C_TO_STRING_PLAIN));
        generic_strip_string(global_config->driver, '\"');
        pr_info(INFO, "driver ==> %s\n", global_config->driver);
        if (!json_object_object_get_ex(json_obj, "setting", &tmp)) {
                pr_info(ERROR, "Not exist error (key: %s)\n", "setting");
                ret = -EACCES;
                goto exception;
        };

        global_config->setting = tmp;

        ret = generic_driver_init(global_config->driver, global_config);
        if (0 > ret) {
                pr_info(ERROR, "Initialize failed... (errno: %d)\n", ret);
                goto exception;
        }

        pr_info(INFO, "Successfully binding driver: %s\n",
                global_config->driver);
        if (NULL != json_obj) {
                json_object_put(json_obj);
                json_obj = NULL;
        }
        global_config->setting = NULL;
        return ret;
exception:
        errno = -ret;
        if (NULL != json_obj) {
                json_object_put(json_obj);
                json_obj = NULL;
        }
        global_config->setting = NULL;
        runner_config_free(global_config, RUNNER_FREE_ALL);
        global_config = NULL;
        return ret;
}

/**
 * @brief Execute the benchmark program.
 *
 * @return Driver's execution result.
 */
int runner_run(void)
{
        return (global_config->op.runner)();
}

/**
 * @brief Wrapping function of `__runner_free()`.
 */
void runner_free(void)
{
        runner_config_free(global_config, RUNNER_FREE_ALL);
        global_config = NULL;
        pr_info(INFO, "runner free success (flags: 0x%X)\n", RUNNER_FREE_ALL);
}

/**
 * @brief Generate the buffer which contains the result.
 *
 * @param[out] buffer Destination buffer which wants to allocate the memory.
 * @param[in] size Buffer's size.
 *
 * @return 0 for buffer allocation success, -EINVAL for buffer allocation fail.
 */
static int runner_get_result_string(char **buffer, size_t size)
{
        *buffer = (char *)malloc(size);
        if (!*buffer) {
                pr_info(WARNING, "Memory allocation failed. (%s)\n", "buffer");
                return -EINVAL;
        }
        return 0;
}

/**
 * @brief Deallocate the buffer which is allocated by `runner_get_result_string()` function.
 *
 * @param[in] buffer A dynamically allocated buffer which wants to deallocate.
 */
void runner_put_result_string(char *buffer)
{
        if (NULL != buffer) {
                free(buffer);
        }
}

/**
 * @brief Get a specific driver's execution-time results.
 *
 * @param[in] key The key which specifies target to get execution-time result.
 *
 * @return Return JSON string which contains the execution-time results.
 * However, if allocates fail then returns NULL.
 *
 * @warning Due to the buffer was allocated by `runner_get_result_string()`,
 * you must deallocate this buffer by using `runner_put_result_string()`.
 */
char *runner_get_interval_result(const char *key)
{
        char *buffer = NULL;
        int ret = 0;

        ret = runner_get_result_string(&buffer, INTERVAL_RESULT_STRING_SIZE);
        if (0 > ret) {
                goto exception;
        }

        ret = (global_config->op.get_interval)(key, buffer);
        if (0 > ret) {
                goto exception;
        }

        return buffer;
exception:
        errno = -ret;
        perror("Error detected while running");
        runner_put_result_string(buffer);
        buffer = NULL;
        return buffer;
}

/**
 * @brief Get a specific driver's end-time results.
 *
 * @param[in] key The key which specifies target to get end-time result.
 *
 * @return Return JSON string which contains the end-time results.
 * However, if allocates fail then returns NULL.
 *
 * @warning Due to the buffer was allocated by `runner_get_result_string()`,
 * you must deallocate this buffer by using `runner_put_result_string()`.
 */
char *runner_get_total_result(const char *key)
{
        char *buffer = NULL;
        int ret = 0;

        ret = runner_get_result_string(&buffer, TOTAL_RESULT_STRING_SIZE);
        if (0 > ret) {
                goto exception;
        }

        ret = (global_config->op.get_total)(key, buffer);
        if (0 > ret) {
                goto exception;
        }

        return buffer;
exception:
        errno = -ret;
        perror("Error detected while running");
        runner_put_result_string(buffer);
        buffer = NULL;
        return buffer;
}

/**
 * @brief Get a global configuration pointer.
 *
 * @return `global_config` pointer.
 * @note You must use only on debugging or restricted environments.
 */
const struct runner_config *runner_get_global_config(void)
{
        return global_config;
}
