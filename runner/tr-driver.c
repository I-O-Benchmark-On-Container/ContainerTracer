/**
 * @file tr-driver.c
 * @brief trace-replay를 동작시키는 driver 구현부에 해당합니다.
 * @author BlaCkinkGJ (ss5kijun@gmail.com)
 * @version 0.1
 * @date 2020-08-05
 */

#define _GNU_SOURCE

/**< system header */
#include <stdlib.h>
#include <errno.h>
#include <search.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

/**< external header */
#include <json.h>
#include <jemalloc/jemalloc.h>

/**< user header */
#include <generic.h>
#include <runner.h>
#include <tr-driver.h>
#include <log.h>
#include <sync.h>

static const char *tr_valid_scheduler[] = {
        "none",
        "kyber",
        "bfq",
        NULL,
}; /**< 현재 드라이버에서 사용 가능한 스케쥴러가 들어갑니다. */

static char *global_program_path =
        NULL; /**< trace-replay 실행 파일의 경로가 들어갑니다. */
static struct tr_info *global_info_head =
        NULL; /**< trace-replay의 각각을 실행시킬 때 필요한 정보를 담고있는 구조체 리스트의 헤드입니다. */

/**
 * @brief 실질적으로 trace-replay 관련 구조체의 동적 할당된 내용을 해제하는 부분에 해당합니다.
 */
static void __tr_free(void)
{
        if (global_program_path != NULL) {
                free(global_program_path);
                global_program_path = NULL;
        }

        while (global_info_head != NULL) {
                struct tr_info *next = global_info_head->next;
                pr_info(INFO, "Delete target %p\n", global_info_head);
                free(global_info_head);
                global_info_head = next;
        }
        pr_info(INFO, "Do trace-replay free success ==> %p\n",
                global_info_head);
}

/**
 * @brief 정수 형태의 `info->(member)`에 json에서 값을 읽어서 값을 주도록 합니다. 
 *
 * @param setting 탐색을 할 특정 위치를 지칭합니다.
 * @param key json에서 가져오고자 하는 데이터의 key 또는 field에 해당합니다.
 * @param member 실제 값이 삽입되는 위치에 해당합니다.
 * @param is_print 에러를 사용자에게 출력을 할 지 말지를 선택합니다.
 *
 * @return 성공적으로 입력이 되었다면 0이 반환되고, 그렇지 않은 경우에는 -EINVAL이 반환됩니다.
 */
static int tr_info_int_value_set(struct json_object *setting, const char *key,
                                 unsigned int *member, int is_print)
{
        struct json_object *tmp = NULL;
        if (!json_object_object_get_ex(setting, key, &tmp)) {
                if (is_print == TR_ERROR_PRINT) {
                        pr_info(ERROR, "Not exist error (key: %s)\n", key);
                }
                return -EINVAL;
        }
        *member = json_object_get_int(tmp);
        return 0;
}

/**
 * @brief 문자열 형태의 `info->(member)`에 json에서 값을 읽어서 값을 주도록 합니다. 
 *
 * @param setting 탐색을 할 특정 위치를 지칭합니다.
 * @param key json에서 가져오고자 하는 데이터의 key 또는 field에 해당합니다.
 * @param member 실제 값이 삽입되는 위치에 해당합니다.
 * @param is_print 에러를 사용자에게 출력을 할 지 말지를 선택합니다.
 *
 * @return 성공적으로 입력이 되었다면 0이 반환되고, 그렇지 않은 경우에는 -EINVAL이 반환됩니다.
 */
static int tr_info_str_value_set(struct json_object *setting, const char *key,
                                 char *member, int is_print)
{
        struct json_object *tmp = NULL;
        if (!json_object_object_get_ex(setting, key, &tmp)) {
                if (is_print == TR_ERROR_PRINT) {
                        pr_info(ERROR, "Not exist error (key: %s)\n", key);
                }
                return -EINVAL;
        }
        strcpy(member, json_object_get_string(tmp));
        generic_strip_string(member, '\"');
        return 0;
}

/**
 * @brief 현재 입력되는 scheduler가 driver가 지원하는 지를 확인하도록 합니다.
 *
 * @param scheduler 검사하고자 하는 스케쥴러 문자열을 가진 문자열 포인터입니다.
 *
 * @return 가지고 있는 경우 0이 반환되고, 그렇지 않은 경우 -EINVAL이 반환됩니다.
 */
static int valid_scheduler_test(const char *scheduler)
{
        const char *index = tr_valid_scheduler[0];
        while (index != NULL) {
                if (strcmp(scheduler, index) == 0) {
                        return 0;
                }
                index++;
        }
        return -EINVAL;
}

