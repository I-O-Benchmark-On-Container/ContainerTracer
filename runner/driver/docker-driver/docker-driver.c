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
 * @file docker-driver.c
 * @brief trace-replay를 동작시키는 driver 구현부에 해당합니다.
 * @author SuhoSon (ngeol564@gmail.com)
 * @version 0.1
 * @date 2020-08-19
 */

#include <stdlib.h>
#include <errno.h>
#include <search.h>
#include <assert.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/syscall.h>

#include <json.h>
#include <jemalloc/jemalloc.h>

#include <generic.h>
#include <runner.h>
#include <driver/docker-driver.h>
#include <log.h>
#include <trace_replay.h>

enum { DOCKER_NONE_SCHEDULER = 0,
       DOCKER_KYBER_SCHEDULER,
       DOCKER_BFQ_SCHEDULER,
};

/**
 * @brief 현재 tr-driver에서 지원하는 I/O 스케줄러가 들어가게 됩니다.
 * @warning kyber는 SCSI를 지원하지 않음을 유의해주시길 바랍니다.
 */
static const char *docker_valid_scheduler[] = {
        [DOCKER_NONE_SCHEDULER] = "none",
        [DOCKER_KYBER_SCHEDULER] = "kyber",
        [DOCKER_BFQ_SCHEDULER] = "bfq",
        NULL,
};

static struct docker_info *global_info_head =
        NULL; /**< trace-replay의 각각을 실행시킬 때 필요한 정보를 담고있는 구조체 리스트의 헤드입니다. */

static void __docker_rm_container(struct docker_info *info)
{
        char cmd[1000];

        sprintf(cmd, "docker rm -f %s > /dev/null 2>&1",
                info->cgroup_id); /* Error는 무시 */

        if (system(cmd) ==
            -1) { /* Container가 존재하지 않을 경우 0이 아닌 값이 리턴 됨 */
                pr_info(ERROR, "Cannot execute command: %s\n", cmd);
        }

        sprintf(cmd, "rm -rf /tmp/%s", info->cgroup_id);

        if (system(cmd) ==
            -1) { /* Container가 존재하지 않을 경우 0이 아닌 값이 리턴 됨 */
                pr_info(ERROR, "Cannot execute command: %s\n", cmd);
        }
}

/**
 * @brief 실질적으로 trace-replay 관련 구조체의 동적 할당된 내용을 해제하는 부분에 해당합니다.
 * @note http://www.ascii-art.de/ascii/def/dr_who.txt
 */
static void __docker_free(void)
{
        while (global_info_head != NULL) {
                struct docker_info *current = global_info_head;
                struct docker_info *next = global_info_head->next;

                if (getpid() == current->ppid && 0 != current->pid) {
                        /********************************************
                                                 Exterminate!
                                                /
                                           ___
                                   D>=G==='   '.
                                         |======|
                                         |======|
                                     )--/]IIIIII]
                                        |_______|
                                        C O O O D
                                       C O  O  O D
                                      C  O  O  O  D
                                      C__O__O__O__D
                                     [_____________]
                         ********************************************/

                        __docker_rm_container(current);

                        /* IPC 객체를 삭제합니다. */
                        docker_shm_free(current, DOCKER_IPC_FREE);
                        docker_mq_free(current, DOCKER_IPC_FREE);

                        pr_info(INFO, "Delete target %p\n", current);
                        free(current);

                        global_info_head = next;
                }
                pr_info(INFO, "Do trace-replay free success ==> %p\n", current);
        }
}

/**
 * @brief 현재 입력되는 scheduler가 driver가 지원하는 지를 확인하도록 합니다.
 *
 * @param scheduler 검사하고자 하는 스케쥴러 문자열을 가진 문자열 포인터입니다.
 *
 * @return 가지고 있는 경우 0이 반환되고, 그렇지 않은 경우 -EINVAL이 반환됩니다.
 */
int docker_valid_scheduler_test(const char *scheduler)
{
        const char *index = docker_valid_scheduler[0];
        while (index != NULL) {
                if (strcmp(scheduler, index) == 0) {
                        return 0;
                }
                index++;
        }
        return -EINVAL;
}

