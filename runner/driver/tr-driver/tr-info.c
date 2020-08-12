/**
 * @file tr-info.c
 * @brief info 구조체를 초기화하는 역할을 합니다.
 * @author BlaCkinkGJ (ss5kijun@gmail.com)
 * @version 0.1
 * @date 2020-08-10
 */

#include <stdlib.h>
#include <search.h>
#include <assert.h>

#include <json.h>
#include <jemalloc/jemalloc.h>

#include <driver/tr-driver.h>

/**
 * @brief 알려진 synthetic 형태에 대한 정의입니다.
 * 이 안에 있는 경우에는 파일로 확인을 하지 않습니다.
 */
static const char *global_synth_type[] = { "rand_read",  "rand_write",
                                           "rand_mixed", "seq_read",
                                           "seq_write",  "seq_mixed",
                                           NULL };

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
                if (TR_ERROR_PRINT == is_print) {
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
                if (TR_ERROR_PRINT == is_print) {
                        pr_info(ERROR, "Not exist error (key: %s)\n", key);
                }
                return -EINVAL;
        }
        strcpy(member, json_object_get_string(tmp));
        generic_strip_string(member, '\"');
        return 0;
}

/**
 * @brief 현재 trace_data_path 값이 synthetic 형태인지를 확인합니다.
 *
 * @param trace_data_path synthetic 여부를 확인하고자 하는 문자열입니다.
 *
 * @return 만약에 synthetic이면 TR_SYNTH를 아니면 TR_NOT_SYNTH를 반환합니다.
 */
static int tr_is_synth_type(const char *trace_data_path)
{
        int i = 0;
        for (i = 0; global_synth_type[i] != NULL; i++) {
                if (!strcmp(trace_data_path, global_synth_type[i])) {
                        return TR_SYNTH;
                }
        }
        return TR_NOT_SYNTH;
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

        assert(NULL != info);

        if (!json_object_object_get_ex(setting, "task_option", &tmp)) {
                pr_info(ERROR, "Not exist error (key: %s)\n", "task_option");
                ret = -EINVAL;
                goto exception;
        }

        tmp = json_object_array_get_idx(tmp, index);
        if (NULL == tmp) {
                pr_info(ERROR, "Array out-of-bound error (index: %d)\n", index);
                ret = -EINVAL;
                goto exception;
        }

        tr_info_int_value_set(tmp, "time", &info->time, TR_PRINT_NONE);
        tr_info_int_value_set(tmp, "q_depth", &info->q_depth, TR_PRINT_NONE);
        tr_info_int_value_set(tmp, "nr_thread", &info->nr_thread,
                              TR_PRINT_NONE);
        tr_info_int_value_set(tmp, "weight", &info->weight, TR_PRINT_NONE);
        tr_info_int_value_set(tmp, "trace_repeat", &info->weight,
                              TR_PRINT_NONE);
        tr_info_int_value_set(tmp, "wss", &info->wss, TR_PRINT_NONE);
        tr_info_int_value_set(tmp, "utilization", &info->utilization,
                              TR_PRINT_NONE);
        tr_info_int_value_set(tmp, "iosize", &info->iosize, TR_PRINT_NONE);
        tr_info_str_value_set(tmp, "prefix_cgroup_name",
                              info->prefix_cgroup_name, TR_PRINT_NONE);
        tr_info_str_value_set(tmp, "scheduler", info->scheduler, TR_PRINT_NONE);
        tr_info_str_value_set(tmp, "trace_replay_path", info->trace_replay_path,
                              TR_PRINT_NONE);
        tr_info_str_value_set(tmp, "device", info->device, TR_PRINT_NONE);
        ret = tr_valid_scheduler_test(info->scheduler);
        if (0 != ret) {
                pr_info(ERROR, "Unsupported scheduler (name: %s)\n",
                        info->scheduler);
                goto exception;
        }

        ret = tr_info_int_value_set(tmp, "weight", &info->weight,
                                    TR_ERROR_PRINT);
        if (0 != ret) {
                goto exception;
        }

        ret = tr_info_str_value_set(tmp, "trace_data_path",
                                    info->trace_data_path, TR_ERROR_PRINT);
        if (0 != ret) {
                goto exception;
        }

        ret = tr_info_str_value_set(tmp, "cgroup_id", info->cgroup_id,
                                    TR_ERROR_PRINT);
        if (0 != ret) {
                goto exception;
        }

        if (TR_SYNTH != tr_is_synth_type(info->trace_data_path)) {
                if (-1 == (ret = access(info->trace_data_path, F_OK))) {
                        pr_info(ERROR, "Trace data file not exist: %s\n",
                                info->trace_data_path);
                        goto exception;
                } else {
                        pr_info(INFO, "Trace data file exist: %s\n",
                                info->trace_data_path);
                }
        }
        info->ppid = getpid();

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
struct tr_info *tr_info_init(struct json_object *setting, int index)
{
        struct tr_info *info;
        int ret = 0;

        info = (struct tr_info *)malloc(sizeof(struct tr_info));
        if (!info) {
                pr_info(ERROR, "Memory allocation fail (name: %s)\n", "info");
                ret = -ENOMEM;
                goto exception;
        }

        memset(info, 0, sizeof(struct tr_info));
        info->trace_repeat = 1;

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
        ret |= tr_info_str_value_set(setting, "device", info->device,
                                     TR_ERROR_PRINT);
        ret |= tr_info_str_value_set(setting, "trace_replay_path",
                                     info->trace_replay_path, TR_ERROR_PRINT);
        if (0 != ret) {
                pr_info(ERROR, "error detected (errno: %d)\n", ret);
                goto exception;
        }

        ret = tr_valid_scheduler_test(info->scheduler);
        if (0 != ret) {
                pr_info(ERROR, "Unsupported scheduler (name: %s)\n",
                        info->scheduler);
                goto exception;
        }

        tr_info_int_value_set(setting, "weight", &info->weight, TR_PRINT_NONE);
        tr_info_int_value_set(setting, "trace_repeat", &info->weight,
                              TR_PRINT_NONE);
        tr_info_int_value_set(setting, "wss", &info->wss, TR_PRINT_NONE);
        tr_info_int_value_set(setting, "utilization", &info->utilization,
                              TR_PRINT_NONE);
        tr_info_int_value_set(setting, "iosize", &info->iosize, TR_PRINT_NONE);
        /* trace_data_path의 검사는 __tr_info_init에서 합니다. */
        tr_info_str_value_set(setting, "trace_data_path", info->trace_data_path,
                              TR_PRINT_NONE);

        ret = __tr_info_init(setting, index, info);
        if (0 != ret) {
                pr_info(ERROR, "error detected (errno: %d)\n", ret);
                goto exception;
        }

        info->mqid = info->semid = info->shmid = -1;

        info->next = NULL;

        return info;
exception:
        if (NULL != info) {
                free(info);
                info = NULL;
        }
        return info;
}
