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
 * @file docker-shm.c
 * @brief This has the contents of creating and using Shared Memory.
 * @author SuhoSon (ngeol564@gmail.com)
 * @version 0.1
 * @date 2020-08-19
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <fcntl.h>

#include <trace_replay.h>
#include <log.h>
#include <driver/docker-driver.h>

/**
 * @brief Initialize the Semaphore.
 *
 * @param[in] pid Process' ID of using this Semaphore.
 *
 * @return SemaphoreID for success to init, negative value for fail to init.
 */
static int __docker_sem_init(struct docker_info *info)
{
        char *sem_path = NULL;
        key_t sem_key = 0;
        int semid = -1, ret = 0;
        union {
                int val;
        } sem_attr;

        sem_attr.val = 0;

        sem_path = (char *)malloc(BASE_KEY_PATHNAME_LEN);
        if (!sem_path) {
                pr_info(ERROR, "Memory allocation fail. (\"%s\")", "sem_path");
                ret = -ENOMEM;
                goto exception;
        }

        snprintf(sem_path, BASE_KEY_PATHNAME_LEN, "/tmp/%s%s_%d",
                 info->cgroup_id, SEM_KEY_PATHNAME, info->pid);
        (void)close(open(sem_path, O_WRONLY | O_CREAT, 0));

        if (0 > (sem_key = ftok(sem_path, PROJECT_ID))) {
                pr_info(ERROR, "Key generation failed (name: %s, token: %c)\n",
                        sem_path, PROJECT_ID);
                ret = -ENOKEY;
                goto exception;
        }

        if (0 > (semid = semget(sem_key, 1, IPC_CREAT | PROJECT_PERM))) {
                pr_info(ERROR, "Semaphore get failed (key: %d)\n", sem_key);
                ret = -EINVAL;
                goto exception;
        }

        sem_attr.val = 0;
        if (0 > semctl(semid, 0, SETVAL, sem_attr)) {
                pr_info(ERROR, "Semaphore control failed (command: %d)\n",
                        SETVAL);
                ret = -EINVAL;
                goto exception;
        }

        free(sem_path);
        ret = semid;
        return ret;
exception:
        if (sem_path) {
                free(sem_path);
        }

        if (0 <= semid) {
                semctl(semid, 0, IPC_RMID, sem_attr);
        }

        semid = ret;
        return semid;
}

/**
 * @brief Initialize the Shared Memory.
 *
 * @param[in] pid Process' ID of using this Shared Memory.
 *
 * @return Shared MemoryID for success to init, negative value for fail to init.
 */
static int __docker_shm_init(struct docker_info *info)
{
        char *shm_path;
        key_t shm_key = 0;
        int shmid = -1, ret = -1;

        shm_path = (char *)malloc(BASE_KEY_PATHNAME_LEN);
        if (!shm_path) {
                pr_info(ERROR, "Memory allocation fail. (\"%s\")", "shm_path");
                ret = -ENOMEM;
                goto exception;
        }
        snprintf(shm_path, BASE_KEY_PATHNAME_LEN, "/tmp/%s%s_%d",
                 info->cgroup_id, SHM_KEY_PATHNAME, info->pid);

        /* If there doesn't exist the file then create the file. */
        (void)close(open(shm_path, O_WRONLY | O_CREAT, 0));

        if (0 > (shm_key = ftok(shm_path, PROJECT_ID))) {
                pr_info(ERROR, "Key generation failed (name: %s, token: %c)\n",
                        shm_path, PROJECT_ID);
                ret = -ENOKEY;
                goto exception;
        }

        if (0 > (shmid = shmget(shm_key, sizeof(struct total_results),
                                IPC_CREAT | PROJECT_PERM))) {
                pr_info(ERROR, "Shared Memory get failed (key: %d)\n", shm_key);
                ret = -EINVAL;
                goto exception;
        }

        free(shm_path);
        ret = shmid;
        return ret;
exception:
        if (shm_path) {
                free(shm_path);
        }
        if (0 <= shmid) {
                shmctl(shmid, IPC_RMID, NULL);
        }
        shmid = ret;
        return shmid;
}