/**
 * @brief 전역 옵션을 설정한 후에 각각의 프로세스 별로 할당할 옵션을 설정하도록 합니다.
 *
 * @param object 전역 runner_config의 포인터가 들어가야 합니다.
 *
 * @return 정상적으로 초기화가 된 경우 0을 그렇지 않은 경우 적절한 오류 번호를 반환합니다. 
 * @warning 자식 프로세스에서 이 함수가 절대로 실행되서는 안됩니다.
 */
int docker_init(void *object)
{
        struct runner_config *config = (struct runner_config *)object;
        struct generic_driver_op *op = &config->op;
        struct json_object *setting = config->setting;
        struct json_object *tmp = NULL;
        struct docker_info *current = NULL, *prev = NULL;

        int ret = 0, i = 0;
        int nr_tasks = -1;

        op->runner = docker_runner;
        op->get_interval = docker_get_interval;
        op->get_total = docker_get_total;
        op->free = docker_free;

        assert(NULL != op->runner);
        assert(NULL != op->get_interval);
        assert(NULL != op->get_total);
        assert(NULL != op->free);

        if (getuid() != 0) {
                pr_info(ERROR,
                        "You have to run this program with superuser privilege\n");
                return -EINVAL;
        }

        if (!json_object_object_get_ex(setting, "trace_replay_path", &tmp)) {
                pr_info(ERROR, "Not exist error (key: %s)\n",
                        "trace_replay_path");
                ret = -EACCES;
                goto exception;
        }

        if (!json_object_object_get_ex(setting, "nr_tasks", &tmp)) {
                pr_info(ERROR, "Not exist error (key: %s)\n", "nr_tasks");
                ret = -EACCES;
                goto exception;
        }
        nr_tasks = json_object_get_int(tmp);
        pr_info(INFO, "Generate %d tasks\n", nr_tasks);

        if (!hcreate(nr_tasks + 1)) {
                pr_info(ERROR, "hash search table create failed. (size: %d)\n",
                        nr_tasks + 1);
                ret = -ENOMEM;
                goto exception;
        }

        for (i = 0; i < nr_tasks; i++) {
                current = docker_info_init(setting, i);
                if (!current) { /* 할당 실패가 벌어진 경우 */
                        ret = -ENOMEM;
                        goto exception;
                }
                current->global_config = object;
                if (!current->global_config) {
                        ret = -EFAULT;
                        goto exception;
                }

                if (global_info_head == NULL) { /* 초기화 과정 */
                        prev = global_info_head = current;
                } else {
                        prev->next = current;
                        prev = current;
                }
        }

        /* Trace replay가 포함된 ubuntu 16.04 이미지 다운로드 */
        ret = system(
                "docker pull suhoson/trace_replay:latest > /dev/null 2>&1");
        if (ret) {
                pr_info(ERROR,
                        "Cannot pull image: suhoson/trace_replay:latest\n");
                goto exception;
        }

        return ret;
exception:
        __docker_free();
        return ret;
}

/**
 * @brief control group에 자식 프로세스가 작동할 수 있도록 설정합니다.
 *
 * @param current 현재 프로세스의 정보를 가진 구조체를 가리킵니다.
 *
 * @return 정상 종료한 경우에는 0 그 이외에는 적절한 errno이 반환됩니다.
 */
static int docker_set_cgroup_state(struct docker_info *current)
{
        int ret = 0;

        ret = strcmp(current->scheduler,
                     docker_valid_scheduler[DOCKER_BFQ_SCHEDULER]);
        if (ret == 0) { /* BFQ 스케쥴러이면 weight를 설정해줍니다. */
                char cmd[PATH_MAX];

                snprintf(
                        cmd, PATH_MAX,
                        "echo %d > /sys/fs/cgroup/blkio/docker/%s/blkio.%s.weight",
                        current->weight, current->container_id,
                        current->scheduler);
                pr_info(INFO, "Do command: \"%s\"\n", cmd);
                ret = system(cmd);
                if (ret) {
                        pr_info(ERROR, "Cannot set weight: \"%s\"\n", cmd);
                        return ret;
                }
        }

        pr_info(INFO, "CGROUP READY (CONTAINER ID: %s)\n",
                current->container_id);

        return 0;
}

