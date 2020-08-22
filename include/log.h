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
 * @file log.h
 * @brief 로그 출력 양식에 해당합니다.
 * @author BlaCkinkGJ (ss5kijun@gmail.com)
 * @version 0.1
 * @date 2020-08-05
 */
#ifndef _LOG_H
#define _LOG_H
#include <time.h>
#include <unistd.h>
#include <stdio.h>

#define INFO "INFO"
#define WARNING "WARNING"
#define ERROR "ERROR"

/**
 * @details 각각 로그 출력 범위는 아래와 같습니다.
 * - `LOG_INFO`: INFO, WARNING, ERROR에 해당하는 로그를 출력합니다.
 * - `LOG_WARNING`: WARNING, ERROR에 해당하는 로그를 출력합니다.
 * - `LOG_ERROR`: ERROR에 해당하는 로그만 출력합니다.
 * - `UNDEFINED`: 아무것도 출력하지 않습니다.
 */

#if defined(LOG_INFO)
#define pr_info(level, msg, ...)                                               \
        fprintf(stderr, "(" level ")[{%lfs} %s(%s):%d] (pid: %d) " msg,        \
                ((double)clock() / CLOCKS_PER_SEC), __FILE__, __func__,        \
                __LINE__, getpid(), ##__VA_ARGS__)
#elif defined(LOG_WARNING)
#include <string.h>
#define pr_info(level, msg, ...)                                               \
        do {                                                                   \
                if (!strcmp(level, WARNING) || !strcmp(level, ERROR))          \
                        fprintf(stderr,                                        \
                                "(" level                                      \
                                ")[{%lfs} %s(%s):%d] (pid: %d) " msg,          \
                                ((double)clock() / CLOCKS_PER_SEC), __FILE__,  \
                                __func__, __LINE__, getpid(), ##__VA_ARGS__);  \
        } while (0)
#elif defined(LOG_ERROR)
#include <string.h>
#define pr_info(level, msg, ...)                                               \
        do {                                                                   \
                if (!strcmp(level, ERROR))                                     \
                        fprintf(stderr,                                        \
                                "(" level                                      \
                                ")[{%lfs} %s(%s):%d] (pid: %d) " msg,          \
                                ((double)clock() / CLOCKS_PER_SEC), __FILE__,  \
                                __func__, __LINE__, getpid(), ##__VA_ARGS__);  \
        } while (0)
#else
#define pr_info(level, msg, ...)                                               \
        do {                                                                   \
                (void)level;                                                   \
                (void)msg;                                                     \
        } while (0)
#endif

#endif
