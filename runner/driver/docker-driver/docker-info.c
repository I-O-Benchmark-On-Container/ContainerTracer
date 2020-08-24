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
 * @file docker-info.c
 * @brief info 구조체를 초기화하는 역할을 합니다.
 * @author SuhoSon (ngeol564@gmail.com)
 * @version 0.1
 * @date 2020-08-19
 */

#include <stdlib.h>
#include <search.h>
#include <assert.h>
#include <sys/stat.h>

#include <json.h>
#include <jemalloc/jemalloc.h>

#include <driver/docker-driver.h>

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
static int docker_info_int_value_set(struct json_object *setting,
                                     const char *key, unsigned int *member,
                                     int is_print)
{
        struct json_object *tmp = NULL;
        if (!json_object_object_get_ex(setting, key, &tmp)) {
                if (DOCKER_ERROR_PRINT == is_print) {
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
 * @param size 메모리의 크기를 나타냅니다.
 * @param is_print 에러를 사용자에게 출력을 할 지 말지를 선택합니다.
 *
 * @return 성공적으로 입력이 되었다면 0이 반환되고, 그렇지 않은 경우에는 -EINVAL이 반환됩니다.
 * @warning size의 값은 반드시 member의 크기보다 작거나 같아야 합니다.
 */
static int docker_info_docker_value_set(struct json_object *setting,
                                        const char *key, char *member,
                                        size_t size, int is_print)
{
        struct json_object *tmp = NULL;
        if (!json_object_object_get_ex(setting, key, &tmp)) {
                if (DOCKER_ERROR_PRINT == is_print) {
                        pr_info(ERROR, "Not exist error (key: %s)\n", key);
                }
                return -EINVAL;
        }
        snprintf(member, size, "%s", json_object_get_string(tmp));
        generic_strip_string(member, '\"');
        return 0;
}

/**
 * @brief 현재 trace_data_path 값이 synthetic 형태인지를 확인합니다.
 *
 * @param trace_data_path synthetic 여부를 확인하고자 하는 문자열입니다.
 *
 * @return 만약에 synthetic이면 DOCKER_SYNTH를 아니면 DOCKER_NOT_SYNTH를 반환합니다.
 */
static int docker_is_synth_type(const char *trace_data_path)
{
        int i = 0;
        for (i = 0; global_synth_type[i] != NULL; i++) {
                if (!strcmp(trace_data_path, global_synth_type[i])) {
                        return DOCKER_SYNTH;
                }
        }
        return DOCKER_NOT_SYNTH;
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
static int __docker_info_init(struct json_object *setting, int index,
                              struct docker_info *info)
{
        struct json_object *tmp;
        struct stat lstat_info;
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

        docker_info_int_value_set(tmp, "time", &info->time, DOCKER_PRINT_NONE);
        docker_info_int_value_set(tmp, "q_depth", &info->q_depth,
                                  DOCKER_PRINT_NONE);
        docker_info_int_value_set(tmp, "nr_thread", &info->nr_thread,
                                  DOCKER_PRINT_NONE);
        docker_info_int_value_set(tmp, "weight", &info->weight,
                                  DOCKER_PRINT_NONE);
        docker_info_int_value_set(tmp, "trace_repeat", &info->weight,
                                  DOCKER_PRINT_NONE);
        docker_info_int_value_set(tmp, "wss", &info->wss, DOCKER_PRINT_NONE);
        docker_info_int_value_set(tmp, "utilization", &info->utilization,
                                  DOCKER_PRINT_NONE);
        docker_info_int_value_set(tmp, "iosize", &info->iosize,
                                  DOCKER_PRINT_NONE);
        docker_info_docker_value_set(tmp, "prefix_cgroup_name",
                                     info->prefix_cgroup_name,
                                     sizeof(info->prefix_cgroup_name),
                                     DOCKER_PRINT_NONE);
        docker_info_docker_value_set(tmp, "scheduler", info->scheduler,
                                     sizeof(info->scheduler),
                                     DOCKER_PRINT_NONE);
        docker_info_docker_value_set(tmp, "trace_replay_path",
                                     info->trace_replay_path,
                                     sizeof(info->trace_replay_path),
                                     DOCKER_PRINT_NONE);
        docker_info_docker_value_set(tmp, "device", info->device,
                                     sizeof(info->device), DOCKER_PRINT_NONE);
        ret = docker_valid_scheduler_test(info->scheduler);
        if (0 != ret) {
                pr_info(ERROR, "Unsupported scheduler (name: %s)\n",
                        info->scheduler);
                goto exception;
        }

        ret = docker_info_int_value_set(tmp, "weight", &info->weight,
                                        DOCKER_ERROR_PRINT);
        if (0 != ret) {
                goto exception;
        }

        ret = docker_info_docker_value_set(tmp, "trace_data_path",
                                           info->trace_data_path,
                                           sizeof(info->trace_data_path),
                                           DOCKER_ERROR_PRINT);
        if (0 != ret) {
                goto exception;
        }

        ret = docker_info_docker_value_set(tmp, "cgroup_id", info->cgroup_id,
                                           sizeof(info->cgroup_id),
                                           DOCKER_ERROR_PRINT);
        if (0 != ret) {
                goto exception;
        }

        if (DOCKER_SYNTH != docker_is_synth_type(info->trace_data_path)) {
                if (-1 == (ret = lstat(info->trace_data_path, &lstat_info))) {
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
struct docker_info *docker_info_init(struct json_object *setting, int index)
{
        struct docker_info *info;
        int ret = 0;

        info = (struct docker_info *)malloc(sizeof(struct docker_info));
        if (!info) {
                pr_info(ERROR, "Memory allocation fail (name: %s)\n", "info");
                ret = -ENOMEM;
                goto exception;
        }

        memset(info, 0, sizeof(struct docker_info));
        info->trace_repeat = 1;

        ret |= docker_info_int_value_set(setting, "time", &info->time,
                                         DOCKER_ERROR_PRINT);
        ret |= docker_info_int_value_set(setting, "q_depth", &info->q_depth,
                                         DOCKER_ERROR_PRINT);
        ret |= docker_info_int_value_set(setting, "nr_thread", &info->nr_thread,
                                         DOCKER_ERROR_PRINT);
        ret |= docker_info_docker_value_set(setting, "prefix_cgroup_name",
                                            info->prefix_cgroup_name,
                                            sizeof(info->prefix_cgroup_name),
                                            DOCKER_ERROR_PRINT);
        ret |= docker_info_docker_value_set(setting, "scheduler",
                                            info->scheduler,
                                            sizeof(info->scheduler),
                                            DOCKER_ERROR_PRINT);
        ret |= docker_info_docker_value_set(setting, "device", info->device,
                                            sizeof(info->device),
                                            DOCKER_ERROR_PRINT);
        ret |= docker_info_docker_value_set(setting, "trace_replay_path",
                                            info->trace_replay_path,
                                            sizeof(info->trace_replay_path),
                                            DOCKER_ERROR_PRINT);
        if (0 != ret) {
                pr_info(ERROR, "error detected (errno: %d)\n", ret);
                goto exception;
        }

        ret = docker_valid_scheduler_test(info->scheduler);
        if (0 != ret) {
                pr_info(ERROR, "Unsupported scheduler (name: %s)\n",
                        info->scheduler);
                goto exception;
        }

        docker_info_int_value_set(setting, "weight", &info->weight,
                                  DOCKER_PRINT_NONE);
        docker_info_int_value_set(setting, "trace_repeat", &info->weight,
                                  DOCKER_PRINT_NONE);
        docker_info_int_value_set(setting, "wss", &info->wss,
                                  DOCKER_PRINT_NONE);
        docker_info_int_value_set(setting, "utilization", &info->utilization,
                                  DOCKER_PRINT_NONE);
        docker_info_int_value_set(setting, "iosize", &info->iosize,
                                  DOCKER_PRINT_NONE);
        /* trace_data_path의 검사는 __docker_info_init에서 합니다. */
        docker_info_docker_value_set(setting, "trace_data_path",
                                     info->trace_data_path,
                                     sizeof(info->trace_data_path),
                                     DOCKER_PRINT_NONE);

        ret = __docker_info_init(setting, index, info);
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