/**
 * @brief 각각의 프로세스의 동작 옵션을 설정을 해주도록 합니다.
 *
 * @param setting 설정 값을 가지고 있는 json 객체의 포인터에 해당합니다.
 * @param index json의 task_option의 배열 순번에 대한 정보를 가집니다.
 * @param info 실제 값이 들어가는 위치에 해당합니다.
 *
 * @return 정상적으로 초기화가 된 경우 0을 그렇지 않은 경우 적절한 오류 번호를 반환합니다. 
 */
static int __tr_info_init(struct json_object *setting, int index,
                          struct tr_info *info)
{
        struct json_object *tmp;
        int ret = 0;

        ENTRY item; /**< hsearch를 위해서 사용되는 변수입니다. */
        ENTRY *result;

        assert(info != NULL);

        if (!json_object_object_get_ex(setting, "task_option", &tmp)) {
                pr_info(ERROR, "Not exist error (key: %s)\n", "task_option");
                ret = -EINVAL;
                goto exception;
        }

        tmp = json_object_array_get_idx(tmp, index);
        if (tmp == NULL) {
                pr_info(ERROR, "Array out-of-bound error (index: %d)\n", index);
                ret = -EINVAL;
                goto exception;
        }

        tr_info_int_value_set(tmp, "time", &info->time, TR_PRINT_NONE);
        tr_info_int_value_set(tmp, "q_depth", &info->q_depth, TR_PRINT_NONE);
        tr_info_int_value_set(tmp, "nr_thread", &info->nr_thread,
                              TR_PRINT_NONE);
        tr_info_str_value_set(tmp, "prefix_cgroup_name",
                              info->prefix_cgroup_name, TR_PRINT_NONE);
        tr_info_str_value_set(tmp, "scheduler", info->scheduler, TR_PRINT_NONE);
        ret = valid_scheduler_test(info->scheduler);
        if (ret != 0) {
                pr_info(ERROR, "Unsupported scheduler (name: %s)\n",
                        info->scheduler);
                goto exception;
        }

        ret = tr_info_int_value_set(tmp, "weight", &info->weight,
                                    TR_ERROR_PRINT);
        if (ret != 0) {
                goto exception;
        }

        ret = tr_info_str_value_set(tmp, "trace_data_path",
                                    info->trace_data_path, TR_ERROR_PRINT);
        if (ret != 0) {
                goto exception;
        }

        ret = tr_info_str_value_set(tmp, "cgroup_id", info->cgroup_id,
                                    TR_ERROR_PRINT);
        if (ret != 0) {
                goto exception;
        }

        if ((ret = access(info->trace_data_path, F_OK)) == -1) {
                pr_info(ERROR, "Trace data file not exist: %s\n",
                        info->trace_data_path);
                goto exception;
        } else {
                pr_info(INFO, "Trace data file exist: %s\n",
                        info->trace_data_path);
        }

        item.key = info->cgroup_id;
        if (NULL != (result = hsearch(item, FIND))) {
                pr_info(ERROR, "Duplicate c-group name detected (name: %s)\n",
                        result->key);
                ret = -EINVAL;
                goto exception;
        }

        item.key = info->cgroup_id;
        item.data = info;
        hsearch(item, ENTER);

exception:
        return ret;
}

/**
 * @brief 각 프로세스에 들어가는 info 객체를 __생성__하고 구축하여 반환합니다.
 *
 * @param setting 설정 값을 가지고 있는 json 객체의 포인터에 해당합니다.
 * @param index json의 task_option의 배열 순번에 대한 정보를 가집니다.
 *
 * @return 정상적으로 초기화가 된 경우 0을 그렇지 않은 경우 적절한 오류 번호를 반환합니다. 
 */
static struct tr_info *tr_info_init(struct json_object *setting, int index)
{
        struct tr_info *info = NULL;
        int ret = 0;

        info = (struct tr_info *)malloc(sizeof(struct tr_info));
        if (!info) {
                pr_info(ERROR, "Memory allocation fail (name: %s)\n", "info");
                ret = -ENOMEM;
                goto exception;
        }

        memset(info, 0, sizeof(struct tr_info));

        ret |= tr_info_int_value_set(setting, "time", &info->time,
                                     TR_ERROR_PRINT);
        ret |= tr_info_int_value_set(setting, "q_depth", &info->q_depth,
                                     TR_ERROR_PRINT);
        ret |= tr_info_int_value_set(setting, "nr_thread", &info->nr_thread,
                                     TR_ERROR_PRINT);
        ret |= tr_info_str_value_set(setting, "prefix_cgroup_name",
                                     info->prefix_cgroup_name, TR_ERROR_PRINT);
        ret |= tr_info_str_value_set(setting, "scheduler", info->scheduler,
                                     TR_ERROR_PRINT);
        if (ret != 0) {
                pr_info(ERROR, "error detected (errno: %d)\n", ret);
                goto exception;
        }

