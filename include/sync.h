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
 * @brief Synchronization program based on pipe.
 * @details In Linux, the size of pipe is about 4KB. And the synchronization method is implemented by using R/W's blocking specification.
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
 * @brief Pipe initialize sequence. You must call before use this source.
 *
 * @return 0 for success to make pipe, -EPIPE for fail to make pipe.
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
 * @brief The child calls parents.
 *
 * @return 0 for success to write, -EIO for fail to write
 * @note This doesn't wake up a specific parent. This wakes randomly selected a parent.
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
 * @brief Receive from parent's call.
 *
 * @return 0 for success to read, -EIO for fail to read.
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
 * @brief The parent calls children.
 *
 * @return 0 for success to write, -EIO for fail to write
 * @note This doesn't wake up a specific child. This wakes randomly selected a child.
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
 * @brief Receive from child's call.
 *
 * @return 0 for success to read, -EIO for fail to read.
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
