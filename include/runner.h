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
 * @brief runner에 대한 선언적 내용이 들어가게 됩니다.
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

#define INTERVAL_RESULT_STRING_SIZE (PAGE_SIZE) /**< 4KB 예상 */
#define TOTAL_RESULT_STRING_SIZE (PAGE_SIZE * PAGE_SIZE) /**< 16MB 예상 */

enum { RUNNER_FREE_ALL_MASK = 0xFFFF, /**< 전체를 해제하도록 합니다. */
       RUNNER_FREE_DRIVER_MASK =
               0x0001, /**< Driver 내용만 해제하도록 합니다. */
};

enum { RUNNER_FREE_ALL =
               0xFFFF, /**< 동적으로 할당된 모든 설정 내역을 지울 때 사용합니다. */
};

/**
 * @brief 사용자가 입력한 설정 정보가 들어가게 됩니다.
 */
struct runner_config {
        char driver[PATH_MAX]; /**< 현재 사용자가 지정한 driver의 이름을 지칭합니다. 입력된 json에서 "driver" 키에 대응되는 값입니다. */
        struct generic_driver_op
                op; /**< driver의 추상화된 명령어 함수들을 가지고 있는 멤버입니다. */
        struct json_object *
                setting; /**< 현재 사용자가 지정한 json 설정 값이 들어갑니다. 입력된 json에서 "setting" 키에 대응되는 값입니다. */
};

int runner_init(const char *json_str);
int runner_run(void);
char *runner_get_interval_result(const char *key);
char *runner_get_total_result(const char *key);
void runner_put_result_string(char *buffer);
void runner_free(void);
void runner_config_free(struct runner_config *config, const int flags);
const struct runner_config *runner_get_global_config(void);

#endif
