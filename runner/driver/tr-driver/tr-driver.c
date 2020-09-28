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
 * @file tr-driver.c
 * @brief Implementation of run the `trace-replay` benchmark.
 * @author BlaCkinkGJ (ss5kijun@gmail.com)
 * @version 0.1.2
 * @date 2020-08-05
 */

#include <stdlib.h>
#include <errno.h>
#include <search.h>
#include <assert.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/syscall.h>

#include <json.h>
#include <jemalloc/jemalloc.h>

#include <generic.h>
#include <runner.h>
#include <driver/tr-driver.h>
#include <log.h>
#include <sync.h>
#include <trace_replay.h>

enum { TR_NONE_SCHEDULER = 0,
       TR_KYBER_SCHEDULER,
       TR_BFQ_SCHEDULER,
};

/**
 * @brief Associated table of I/O scheduler.
 * @warning Kyber sceduler doesn't support in SCSI devices.
 */
static const char *tr_valid_scheduler[] = {
        [TR_NONE_SCHEDULER] = "none",
        [TR_KYBER_SCHEDULER] = "kyber",
        [TR_BFQ_SCHEDULER] = "bfq",
        NULL,
};

static const int tr_weight_support_scheduler[] = { TR_BFQ_SCHEDULER };

static struct tr_info *global_info_head = NULL; /**< global `tr_info` list */

/**
 * @brief Capture the SIGTERM and deallocate all resources which this process has.
 *
 * @param[in] signum Current captured signal.
 * @note SIGKILL isn't captured.
	 */
static void tr_kill_handle(int signum)
{
        struct tr_info *current = global_info_head;

        (void)signum;
        pr_info(INFO, "TERMINATE SEQUENCE DETECTED(signum: %d, pid: %d)\n",
                signum, getpid());
        if (current) {
                assert(NULL != current->global_config);
                runner_config_free(current->global_config, RUNNER_FREE_ALL);
        }
        _exit(EXIT_SUCCESS);
}

/**
 * @brief Deallocate this driver's resources.
 * @note http://www.ascii-art.de/ascii/def/dr_who.txt
 */
static void __tr_free(void)
{
        while (global_info_head != NULL) {
                struct tr_info *current = global_info_head;
                struct tr_info *next = global_info_head->next;

                if (getpid() == current->ppid &&
                    0 != current->pid) { /* KILL CHILD PROCESS. */
                        int status = 0;
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
                        kill(current->pid, SIGTERM);
                        if (waitpid(current->pid, &status, 0) < 0) {
                                pr_info(WARNING,
                                        "waitpid error (pid: %d, status: 0x%X)\n",
                                        current->pid, status);
                                _exit(EXIT_FAILURE);
                        }
                        /* Remove the IPC object. */
                        tr_shm_free(current, TR_IPC_FREE);
                        tr_mq_free(current, TR_IPC_FREE);
                }

                pr_info(INFO, "Delete target %p\n", current);
                free(current);

                global_info_head = next;
        }
        pr_info(INFO, "Do trace-replay free success ==> %p\n",
                global_info_head);
}

/**
 * @brief Check the current inputted scheduler text can be supported by the driver.
 *
 * @param[in] scheduler Inputted scheduler string.
 *
 * @return 0 and positive integer for mean support the scheduler, -EINVAL for mean don't support the scheduler.
 */
int tr_valid_scheduler_test(const char *scheduler)
{
        int index = 0;
        const char *current = NULL;
        while (NULL != (current = tr_valid_scheduler[index++])) {
                if (0 == strcmp(scheduler, current)) {
                        return index - 1;
                }
        }
        return -EINVAL;
}

/**
 * @brief Check the parameter's `scheduler_index` supports weight.
 *
 * @param[in] scheduler_index The scheduler's index which is based on `tr_valid_scheduler` and wants to check.
 *
 * @return 0 for doesn't support the weight, 1 for supporting the weight
 */
int tr_has_weight_scheduler(const int scheduler_index)
{
        const int nr_index = sizeof(tr_weight_support_scheduler) / sizeof(int);
        int index = 0;
        for (index = 0; index < nr_index; index++) {
                if (scheduler_index == tr_weight_support_scheduler[index]) {
                        return 1;
                }
        }
        return 0;
}

/**
 * @brief Initialize the global configuration and per processes configuration.
 *
 * @param[in] object global `runner_config` pointer.
 *
 * @return 0 for success to init, error number for fail to init.
 * @warning Do not run this function in child process.
 */
