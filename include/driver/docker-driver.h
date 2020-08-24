/**
 * @file docker-driver.h
 * @brief docker와 함께 trace-replay를 동작시키는 driver의 선언부에 해당합니다.
 * @author SuhoSon (ngeol564@gmail.com)
 * @version 0.1
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
 * @brief docker_info 구조체를 순회하는 range-based for 구문에 해당합니다.
 *
 * @param ptr 순회하면서 만나는 포인터가 담기는 곳입니다.
 * @param head 순회의 시작 위치에 해당합니다.
 *
 */
#define docker_info_list_traverse(ptr, head)                                   \
        for (ptr = head; ptr != NULL; ptr = ptr->next)

/**
 * @brief docker_json_field 구조체를 순회하는 range-based for 구문에 해당합니다.
 *
 * @param ptr 순회하면서 만나는 포인터가 담기는 곳입니다.
 * @param begin 순회의 시작 위치에 해당합니다.
 * @param end 순회의 끝에 해당합니다.
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

enum { DOCKER_IPC_NOT_FREE = 0x0, /**< IPC 객체를 제거하지 않도록 합니다. */
       DOCKER_IPC_FREE = 0x1,
       /* IPC 객체를 제거합니다. */ };

enum { DOCKER_NOT_SYNTH = 0, /**< 해당 값이 synthetic이 아님을 이이기 합니다. */
       DOCKER_SYNTH = 0x1,
       /**< 해당 값이 synthetic임을 의미합니다.  */ };

enum { DOCKER_ERROR_PRINT, /**< json의 에러를 사용자에게 노출되도록 합니다.*/
       DOCKER_PRINT_NONE, /**< json의 에러를 사용자에게 노출되지 않도록 합니다. */
};

/**
 * @brief `docker_stats_serializer`의 필드가 너무 많은 것을 해결하는 것에
 * 도움을 주는 함수에 해당합니다.
 */
struct docker_json_field {
        const char *name; /**< json의 키에 대응되는 내용입니다. */
        const void *member; /**< 키에 따른 값이 들어가는 곳에 해당합니다. */
};

/**
 * @brief total_results(include/trace_replay.h 참조)의 경우 하위로 들어가는 내용이 많으므로
 * 해당 내용을 받을 수 있도록 동적 할당을 진행할 수 있는 구조체에 해당합니다.
 */
struct docker_total_json_object {
        struct json_object *trace[MAX_THREADS]; /**< trace 정보를 담슴니다. */
        struct json_object
                *per_trace[MAX_THREADS]; /**< per_trace 정보를 담슴니다. */
        struct json_object
                *synthetic[MAX_THREADS]; /**< synthetic 정보를 담슴니다. */
        struct json_object *stats[MAX_THREADS]; /**< stats 정보를 담슴니다. */
};

/**
 * @brief 실제 각각의 프로세스에 대한 정보가 들어가는 구조체에 해당합니다.
 */
struct docker_info {
        pid_t ppid; /**< 부모의 pid가 들어갑니다. 부모 프로세스는 0을 가집니다. */
        pid_t pid; /**< fork로 자식 프로세스에서 trace-replay를 수행했을 때 해당 자식의 pid에 해당합니다.  자식 프로세스는 0 값을 가집니다. */

        unsigned int time; /**< 수행 시간을 의미합니다. (초 단위) */
        unsigned int
                q_depth; /**< 데이터가 들어가는 Queue의 깊이를 지정합니다. 일반적으로 CPU 코어 수로 할당해주면 됩니다. */
        unsigned int
                nr_thread; /**< 태스크가 사용하는 쓰레드의 갯수에 해당합니다. */

        unsigned int
                weight; /**< 일반적으로, bfq의 weight를 설정할 수 있습니다. */

        int mqid; /**< 부모 자식 간 공유하는 메시지 큐의 ID */
        int shmid; /**< 부모 자식 간 공유하는 공유 메모리의 ID */
        int semid; /**< 부모 자식 간 공유하는 세모포어의 ID */

        unsigned int trace_repeat; /**< trace의 반복 횟수 입니다. */
        unsigned int wss; /**< Working 집합의 크기입니다. */
        unsigned int utilization; /**< utilization에 해당합니다. */
        unsigned int iosize; /**< I/O 크기에 해당합니다. */

        char prefix_cgroup_name
                [NAME_MAX]; /**< /sys/fs/cgroup/blkio 에 들어갈 cgroup의 접두사가 들어갑니다. */
        char device[NAME_MAX]; /**< device의 이름이 들어갑니다. /dev/ 포함 금지!  */
        char scheduler[NAME_MAX]; /**< 스케쥴러의 이름이 들어갑니다. */
        char cgroup_id
                [NAME_MAX]; /**< 현재 cgroup의 이름입니다. 이 값은 고유값이어야 합니다. */
        char trace_replay_path
                [PATH_MAX]; /**< trace replay 실행 파일의 위치를 가리키는 위치입니다. */
        char trace_data_path
                [PATH_MAX]; /**< trace replay에서 수행할 데이터의 위치를 가리킵니다. 여기에는 synthetic 내용도 들어갈 수 있습니다. */

        void *global_config; /**< runner_config에 대한 전역 정보가 들어가게 됩니다. */
        struct docker_info
                *next; /**< 다음 docker_info의 위치가 들어가게 됩니다. */
        char container_id[DOCKER_ID_LEN]; /**< container id가 저장됩니다. **/
};

/* docker-driver.c */
int docker_init(void *object);
int docker_runner(void);
int docker_valid_scheduler_test(const char *scheduler);
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

/* docker-shm.c */
int docker_shm_init(struct docker_info *info);
int docker_shm_get(const struct docker_info *info, void *buffer);
void docker_shm_free(struct docker_info *info, int flags);

/* docker-mq.c */
int docker_mq_init(struct docker_info *info);
int docker_mq_get(const struct docker_info *info, void *buffer);
void docker_mq_free(struct docker_info *info, int flags);

#endif
