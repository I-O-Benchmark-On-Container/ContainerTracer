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
 * @brief Implementation of run the `trace-replay` benchmark with Docker.
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
 * @brief Associated table of I/O scheduler.
 * @warning Kyber sceduler doesn't support in SCSI devices.
 */
static const char *docker_valid_scheduler[] = {
        [DOCKER_NONE_SCHEDULER] = "none",
        [DOCKER_KYBER_SCHEDULER] = "kyber",
        [DOCKER_BFQ_SCHEDULER] = "bfq",
        NULL,
};

static struct docker_info *global_info_head =
        NULL; /**< global `docker_info` list */

/**
 * @brief Remove the docker container.
 *
 * @param[in] info Docker information that wants to remove.
 */
static void __docker_rm_container(struct docker_info *info)
{
        char cmd[1000];

        sprintf(cmd, "docker rm -f %s > /dev/null 2>&1",
                info->cgroup_id); /* Ignore the Error */

        if (system(cmd) == -1) {
                pr_info(ERROR, "Cannot execute command: %s\n", cmd);
        }

        sprintf(cmd, "rm -rf /tmp/%s", info->cgroup_id);

        if (system(cmd) == -1) {
                pr_info(ERROR, "Cannot execute command: %s\n", cmd);
        }
}

/**
 * @brief Deallocate this driver's resources.
 * @note http://www.ascii-art.de/ascii/def/dr_who.txt
 */
static void __docker_free(void)
{
        if (system("docker rmi -f suhoson/trace_replay:local")) {
                pr_info(INFO, "Fail to remove local image: not error\n");
        }

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

                        /* Remove the IPC object. */
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
 * @brief Check the current inputted scheduler text can be supported by the driver.
 *
 * @param[in] scheduler Inputted scheduler string.
 *
 * @return 0 for mean support the scheduler, -EINVAL for mean don't support the scheduler.
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
 * @brief Initialize the global configuration and per processes configuration.
 *
 * @param[in] object global `runner_config` pointer.
 *
 * @return 0 for success to init, error number for fail to init.
 * @warning Do not run this function in child process.
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

        char *cmd = NULL;

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
                if (!current) { /* Fail to allocate */
                        ret = -ENOMEM;
                        goto exception;
                }
                current->global_config = object;
                if (!current->global_config) {
                        ret = -EFAULT;
                        goto exception;
                }

                if (global_info_head == NULL) { /* Initialize sequence */
                        prev = global_info_head = current;
                } else {
                        prev->next = current;
                        prev = current;
                }
        }

        ret = system(
                "docker pull suhoson/trace_replay:latest > /dev/null 2>&1");
        if (ret) {
                pr_info(ERROR,
                        "Cannot pull image: suhoson/trace_replay:latest\n");
                goto exception;
        }

        ret = system("docker rm -f new_trace_replay");
        if (ret) {
                pr_info(INFO, "Cannot remove new_trace_replay: not error\n");
        }
        ret = system("docker rmi -f suhoson/trace_replay:local");
        if (ret) {
                pr_info(INFO,
                        "Cannot remove trace_replay local version image: not error\n");
        }

        ret = system(
                "docker run --name new_trace_replay -d suhoson/trace_replay tail -f /dev/null");
        if (ret) {
                pr_info(ERROR, "Cannot run new_trace_replay\n");
                goto exception;
        }

        cmd = (char *)malloc(PAGE_SIZE * PAGE_SIZE);
        if (!cmd) {
                ret = -ENOMEM;
                goto exception;
        }

        sprintf(cmd, "docker cp %s new_trace_replay:/usr/local/bin/",
                global_info_head->trace_replay_path);
        ret = system(cmd);
        if (ret) {
                pr_info(ERROR, "Cannot copy trace_replay binary.\n");
                goto exception;
        }

        ret = system(
                "docker commit new_trace_replay suhoson/trace_replay:local");
        if (ret) {
                pr_info(ERROR, "Cannot commit new image.\n");
                goto exception;
        }

        ret = system("docker rm -f new_trace_replay");

        free(cmd);

        return ret;
