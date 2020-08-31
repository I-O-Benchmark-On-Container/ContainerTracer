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
 * @file docker-driver.h
 * @brief driver declaration part of run `docker with trace-replay`
 * @author SuhoSon (ngeol564@gmail.com)
 * @version 0.1.1
 * @date 2020-08-19
 */
#ifndef _DOCKER_DRIVER_H
#define _DOCKER_DRIVER_H

#include <linux/limits.h>
#include <unistd.h>
#include <sys/types.h>

#include <json.h>

#include <generic.h>
#include <trace_replay.h>

#define DOCKER_ID_LEN 65

/**
 * @brief Traverse the `docker_info` structrues.
 *
 * @param[out] ptr Current pointer of `tr_info` structure.
 * @param[in] head Start pointer of the traverse.
 *
 */
#define docker_info_list_traverse(ptr, head)                                   \
        for (ptr = head; ptr != NULL; ptr = ptr->next)

/**
 * @brief Traverse the `docker_json_field` structures.
 *
 * @param[out] ptr Current pointer of `docker_json_field` structures.
 * @param[in] begin Start pointer of traverse.
 * @param[in] end End pointer of the traverse.
 */
#define docker_json_field_traverse(ptr, begin, end)                            \
        for (ptr = begin; ptr != end; ptr++)

#ifdef DEBUG
#define docker_print_info(info)                                                \
        pr_info(INFO,                                                          \
                "\n"                                                           \
                "\t\t[[ current %p ]]\n"                                       \
                "\t\tpid: %d\n"                                                \
                "\t\ttime: %u\n"                                               \
                "\t\tq_depth: %u\n"                                            \
                "\t\tnr_thread: %u\n"                                          \
                "\t\tweight: %u\n"                                             \
                "\t\ttrace_repeat: %u\n"                                       \
                "\t\twss: %u\n"                                                \
                "\t\tutilization: %u\n"                                        \
                "\t\tiosize: %u\n"                                             \
                "\t\tmqid: %d\n"                                               \
                "\t\tshmid: %d\n"                                              \
                "\t\tsemid: %d\n"                                              \
                "\t\tprefix_cgroup_name: %s\n"                                 \
                "\t\tscheduler: %s\n"                                          \
                "\t\tname: %s\n"                                               \
                "\t\ttrace_replay_path: %s\n"                                  \
                "\t\ttrace_data_path: %s\n"                                    \
                "\t\tdevice: %s\n"                                             \
                "\t\tglobal_config: %p\n"                                      \
                "\t\tnext: %p\n",                                              \
                (info), (info)->pid, (info)->time, (info)->q_depth,            \
                (info)->nr_thread, (info)->weight, (info)->trace_repeat,       \
                (info)->wss, (info)->utilization, (info)->iosize,              \
                (info)->mqid, (info)->shmid, (info)->semid,                    \
                (info)->prefix_cgroup_name, (info)->scheduler,                 \
                (info)->cgroup_id, (info)->trace_replay_path,                  \
                (info)->trace_data_path, (info)->device,                       \
                (info)->global_config, (info)->next);
#endif

enum { DOCKER_IPC_NOT_FREE = 0x0, /**< Cannot remove the IPC object. */
       DOCKER_IPC_FREE = 0x1,
       /**< Remove the IPC Object */ };

enum { DOCKER_NOT_SYNTH = 0, /**< The value is not synthetic */
       DOCKER_SYNTH = 0x1,
       /**< The value is synthetic.  */ };

enum { DOCKER_ERROR_PRINT, /**< Reveal the JSON error to user. */
       DOCKER_PRINT_NONE,
       /**< Doesn't reveal the JSON error to user */ };

/**
 * @brief Support structure of the `docker_stats_serializer()` function.
 */
struct docker_json_field {
        const char *name; /**< Corresponds to the JSON key. */
        const void *member; /**< Corresponds to the JSON value. */
};

/**
 * @brief Support structure of the `total_results` structures which located in `include/trace_replay.h`.
 */
struct docker_total_json_object {
        struct json_object
                *trace[MAX_THREADS]; /**< Contain the trace's informations. */
        struct json_object *per_trace
                [MAX_THREADS]; /**< Contain the per_trace's informations. */
        struct json_object *synthetic
                [MAX_THREADS]; /**< Contain the synthetic's informations. */
        struct json_object
                *stats[MAX_THREADS]; /**< Contain the stats' informations. */
};

/**
 * @brief This structure has the major information of the each process.
 */
struct docker_info {
        pid_t ppid; /**< Contain the parent's PID. Usually, parent's process has this value to 0 */
        pid_t pid; /**< Contain the PID of the child's process which is created by parent's `fork` command. Usually, the child process has this value to 0. */

        unsigned int
                time; /**< Total execution time of benchmark program. (seconds) */
        unsigned int
                q_depth; /**< Queue depth of the benchmark program. Generally, this value is equal to the number of cores. */
        unsigned int nr_thread; /**< The number of thread per task. */

        unsigned int weight; /**< You can use only on BFQ scheduler. */

        int mqid; /**< Message Queue ID which is shared between parent and child. */
        int shmid; /**< Shared Memory ID which is shared between parent and child. */
        int semid; /**< Semaphore ID which is shared betweeen parent and child. */

        unsigned int trace_repeat; /**< The number of repeat of task. */
        unsigned int wss; /**< Working set size. */
        unsigned int utilization; /**< utilization of task. */
        unsigned int iosize; /**< I/O size of task. */

        char prefix_cgroup_name
                [NAME_MAX]; /**< Cgroup prefix which will be made inside of `/sys/fs/cgroup/blkio` */
        char device[NAME_MAX]; /**< Device name.(e.g. sda, sdb) WARNING, you must not contain the `/dev/` */
        char scheduler[NAME_MAX]; /**< Scheduler name(e.g. none, bfq, kyber). */
        char cgroup_id
                [NAME_MAX]; /**< Current cgroup name. This value must be unique. */
        char trace_replay_path[PATH_MAX]; /**< `trace-replay` binary path */
        char trace_data_path[PATH_MAX]; /**< `trace-replay` trace data path */

        void *global_config; /**< runner's global_config information */
        struct docker_info *next; /**< Contain the next `docker_info` pointer */
        char container_id[DOCKER_ID_LEN]; /**< Contain the `container_id` **/
};

/* docker-driver.c */
int docker_init(void *object);
int docker_runner(void);
int docker_valid_scheduler_test(const char *scheduler);
int docker_has_weight_scheduler(const int scheduler_index);
int docker_get_interval(const char *key, char *buffer);
int docker_get_total(const char *key, char *buffer);
void docker_free(void);

/* docker-serializer.c */
void docker_total_serializer(const struct docker_info *info,
                             const struct total_results *total, char *buffer);
void docker_realtime_serializer(const struct docker_info *info,
                                const struct realtime_log *log, char *buffer);
/* docker-info.c */
struct docker_info *docker_info_init(struct json_object *setting, int index);
int docker_is_synth_type(const char *trace_data_path);

/* docker-shm.c */
int docker_shm_init(struct docker_info *info);
int docker_shm_get(const struct docker_info *info, void *buffer);
void docker_shm_free(struct docker_info *info, int flags);

/* docker-mq.c */
int docker_mq_init(struct docker_info *info);
int docker_mq_get(const struct docker_info *info, void *buffer);
void docker_mq_free(struct docker_info *info, int flags);

#endif