int tr_init(void *object)
{
        struct runner_config *config = (struct runner_config *)object;
        struct generic_driver_op *op = &config->op;
        struct json_object *setting = config->setting;
        struct json_object *tmp = NULL;
        struct tr_info *current = NULL, *prev = NULL;

        int ret = 0, i = 0;
        int nr_tasks = -1;

        op->runner = tr_runner;
        op->get_interval = tr_get_interval;
        op->get_total = tr_get_total;
        op->free = tr_free;

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
                current = tr_info_init(setting, i);
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

        return ret;
exception:
        __tr_free();
        return ret;
}

/**
 * @brief Set the child process to specific control group(cgroup)
 *
 * @param[in] current The structure which has the current process information.
 *
 * @return 0 for success to set, errno for fail to set.
 */
static int tr_set_cgroup_state(struct tr_info *current)
{
        int ret = 0;
        char cmd[PATH_MAX];

        /* Generate cgroup seqeunce. */
        snprintf(cmd, PATH_MAX, "mkdir " TR_CGROUP_PREFIX "%s%d",
                 current->prefix_cgroup_name, current->pid);
        pr_info(INFO, "Do command: \"%s\"\n", cmd);
        ret = system(cmd);
        if (ret) {
                pr_info(ERROR, "Cannot create the directory: \"%s\"\n", cmd);
                return -EINVAL;
        }

        ret = tr_valid_scheduler_test(current->scheduler);
        if (0 > ret) {
                pr_info(ERROR, " Cannot support the scheduler: \"%s\"\n",
                        current->scheduler);
                return ret;
        }
        ret = tr_has_weight_scheduler(ret);
        if (ret) { /* Set the weight when BFQ scheduler. */
                if (!runner_is_valid_bfq_weight(current->weight)) {
                        pr_info(ERROR, "BFQ weight is out of range: \"%u\"\n",
                                current->weight);
                        return -EINVAL;
                }
                snprintf(cmd, PATH_MAX,
                         "echo %d > " TR_CGROUP_PREFIX
                         "%s%d/" TR_CGROUP_WEIGHT_PREFIX ".%s.weight",
                         current->weight, current->prefix_cgroup_name,
                         current->pid, current->scheduler);
                pr_info(INFO, "Do command: \"%s\"\n", cmd);
                ret = system(cmd);
                if (ret) {
                        pr_info(ERROR, "Cannot set weight: \"%s\"\n", cmd);
                        return -EINVAL;
                }
        }

        snprintf(cmd, PATH_MAX,
                 "echo %d > " TR_CGROUP_PREFIX "%s%d/" TR_CGROUP_SET_PID,
                 current->pid, current->prefix_cgroup_name, current->pid);
        pr_info(INFO, "Do command: \"%s\"\n", cmd);
        ret = system(cmd);
        if (ret) {
                pr_info(ERROR, "Cannot hang (pid: %d) to control group: %s\n",
                        current->pid, cmd);
                return -EINVAL;
        }

        pr_info(INFO, "CGROUP READY (PID: %d)\n", current->pid);

        return 0;
}

/**
 * @brief Each process `trace-replay` execute part. 
 *
 * @param[in] current The structure which has the current process information.
 *
 * @return 0 for success to execute, negative value for fail to create.
 */
static int tr_do_exec(struct tr_info *current)
{
        struct tr_info info;
        char filename[PATH_MAX];

        char q_depth_str[PAGE_SIZE / 4];
        char nr_thread_str[PAGE_SIZE / 4];
        char time_str[PAGE_SIZE / 4];
        char device_path[PAGE_SIZE / 4];

        char trace_repeat_str[PAGE_SIZE / 4];
        char wss_str[PAGE_SIZE / 4];
        char utilization_str[PAGE_SIZE / 4];
        char iosize_str[PAGE_SIZE / 4];

        assert(NULL != current);
        if (current) {
                memcpy(&info, current, sizeof(struct tr_info));
                assert(NULL != current->global_config);
                runner_config_free(current->global_config, RUNNER_FREE_ALL);
        }
        snprintf(filename, sizeof(filename), "%s_%d_%d_%s.txt", info.scheduler,
                 info.ppid, info.weight, info.cgroup_id);
        snprintf(q_depth_str, sizeof(q_depth_str), "%u", info.q_depth);
        snprintf(nr_thread_str, sizeof(nr_thread_str), "%u", info.nr_thread);
        snprintf(time_str, sizeof(time_str), "%u", info.time);
        snprintf(device_path, sizeof(device_path), "/dev/%s", info.device);

        snprintf(trace_repeat_str, sizeof(trace_repeat_str), "%u",
                 info.trace_repeat);
        snprintf(wss_str, sizeof(wss_str), "%u", info.wss);
        snprintf(utilization_str, sizeof(utilization_str), "%u",
                 info.utilization);
        snprintf(iosize_str, sizeof(iosize_str), "%u", info.iosize);

        pr_info(INFO, "trace replay save location: \"%s\"\n", filename);
        WAIT_PARENT();
#ifdef DEBUG
        tr_print_info(&info);
#endif
        return execlp(info.trace_replay_path, info.trace_replay_path,
                      q_depth_str, nr_thread_str, filename, time_str,
                      trace_repeat_str, device_path, info.trace_data_path,
                      wss_str, utilization_str, iosize_str, (char *)0);
}