/**
 * @brief 자식 프로세스에 trace-replay를 올려서 동작시키는 함수입니다.
 *
 * @param current 자식 프로세스에 돌릴 trace-replay 설정에 해당합니다.
 *
 * @return 성공한 경우에는 0, 그렇지 않은 경우 음수 값이 들어갑니다.
 */
static int docker_create_container(struct docker_info *current)
{
        FILE *fp = NULL;
        char filename[PATH_MAX];
        char *cmd = NULL;
        int ret = 0;

        cmd = (char *)malloc(PAGE_SIZE * PAGE_SIZE);
        if (!cmd) {
                ret = -ENOMEM;
                goto exception;
        }

        /* Docker container 생성 */
        snprintf(filename, sizeof(filename), "%s_%u_%s.txt", current->scheduler,
                 current->weight, current->cgroup_id);
        sprintf(cmd,
                "docker container create --name %s --ipc=host -v /tmp/%s/tmp:/tmp --device /dev/%s suhoson/trace_replay:latest /usr/local/bin/trace-replay %u %u %s %u %u /dev/%s %s %u %u %u",
                current->cgroup_id, current->cgroup_id, current->device,
                current->q_depth, current->nr_thread, filename, current->time,
                current->trace_repeat, current->device,
                current->trace_data_path, current->wss, current->utilization,
                current->iosize);

        fp = popen(cmd, "r");
        if (fp == NULL) {
                pr_info(ERROR, "Getting container id failed (name: %s)\n",
                        current->cgroup_id);
                ret = -EFAULT;
                goto exception;
        }

        if (fgets(current->container_id, DOCKER_ID_LEN, fp) == NULL) {
                ret = -EFAULT;
                goto exception;
        }

        pclose(fp);
        free(cmd);

        return ret;
exception:
        if (fp != NULL) {
                pclose(fp);
        }

        if (cmd != NULL) {
                free(cmd);
        }
        return ret;
}

/**
 * @brief trace-replay를 실행시키도록 합니다.
 *
 * @return 정상적으로 동작이 된 경우 0을 그렇지 않은 경우 적절한 오류 번호를 반환합니다. 
 */
int docker_runner(void)
{
        int ret = 0;
        struct docker_info *current = global_info_head;
        char cmd[PATH_MAX];

        docker_info_list_traverse(current, global_info_head)
        {
                // 기존의 container를 지우도록 합니다.
                __docker_rm_container(current);
        }

        docker_info_list_traverse(current, global_info_head)
        {
                int ignore_ret = 0;
                // 기존의 container를 지우도록 합니다.
                sprintf(cmd, "docker rm -f %s", current->cgroup_id);
                ignore_ret = system(cmd);

                // 기존의 디렉터리를 삭제하도록 합니다.
                sprintf(cmd, "rm -rf /tmp/%s", current->cgroup_id);
                ignore_ret = system(cmd);
        }

        snprintf(cmd, PATH_MAX, "echo %s >> /sys/block/%s/queue/scheduler",
                 global_info_head->scheduler, global_info_head->device);
        pr_info(INFO, "Do command: \"%s\"\n", cmd);
        ret = system(cmd);
        if (ret) {
                pr_info(ERROR, "Scheduler setting failed (scheduler: %s)\n",
                        current->scheduler);
                return ret;
        }

        docker_info_list_traverse(current, global_info_head)
        {
                current->pid = 1; /* Container 안에서 pid입니다. */

                /* IPC 통신 key값 저장을 위한 tmp directory를 생성합니다. */
                sprintf(cmd, "mkdir -p /tmp/%s/tmp", current->cgroup_id);
                if (0 != (ret = system(cmd))) {
                        pr_info(ERROR, "Cannot make directory: %s\n", cmd);
                        return ret;
                }

                /* Container를 생성합니다. */
                if (0 != (ret = docker_create_container(current))) {
                        pr_info(ERROR, "Cannot execute program (errno: %d)\n",
                                ret);
                        __docker_rm_container(current);
                        return ret;
                }

                /* IPC 객체를 생성합니다. */
                if (0 > (ret = docker_shm_init(current))) {
                        pr_info(ERROR,
                                "Shared memory init failed.(errno: %d)\n", ret);
                        __docker_rm_container(current);
                        docker_shm_free(current, DOCKER_IPC_FREE);
                        return ret;
                }
                if (0 > (ret = docker_mq_init(current))) {
                        pr_info(ERROR,
                                "Message Queue init failed.(errno: %d)\n", ret);
                        __docker_rm_container(current);
                        docker_shm_free(current, DOCKER_IPC_FREE);
                        docker_mq_free(current, DOCKER_IPC_FREE);
                        return ret;
                }
        }

        docker_info_list_traverse(current, global_info_head)
        {
                /* cgroup weight를 설정하고 실행합니다. */
                docker_set_cgroup_state(current);

                sprintf(cmd, "docker start %s > /dev/null", current->cgroup_id);
                if (0 != (ret = system(cmd))) {
                        pr_info(ERROR, "Cannot start container: %s\n",
                                current->cgroup_id);
                        __docker_rm_container(current);
                        docker_shm_free(current, DOCKER_IPC_FREE);
                        docker_mq_free(current, DOCKER_IPC_FREE);
                }
        }

        return ret;
}

