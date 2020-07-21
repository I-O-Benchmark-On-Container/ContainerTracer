/****************************************************************************
 * Block I/O Trace Replayer 
 * Yongseok Oh (ysoh@uos.ac.kr) 2013 - 2014

 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; under version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/

#include <trace_replay.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <sgio.h>

#ifndef ENOIOCTLCMD
#define ENOIOCTLCMD ENOTTY
#endif

int timeval_subtract(struct timeval *result, struct timeval *x,
                     struct timeval *y);
int verbose;

int disk_open(const char *dev, int flag)
{
        int fd;

        fd = open(dev, flag);
        if (fd < 0)
                printf("device open error\n");

        //	printf("open fd = %d\n",fd);

        return fd;
}

void flush_buffer_cache(int fd)
{
        fsync(fd); /* flush buffers */
        if (ioctl(fd, BLKFLSBUF, NULL)) /* do it again, big time */
                perror("BLKFLSBUF failed");
        /* await completion */
        if (do_drive_cmd(fd, NULL) && errno != EINVAL && errno != ENOTTY &&
            errno != ENOIOCTLCMD)
                perror("HDIO_DRIVE_CMD(null) (wait for flush complete) failed");
}

void disk_close(int fd)
{
        flush_buffer_cache(fd);
        close(fd);
}