/**
 * @brief Run all processes' `trace-replay` part.
 *
 * @return 0 for success to run, error number for fail to run.
 */
int tr_runner(void)
{
        int ret = 0;
        struct tr_info *current = global_info_head;
        char cmd[PATH_MAX];
        pid_t pid;

        snprintf(cmd, PATH_MAX, "rmdir " TR_CGROUP_PREFIX "%s*",
                 current->prefix_cgroup_name);
        pr_info(INFO, "Do command: \"%s\"\n", cmd);
        ret = system(cmd); /* You can ignore this return value. */
        if (ret) {
                pr_info(WARNING, "Deletion sequence ignore: \"%s\"\n", cmd);
        }

        snprintf(cmd, PATH_MAX, "echo %s >> /sys/block/%s/queue/scheduler",
                 current->scheduler, current->device);
        pr_info(INFO, "Do command: \"%s\"\n", cmd);
        ret = system(cmd);
        if (ret) {
                pr_info(ERROR, "Scheduler setting failed (scheduler: %s)\n",
                        current->scheduler);
                return -EINVAL;
        }

        TELL_WAIT(); /* Prepare to synchronization. */
        tr_info_list_traverse(current, global_info_head)
        {
                if (0 > (pid = fork())) {
                        pr_info(ERROR, "Fork failed. (pid: %d)\n", pid);
                        return -EFAULT;
                } else if (0 == pid) { /* Child process */
                        struct sigaction act;
                        memset(&act, 0, sizeof(act));
                        act.sa_handler = tr_kill_handle;
                        sigaction(SIGTERM, &act, NULL);
                        if (0 != (ret = tr_do_exec(current))) {
                                pr_info(ERROR,
                                        "Cannot execute program (errno: %d)\n",
                                        ret);
                                perror("Execution error detected");
                                tr_kill_handle(SIGKILL); /* Failt to execute. */
                                _exit(EXIT_FAILURE);
                        }
                        pr_info(WARNING,
                                "Child process reaches the restriction area. (pid: %d)\n",
                                getpid());
                        tr_kill_handle(SIGTERM);
                        _exit(EXIT_SUCCESS);
                }

                /* Parent process */
                current->pid = pid;
                tr_set_cgroup_state(current);

                /* Create the IPC object. */
                if (0 > (ret = tr_shm_init(current))) {
                        return ret;
                }
                if (0 > (ret = tr_mq_init(current))) {
                        return ret;
                }
        }

        /* Wake all children. */
        tr_info_list_traverse(current, global_info_head)
        {
                TELL_CHILD();
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
int tr_get_interval(const char *key, char *buffer)
{
        ENTRY query = { .key = NULL, .data = NULL };
        ENTRY *result = NULL;

        struct tr_info *info = NULL;
        struct realtime_log log = { 0 };

        int ret;

        snprintf(buffer, INTERVAL_RESULT_STRING_SIZE, "%s", key);

        query.key = buffer;
        if (NULL == (result = hsearch(query, FIND))) {
                pr_info(ERROR, "Cannot find item (key: %s)\n", buffer);
                return -EINVAL;
        }
        info = (struct tr_info *)result->data;
        if (NULL != info) {
                if (0 > (ret = tr_mq_get(info, (void *)&log))) {
                        return ret;
                }
                tr_realtime_serializer(info, &log, buffer);
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
int tr_get_total(const char *key, char *buffer)
{
        ENTRY query = { .key = NULL, .data = NULL };
        ENTRY *result = NULL;

        struct tr_info *info = NULL;
        struct total_results results = { 0 };

        int ret;

        snprintf(buffer, TOTAL_RESULT_STRING_SIZE, "%s", key);

        query.key = buffer;
        if (NULL == (result = hsearch(query, FIND))) {
                pr_info(ERROR, "Cannot find item (key: %s)\n", buffer);
                return -EINVAL;
        }
        info = (struct tr_info *)result->data;
        if (NULL != info) {
                if (0 > (ret = tr_shm_get(info, (void *)&results))) {
                        return ret;
                }
                tr_total_serializer(info, &results, buffer);
        } else {
                pr_info(ERROR, "`info` doesn't exist: %p\n", info);
                return -EACCES;
        }

        return ret;
}

/**
 * @brief Deallocate resources of this driver.
 */
void tr_free(void)
{
        __tr_free();
        hdestroy();
}
