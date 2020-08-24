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
 * @brief Shared Memory를 생성 및 사용하는 방식이 구현되어 있습니다.
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
 * @brief 세마포어만을 초기화하는 함수입니다.
 *
 * @param info 세마포어 획득 대상 정보를 가진 구조체 포인터입니다.
 *
 * @return 성공적으로 종료된 경우에는 세마포어의 ID가 반환되고, 그렇지 않은 경우에는 음수 값이 반환됩니다.
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
 * @brief Shared Memory만을 초기화하는 함수입니다.
 *
 * @param info 초기화를 진행하고자 하는 대상을 가리키는 구조체의 포인터입니다.
 *
 * @return 성공적으로 종료된 경우에는 Shared Memory의 ID가 반환되고, 그렇지 않은 경우에는 음수 값이 반환됩니다.
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

        /* 파일이 존재하지 않는 경우에 파일을 생성합니다. */
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
 * @brief 세마포어 락을 획득합니다.
 *
 * @param info 세마포어 획득 대상 정보를 가진 구조체 포인터입니다.
 */
static void docker_sem_wait(const struct docker_info *info)
{
        struct sembuf sop = {
                .sem_num = 0,
                .sem_op = -1,
                .sem_flg = SEM_UNDO,
        };

        pr_info(INFO, "Going to sleep (pid: %d)\n", info->pid);
        /* 1은 연산 갯수를 지칭합니다. */
        assert(-1 != semop(info->semid, &sop, 1));
}

/**
 * @brief 세마포어 락을 반환합니다.
 *
 * @param info 세마포어 반환 대상 정보를 가진 구조체 포인터입니다.
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
 * @brief Shared Memory와 관련된 설정에 대해서 초기화를 진행하도록 합니다.
 *
 * @param info 초기화를 진행하고자 하는 대상을 가리키는 구조체의 포인터입니다.
 *
 * @return 정상 종료가 된 경우에는 0이 반환되고, 그렇지 않은 경우에는 음수 값이 반환됩니다.
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
 * @brief Shared Memory로 부터 데이터를 가져오는 함수입니다.
 *
 * @param info 데이터를 가져와야 하는 곳을 가리키는 정보를 가진 구조체의 포인터에 해당합니다.
 * @param buffer 데이터가 실제 저장되는 버퍼를 가리킵니다.
 *
 * @return 정상 종료가 된 경우에는 0, 그렇지 않은 경우에는 음수 값이 반환됩니다.
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
 * @brief Shared Memory의 할당된 내용을 해제하도록 합니다.
 *
 * @param info 할당 해제를 진행할 docker_info에 해당합니다.
 * @param flags 해제의 정도를 설정하는 flag에 해당합니다.
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