        ret = valid_scheduler_test(info->scheduler);
        if (ret != 0) {
                pr_info(ERROR, "Unsupported scheduler (name: %s)\n",
                        info->scheduler);
                goto exception;
        }

        ret = __tr_info_init(setting, index, info);
        if (ret != 0) {
                pr_info(ERROR, "error detected (errno: %d)\n", ret);
                goto exception;
        }

        info->next = NULL;

        return info;
exception:
        if (info != NULL) {
                free(info);
                info = NULL;
        }
        return info;
}

/**
 * @brief 전역 옵션을 설정한 후에 각각의 프로세스 별로 할당할 옵션을 설정하도록 합니다.
 *
 * @param object 전역 runner_config의 포인터가 들어가야 합니다.
 *
 * @return 정상적으로 초기화가 된 경우 0을 그렇지 않은 경우 적절한 오류 번호를 반환합니다. 
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

        assert(op->runner != NULL);
        assert(op->get_interval != NULL);
        assert(op->get_total != NULL);
        assert(op->free != NULL);

        global_program_path = (char *)malloc(sizeof(char) * PATH_MAX);
        if (!global_program_path) {
                pr_info(ERROR, "Memory allocation fail (name: %s)\n",
                        "global_program_path");
                ret = -ENOMEM;
                goto exception;
        }

        if (!json_object_object_get_ex(setting, "trace_replay_path", &tmp)) {
                pr_info(ERROR, "Not exist error (key: %s)\n",
                        "trace_replay_path");
                ret = -EACCES;
                goto exception;
        }
        strcpy(global_program_path, json_object_get_string(tmp));
        pr_info(INFO, "Trace-replay path: %s\n", global_program_path);
        if ((ret = access(global_program_path, F_OK)) == -1) {
                pr_info(ERROR, "trace-replay not exist: %s\n",
                        global_program_path);
                goto exception;
        } else {
                pr_info(INFO, "trace-replay exist: %s\n", global_program_path);
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
                if (current == NULL) { /**< 할당 실패가 벌어진 경우 */
                        ret = -ENOMEM;
                        goto exception;
                }

                if (global_info_head == NULL) { /**< 초기화 과정 */
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
 * @brief 현재 info의 내용을 출력합니다. 
 *
 * @param info 출력하고자 하는 info의 포인터에 해당합니다.
 */
static void tr_debug(const struct tr_info *info)
{
        pr_info(INFO,
                "\n"
                "\t\t[[ current %p ]]\n"
                "\t\tpid: %d\n"
                "\t\ttime: %u\n"
                "\t\tq_depth: %u\n"
                "\t\tnr_thread: %u\n"
                "\t\tweight: %u\n"
                "\t\tqid: %d\n"
                "\t\tshmid: %d\n"
                "\t\tsemid: %d\n"
                "\t\tprefix_cgroup_name: %s\n"
                "\t\tscheduler: %s\n"
                "\t\tname: %s\n"
                "\t\ttrace_data_path: %s\n"
                "\t\tnext: %p\n",
                info, info->pid, info->time, info->q_depth, info->nr_thread,
                info->weight, info->qid, info->shmid, info->semid,
                info->prefix_cgroup_name, info->scheduler, info->cgroup_id,
                info->trace_data_path, info->next);
}

/**
 * @brief trace-replay를 실행시키도록 합니다.
 *
 * @return 정상적으로 동작이 된 경우 0을 그렇지 않은 경우 적절한 오류 번호를 반환합니다. 
 */
int tr_runner(void)
{
        const struct tr_info *current = global_info_head;
        while (current != NULL) {
                tr_debug(current);
                current = current->next;
        }
        return 0;
}

/**
 * @brief trace-replay가 실행 중일 때의 정보를 반환합니다.
 *
 * @param key 임의의 cgroup_id에 해당합니다.
 * @param buffer 값이 반환되는 위치에 해당합니다.
 *
 * @return 정상적으로 동작이 된 경우 0을 그렇지 않은 경우 적절한 오류 번호를 반환합니다. 
 */
int tr_get_interval(const char *key, char *buffer)
{
        (void)key, (void)buffer;
        return 0;
}

/**
 * @brief trace-replay가 동작 완료한 후의 정보를 반환합니다.
 *
 * @param key 임의의 cgroup_id에 해당합니다.
 * @param buffer 값이 반환되는 위치에 해당합니다.
 *
 * @return 정상적으로 동작이 된 경우 0을 그렇지 않은 경우 적절한 오류 번호를 반환합니다. 
 */

int tr_get_total(const char *key, char *buffer)
{
        (void)key, (void)buffer;
        return 0;
}

/**
 * @brief trace-replay 관련 동적 할당 정보를 해제합니다.
 */
void tr_free(void)
{
        __tr_free();
        hdestroy();
}
