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
 * @file sync.h
 * @brief 파이프를 활용한 동기화 도구입니다.
 * @details 파이프 크기는 Linux의 경우에는 약 4KB 입니다. 동기화 방식은 read, write의 Blocking 특성을 활용하여 개발되었습니다.
 * @author BlaCkinkGJ (ss5kijun@gmail.com)
 * @version 0.1
 * @date 2020-08-09
 * @note Rochkind, M. J. (2004). Advanced UNIX programming. Pearson Education.
 */

#ifndef SYNC_H
#define SYNC_H

#include <stdio.h>
#include <unistd.h>
#include <errno.h>

/* 0: read 1: write*/
static int pfd1[2], pfd2[2];

/**
 * @brief 파이프를 만들어주는 과정으로 부모가 자식을 부르기 전에 이를 반드시 호출해야 합니다.
 *
 * @return 성공한 경우에는 0 실패한 경우에는 -EPIPE를 반환합니다.
 */
static inline int TELL_WAIT(void)
{
        if (pipe(pfd1) < 0 || pipe(pfd2) < 0) {
                fprintf(stderr, "[%s] pipe error...\n", __func__);
                return -EPIPE;
        }
        return 0;
}

/**
 * @brief 자식이 부모를 호출하는 역할을 합니다.
 *
 * @return 성공한 경우에는 0 실패한 경우에는 -EIO를 반환합니다.
 * @warning 특정 부모가 아닌 임의의 부모를 호출합니다.
 */
static inline int TELL_PARENT(void)
{
        if (write(pfd2[1], "c", 1) != 1) {
                fprintf(stderr, "[%s] write error...\n", __func__);
                return -EIO;
        }
        return 0;
}

/**
 * @brief 부모에게 완료되었다는 메시지를 수신합니다.
 *
 * @return 성공한 경우에는 0 실패한 경우에는 -EIO를 반환합니다.
 */
static inline int WAIT_PARENT(void)
{
        char recv;

        if (read(pfd1[0], &recv, 1) != 1) {
                fprintf(stderr, "[%s] read error...\n", __func__);
                return -EIO;
        }

        if (recv != 'p') {
                fprintf(stderr, "[%s] incorrect data received(%c)\n", __func__,
                        recv);
                return -EIO;
        }
        return 0;
}

/**
 * @brief 부모가 자식을 호출하는 역할을 합니다.
 *
 * @return 성공한 경우에는 0 실패한 경우에는 -EIO를 반환합니다.
 * @warning 특정 자식이 아닌 임의의 자식을 호출합니다.
 */
static inline int TELL_CHILD(void)
{
        if (write(pfd1[1], "p", 1) != 1) {
                fprintf(stderr, "[%s] write error...\n", __func__);
                return -EIO;
        }
        return 0;
}

/**
 * @brief 자식에서 완료되었다는 메시지를 수신합니다.
 *
 * @return 성공한 경우에는 0 실패한 경우에는 -EIO를 반환합니다.
 */
static inline int WAIT_CHILD(void)
{
        char recv;

        if (read(pfd2[0], &recv, 1) != 1) {
                fprintf(stderr, "[%s] read error...\n", __func__);
                return -EIO;
        }

        if (recv != 'c') {
                fprintf(stderr, "[%s] incorrect data received(%c)\n", __func__,
                        recv);
                return -EIO;
        }
        return 0;
}

#endif