/**
 * @brief trace-replay가 실행 중일 때의 정보를 반환합니다.
 *
 * @param key 임의의 cgroup_id에 해당합니다.
 * @param buffer 값이 반환되는 위치에 해당합니다.
 *
 * @return 정상적으로 종료되는 경우에는 log.type 정보가 반환되고, 그렇지 않은 경우 적절한 음수 값이 반환됩니다.
 * @note buffer를 재활용을 많이 하기 때문에 buffer의 내용은 반드시 미리 runner에서 할당이 되어 있어야 합니다.
 */
int docker_get_interval(const char *key, char *buffer)
{
        ENTRY query = { .key = NULL, .data = NULL };
        ENTRY *result = NULL;

        struct docker_info *info = NULL;
        struct realtime_log log = { 0 };

        int ret = 0;

        snprintf(buffer, INTERVAL_RESULT_STRING_SIZE, "%s", key);

        query.key = buffer;
        if (NULL == (result = hsearch(query, FIND))) {
                pr_info(ERROR, "Cannot find item (key: %s)\n", buffer);
                return -EINVAL;
        }
        info = (struct docker_info *)result->data;
        if (NULL != info) {
                if (0 > (ret = docker_mq_get(info, (void *)&log))) {
                        return ret;
                }
                docker_realtime_serializer(info, &log, buffer);
        } else {
                pr_info(ERROR, "`info` doesn't exist: %p\n", info);
                return -EACCES;
        }

        ret = log.type;
        return ret;
}

/**
 * @brief trace-replay가 동작 완료한 후의 정보를 반환합니다.
 *
 * @param key 임의의 cgroup_id에 해당합니다.
 * @param buffer 값이 반환되는 위치에 해당합니다.
 *
 * @return 정상적으로 동작이 된 경우 0을 그렇지 않은 경우 적절한 오류 번호를 반환합니다. 
 */
int docker_get_total(const char *key, char *buffer)
{
        ENTRY query = { .key = NULL, .data = NULL };
        ENTRY *result = NULL;

        struct docker_info *info = NULL;
        struct total_results results = { 0 };

        int ret = 0;

        snprintf(buffer, TOTAL_RESULT_STRING_SIZE, "%s", key);

        query.key = buffer;
        if (NULL == (result = hsearch(query, FIND))) {
                pr_info(ERROR, "Cannot find item (key: %s)\n", buffer);
                return -EINVAL;
        }
        info = (struct docker_info *)result->data;
        if (NULL != info) {
                if (0 > (ret = docker_shm_get(info, (void *)&results))) {
                        return ret;
                }
                docker_total_serializer(info, &results, buffer);
        } else {
                pr_info(ERROR, "`info` doesn't exist: %p\n", info);
                return -EACCES;
        }

        return ret;
}

/**
 * @brief trace-replay 관련 동적 할당 정보를 해제합니다.
 */
void docker_free(void)
{
        __docker_free();
        hdestroy();
}
