#ifndef SYNC_H
#define SYNC_H

#include <stdio.h>
#include <unistd.h>
#include <errno.h>

/* 0: read 1: write*/
static int pfd1[2], pfd2[2];

int TELL_WAIT(void)
{
        if (pipe(pfd1) < 0 || pipe(pfd2) < 0) {
                fprintf(stderr, "[%s] pipe error...\n", __func__);
                return -EPIPE;
        }
        return 0;
}

int TELL_PARENT(pid_t pid)
{
        (void)pid;
        if (write(pfd2[1], "c", 1) != 1) {
                fprintf(stderr, "[%s] write error...\n", __func__);
                return -EIO;
        }
        return 0;
}

int WAIT_PARENT(void)
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

int TELL_CHILD(pid_t pid)
{
        (void)pid;
        if (write(pfd1[1], "p", 1) != 1) {
                fprintf(stderr, "[%s] write error...\n", __func__);
                return -EIO;
        }
        return 0;
}

int WAIT_CHILD(void)
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