/**
 * @brief Acquire the semaphore lock.
 *
 * @param[in] info This is an information pointer that wants to acquire the semaphore.
 */
static void docker_sem_wait(const struct docker_info *info)
{
        struct sembuf sop = {
                .sem_num = 0,
                .sem_op = -1,
                .sem_flg = SEM_UNDO,
        };

        pr_info(INFO, "Going to sleep (pid: %d)\n", info->pid);
        /* 1 means the number of operations. */
        assert(-1 != semop(info->semid, &sop, 1));
}

/**
 * @brief Release the semaphore lock.
 *
 * @param[in] info This is an information pointer that wants to release the semaphore.
 */
static void docker_sem_post(const struct docker_info *info)
{
        struct sembuf sop = {
                .sem_num = 0,
                .sem_op = 1,
                .sem_flg = SEM_UNDO,
        };

        pr_info(INFO, "Going to wake (pid: %d)\n", info->pid);
        assert(-1 != semop(info->semid, &sop, 1));
}

/**
 * @brief Do the `__docker_shm_init()` and `__docker_sem_init()`
 *
 * @param[in] info `docker_info` structure which wants to init.
 *
 * @return 0 for success to init, negative value for fail to init.
 */
int docker_shm_init(struct docker_info *info)
{
        int shmid = -1, semid = -1;
        int ret = 0;

        assert(NULL != info);
        assert(0 != info->pid);

        if (0 > (semid = __docker_sem_init(info))) {
                pr_info(ERROR,
                        "Semaphore initialization fail. (target pid :%d)\n",
                        info->pid);
                ret = semid;
                goto exit;
        }
        pr_info(INFO, "Semaphore create success. (path: /tmp/%s%s_%d)\n",
                info->cgroup_id, SEM_KEY_PATHNAME, info->pid);

        if (0 > (shmid = __docker_shm_init(info))) {
                pr_info(ERROR,
                        "Shared Memory initialization fail. (target pid :%d)\n",
                        info->pid);
                ret = shmid;
                goto exit;
        }
        pr_info(INFO, "Shared Memory create success. (path: /tmp/%s%s_%d)\n",
                info->cgroup_id, SHM_KEY_PATHNAME, info->pid);

        info->semid = semid;
        info->shmid = shmid;

exit:
        return ret;
}

/**
 * @brief Retrieve the data from Shared Memory.
 *
 * @param[in] info `docker_info` structure which wants to get data.
 * @param[out] buffer Destination of data will be stored
 *
 * @return 0 for success to init, negative value for fail to init.
 */
int docker_shm_get(const struct docker_info *info, void *buffer)
{
        struct total_results *shm;

        assert(NULL != buffer);
        assert(NULL != info);
        assert(-1 != info->shmid);

        docker_sem_wait(info);
        shm = (struct total_results *)shmat(info->shmid, NULL, 0);
        if ((size_t)(-1) == (size_t)shm) {
                pr_info(ERROR, "Cannot get shared memory (errno: %p)\n", shm);
                return -EFAULT;
        }
        memcpy(buffer, shm, sizeof(struct total_results));
        docker_sem_post(info);
        return 0;
}

/**
 * @brief Deallocate the Shared Memory resources.
 *
 * @param[in] info `docker_info` structure which wants to deallocate.
 * @param[in] flags Set a range of deallocation.
 */
void docker_shm_free(struct docker_info *info, int flags)
{
        int semid;
        int shmid;

        assert(NULL != info);

        semid = info->semid;
        shmid = info->shmid;

        union {
                int val;
        } sem_attr;

        sem_attr.val = 0;

        if ((DOCKER_IPC_FREE & flags) && 0 <= semid) {
                semctl(semid, 0, IPC_RMID, sem_attr);
        }

        if ((DOCKER_IPC_FREE & flags) && 0 <= shmid) {
                shmctl(shmid, IPC_RMID, NULL);
        }
        info->shmid = info->semid = -1;
}
