/**
 * @file tr-driver.h
 * @brief trace-replay를 동작시키는 driver의 선언부에 해당합니다.
 * @author BlaCkinkGJ (ss5kijun@gmail.com)
 * @version 0.1
 * @date 2020-08-05
 */
#ifndef _TR_DRIVER_H
#define _TR_DRIVER_H

#include <linux/limits.h>
#include <unistd.h>
#include <sys/types.h>

#include <json.h>

#include <generic.h>
#include <trace_replay.h>

#define tr_info_list_traverse(ptr, head)                                       \
        for (ptr = head; ptr != NULL; ptr = ptr->next)

#define tr_json_field_traverse(ptr, begin, end)                                \
        for (ptr = begin; ptr != end; ptr++)

#ifdef DEBUG
#define tr_print_info(info)                                                    \
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
                info, info->pid, info->time, info->q_depth, info->nr_thread,   \
                info->weight, info->trace_repeat, info->wss,                   \
                info->utilization, info->iosize, info->mqid, info->shmid,      \
                info->semid, info->prefix_cgroup_name, info->scheduler,        \
                info->cgroup_id, info->trace_replay_path,                      \
                info->trace_data_path, info->device, info->global_config,      \
                info->next);
#endif

enum { TR_IPC_NOT_FREE = 0x0, TR_IPC_FREE = 0x1 };
enum { TR_NOT_SYNTH = 0, TR_SYNTH = 0x1 };

enum { TR_ERROR_PRINT, /**< json의 에러를 사용자에게 노출되도록 합니다.*/
       TR_PRINT_NONE, /**< json의 에러를 사용자에게 노출되지 않도록 합니다. */
};

struct tr_json_field {
        const char *name;
        const void *member;
};
struct tr_total_json_object {
        struct json_object *trace[MAX_THREADS];
        struct json_object *per_trace[MAX_THREADS];
        struct json_object *synthetic[MAX_THREADS];
        struct json_object *stats[MAX_THREADS];
};
/**
 * @brief 실제 각각의 프로세스에 대한 정보가 들어가는 구조체에 해당합니다.
 */
struct tr_info {
        pid_t ppid;
        pid_t pid; /**< fork로 자식 프로세스에서 trace-replay를 수행했을 때 해당 자식의 pid에 해당합니다. */

        unsigned int time;
        unsigned int q_depth;
        unsigned int nr_thread;

        unsigned int
                weight; /**< 일반적으로, bfq의 weight를 설정할 수 있습니다. */

        int mqid; /**< message queue ID */
        int shmid; /**< shared memory ID */
        int semid; /**< semaphore ID */

        unsigned int trace_repeat;
        unsigned int wss; /**< Working set size */
        unsigned int utilization;
        unsigned int iosize;

        char prefix_cgroup_name[NAME_MAX];
        char device[NAME_MAX];
        char scheduler[NAME_MAX];
        char cgroup_id
                [NAME_MAX]; /**< 현재 cgroup의 이름입니다. 이 값은 고유값이어야 합니다. */
        char trace_replay_path[PATH_MAX];
        char trace_data_path[PATH_MAX];

        void *global_config;
        struct tr_info *next;
};

/* tr-driver.c */
int tr_init(void *object);
int tr_runner(void);
int tr_valid_scheduler_test(const char *scheduler);
int tr_get_interval(const char *key, char *buffer);
int tr_get_total(const char *key, char *buffer);
void tr_free(void);

/* tr-serializer.c */
void tr_total_serializer(const struct tr_info *info,
                         const struct total_results *total, char *buffer);
void tr_realtime_serializer(const struct tr_info *info,
                            const struct realtime_log *log, char *buffer);
/* tr-info.c */
struct tr_info *tr_info_init(struct json_object *setting, int index);

/* tr-shm.c */
int tr_shm_init(struct tr_info *info);
int tr_shm_get(const struct tr_info *info, void *buffer);
void tr_shm_free(struct tr_info *info, int flags);

/* tr-mq.c */
int tr_mq_init(struct tr_info *info);
int tr_mq_get(const struct tr_info *info, void *buffer);
void tr_mq_free(struct tr_info *info, int flags);

#endif