exception:
        __docker_free();
        if (cmd != NULL) {
                free(cmd);
        }
        return ret;
}

/**
 * @brief Set the child process to specific control group(cgroup)
 *
 * @param[in] current The structure which has the current process information.
 *
 * @return 0 for success to set, errno for fail to set.
 */
static int docker_set_cgroup_state(struct docker_info *current)
{
        int ret = 0;

        ret = strcmp(current->scheduler,
                     docker_valid_scheduler[DOCKER_BFQ_SCHEDULER]);
        if (ret == 0) { /* Set the weight when BFQ scheduler. */
                char cmd[PATH_MAX];

                if (!runner_is_valid_bfq_weight(current->weight)) {
                        pr_info(ERROR, "BFQ weight is out of range: \"%u\"\n",
                                current->weight);
                        return -EINVAL;
                }

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
 * @brief Each process `trace-replay` execute part. 
 *
 * @param[in] current The structure which has the current process information.
 *
 * @return 0 for success to create, negative value for fail to create.
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

        /* Create the docker container */
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

        if (DOCKER_SYNTH != docker_is_synth_type(current->trace_data_path)) {
                sprintf(cmd, "docker cp %s %s:%s", current->trace_data_path,
                        current->cgroup_id, current->trace_data_path);

                ret = system(cmd);
                if (ret)
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
 * @brief Run all processes' `trace-replay` part.
 *
 * @return 0 for success to run, error number for fail to run.
 */
int docker_runner(void)
{
        int ret = 0;
        struct docker_info *current = global_info_head;
        char cmd[PATH_MAX];

        docker_info_list_traverse(current, global_info_head)
        {
                /* Remove the existing container */
                __docker_rm_container(current);
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
                current->pid = 1; /* Container's PID */

                /* Prepare the `tmp` directory for store the IPC key. */
                sprintf(cmd, "mkdir -p /tmp/%s/tmp", current->cgroup_id);
                if (0 != (ret = system(cmd))) {
                        pr_info(ERROR, "Cannot make directory: %s\n", cmd);
                        return ret;
                }

                /* Create the container. */
                if (0 != (ret = docker_create_container(current))) {
                        pr_info(ERROR, "Cannot execute program (errno: %d)\n",
                                ret);
                        __docker_rm_container(current);
                        return ret;
                }

                /* Create the IPC object. */
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
                sprintf(cmd, "docker start %s > /dev/null", current->cgroup_id);
                if (0 != (ret = system(cmd))) {
                        pr_info(ERROR, "Cannot start container: %s\n",
                                current->cgroup_id);
                        __docker_rm_container(current);
                        docker_shm_free(current, DOCKER_IPC_FREE);
                        docker_mq_free(current, DOCKER_IPC_FREE);
                }

                /* Set cgroup weight and execute. */
                docker_set_cgroup_state(current);
        }

        return ret;
}

/**
 * @brief Get execution-time results from `trace-replay`.
 *
 * @param[in] key `cgroup_id` value which specifies the location of data I want to get.
 * @param[out] buffer Data contains the execution-time results based on `key`.
 *
 * @return `log.type` for success to get information, negative value for fail to get information.
 * @warning `buffer` must be allocated memory over and equal `INTERVAL_RESULT_STRING_SIZE`
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
 * @brief Get end-time results from `trace-replay`.
 *
 * @param[in] key `cgroup_id` value which specifies the location of data I want to get.
 * @param[out] buffer Data contains the end-time results based on `key`.
 *
 * @return 0 for success to get information, negative value for fail to get information.
 * @warning `buffer` must be allocated memory over and equal `TOTAL_RESULT_STRING_SIZE`
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
 * @brief Deallocate resources of this driver.
 */
void docker_free(void)
{
        __docker_free();
        hdestroy();
}
