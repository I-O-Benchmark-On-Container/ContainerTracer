/**
 * @file tr-driver.c
 * @brief trace-replay를 동작시키는 driver 구현부에 해당합니다.
 * @author BlaCkinkGJ (ss5kijun@gmail.com)
 * @version 0.1
 * @date 2020-08-05
 */
#include <stdlib.h>
#include <jemalloc/jemalloc.h>
#include <errno.h>
#include <log.h>
#include <json.h>
#include <search.h>
#include <assert.h>
#include <unistd.h>

#include <generic.h>
#include <runner.h>
#include <tr-driver.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

static const char *tr_valid_scheduler[] = {
        "none",
        "kyber",
        "bfq",
        NULL,
};

static struct tr_info *info_head = NULL;

static void __tr_free(void)
{
        while (info_head != NULL) {
                struct tr_info *next = info_head->next;
                pr_info(INFO, "Delete target %p\n", info_head);
                free(info_head);
                info_head = next;
        }
        pr_info(INFO, "Do trace-replay free success ==> %p\n", info_head);
}

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
        return 0;
}

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

        ret = tr_info_str_value_set(tmp, "name", info->name, TR_ERROR_PRINT);
        if (ret != 0) {
                goto exception;
        }

        item.key = info->name;
        if (NULL != (result = hsearch(item, FIND))) {
                pr_info(ERROR, "Duplicate c-group name detected (name: %s)\n",
                        result->key);
                ret = -EINVAL;
                goto exception;
        }

        item.key = info->name;
        item.data = info;
        hsearch(item, ENTER);

exception:
        return ret;
}

static struct tr_info *tr_info_init(struct json_object *setting, int index)
{
        struct tr_info *info = NULL;
        int ret = 0;

        info = (struct tr_info *)malloc(sizeof(struct tr_info));
        if (!info) {
                pr_info(ERROR, "Memory allocation fail (name: %s)\n", "info");
                goto exception;
        }

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

int tr_init(void *object)
{
        struct runner_config *config = (struct runner_config *)object;
        struct generic_driver_op *op = &config->op;
        struct json_object *setting = config->setting;
        struct json_object *tmp = NULL;
        struct tr_info *current = NULL, *prev = NULL;

        int ret = 0, i = 0;
        int nr_tasks = -1;

        char trace_replay_path[PATH_MAX];
        char cwd[PATH_MAX];

        trace_replay_path[0] = '\0';

        getcwd(cwd, PATH_MAX);
        pr_info(INFO, "Current directory: %s\n", cwd);

        op->runner = tr_runner;
        op->get_interval = tr_get_interval;
        op->get_total = tr_get_total;
        op->free = tr_free;

        assert(op->runner != NULL);
        assert(op->get_interval != NULL);
        assert(op->get_total != NULL);
        assert(op->free != NULL);

        if (!json_object_object_get_ex(setting, "trace_replay_path", &tmp)) {
                pr_info(ERROR, "Not exist error (key: %s)\n",
                        "trace_replay_path");
                ret = -EACCES;
                goto exception;
        }
        strcpy(trace_replay_path, json_object_get_string(tmp));
        pr_info(INFO, "Trace-replay path: %s\n", trace_replay_path);

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

                if (info_head == NULL) { /**< 초기화 과정 */
                        prev = info_head = current;
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

int tr_runner(void)
{
        return 0;
}

int tr_get_interval(const char *key, char *buffer)
{
        (void)key, (void)buffer;
        return 0;
}

int tr_get_total(const char *key, char *buffer)
{
        (void)key, (void)buffer;
        return 0;
}

void tr_free(void)
{
        __tr_free();
        hdestroy();
}
