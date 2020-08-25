/**
 * @copyright "Container Tracer" which executes the container performance mesurements
 * Copyright (C) 2020 SuhoSon
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
 * @file docker-mq.c
 * @brief This has the contents of creating and using Message Queue.
 * @author SuhoSon (ngeol564@gmail.com)
 * @version 0.1
 * @date 2020-08-19
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <fcntl.h>

#include <trace_replay.h>
#include <log.h>
#include <driver/docker-driver.h>

/**
 * @brief Initialize the Message Queue.
 *
 * @param[in] info `docker_info` structure which wants to init.
 *
 * @return Message ID for success to init, negative value for fail to init.
 */
static int __docker_mq_init(struct docker_info *info)
{
        char *mq_path;
        key_t mq_key = 0;
        int mqid = -1, ret = -1;

        mq_path = (char *)malloc(BASE_KEY_PATHNAME_LEN);
        if (!mq_path) {
                pr_info(ERROR, "Memory allocation fail. (\"%s\")", "mq_path");
                ret = -ENOMEM;
                goto exception;
        }
        snprintf(mq_path, BASE_KEY_PATHNAME_LEN, "/tmp/%s%s_%d",
                 info->cgroup_id, MSGQ_KEY_PATHNAME, info->pid);

        /* Create the file if there isn't exist the directory in `mq_path` */
        (void)close(open(mq_path, O_WRONLY | O_CREAT, 0));

        if (0 > (mq_key = ftok(mq_path, PROJECT_ID))) {
                pr_info(ERROR, "Key generation failed (name: %s, token: %c)\n",
                        mq_path, PROJECT_ID);
                ret = -ENOKEY;
                goto exception;
        }

        if (0 > (mqid = msgget(mq_key, IPC_CREAT | PROJECT_PERM))) {
                pr_info(ERROR, "Message Queue get failed (key: %d)\n", mq_key);
                ret = -EINVAL;
                goto exception;
        }

        free(mq_path);
        ret = mqid;
        return ret;
exception:
        if (mq_path) {
                free(mq_path);
        }
        if (0 <= mqid) {
                msgctl(mqid, IPC_RMID, NULL);
        }
        mqid = ret;
        return mqid;
}

/**
 * @brief Wrapping function of `__docker_mq_init()`
 *
 * @param[in] info `docker_info` structure which wants to init.
 *
 * @return 0 for success to init, negative value for fail to init.
 */
int docker_mq_init(struct docker_info *info)
{
        int mqid = -1;
        int ret = 0;

        assert(NULL != info);
        assert(0 != info->pid);

        if (0 > (mqid = __docker_mq_init(info))) {
                pr_info(ERROR,
                        "Message Queue initialization fail. (target pid :%d)\n",
                        info->pid);
                ret = mqid;
                goto exit;
        }
        pr_info(INFO, "Message Queue create success. (path: /tmp/%s%s_%d)\n",
                info->cgroup_id, SHM_KEY_PATHNAME, info->pid);

        info->mqid = mqid;

exit:
        return ret;
}

/**
 * @brief Retrieve the data from Message Queue.
 *
 * @param[in] info `docker_info` structure which wants to get data.
 * @param[out] buffer Destination of data will be stored
 *
 * @return 0 for success to init, negative value for fail to init.
 */
int docker_mq_get(const struct docker_info *info, void *buffer)
{
        struct realtime_msg rmsg;

        assert(NULL != buffer);
        assert(NULL != info);
        assert(-1 != info->mqid);

        if (0 > msgrcv(info->mqid, &rmsg, sizeof(struct realtime_log), 0, 0)) {
                pr_info(ERROR, "Cannot get message queue (mqid: %d)\n",
                        info->mqid);
                return -EFAULT;
        }
        memcpy(buffer, &rmsg.log, sizeof(struct realtime_log));
        return 0;
}

/**
 * @brief Deallocate the Message Queue resources.
 *
 * @param[in] info `docker_info` structure which wants to deallocate.
 * @param[in] flags Set a range of deallocation.
 */
void docker_mq_free(struct docker_info *info, int flags)
{
        int mqid;

        assert(NULL != info);

        mqid = info->mqid;

        if ((DOCKER_IPC_FREE & flags) && 0 <= mqid) {
                msgctl(mqid, IPC_RMID, NULL);
        }

        info->mqid = -1;
}
