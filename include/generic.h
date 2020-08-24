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
 * @brief driver의 매핑 정보 및 runner를 총괄하는 헤더입니다. 관련 정의 내용은 `runner/generic.c`에 존재합니다.
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

enum { TRACE_REPLAY_DRIVER = 0,
       DOCKER_DRIVER = 1,
};

/**
 * @brief driver의 명령어 집합을 가지는 구조체입니다.
 */
struct generic_driver_op {
        int (*runner)(void); /**< 실제 수행 함수입니다. */
        int (*get_interval)(const char *key,
                            char *buffer); /**< 중간 값을 받는 함수입니다. */
        int (*get_total)(const char *key,
                         char *buffer); /**< 종료 값을 받는 함수입니다. */
        void (*free)(void); /**< 자원을 해제하는 함수를 가리키는 포인터입니다. */
};

int get_generic_driver_index(const char *name);
int generic_driver_init(const char *name, void *object);

/**
 * @brief json의 문자열 값을 받는 경우에는 \" 값이 있을 수 있습니다. 따라서, 그 값을 전부 없애주는 데 사용합니다.
 *
 * @param str 입력되는 문자열에 해당합니다.
 * @param ch 지우고자 하는 단어에 해당합니다.
 *
 * @note 이 함수는 입력된 문자열을 선형적으로 읽기에 성능에 부정적인 영향을 끼칠 수 있습니다.
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
