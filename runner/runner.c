/**
 * @file runner.c
 * @brief runner.h에서 선언된 내용들에 대한 정의가 들어갑니다.
 * @author BlaCkinkGJ (ss5kijun@gmail.com)
 * @version 0.1
 * @date 2020-08-04
 */

/**< system header */
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <linux/limits.h>

/**< external header */
#include <jemalloc/jemalloc.h>
#include <json.h>

/**< user header */
#include <generic.h>
#include <runner.h>
#include <log.h>

static struct runner_config *global_config = NULL;

/**
 * @brief global_runner가 가진 멤버 중 동적할당이 되어 있는 데이터를 반환합니다.
 *
 * @param flags global_runner의 멤버 중 지우고 싶은 멤버를 선택할 수 있습니다.
 */
static void __runner_free(const int flags)
{
        if (global_config == NULL) {
                pr_info(ERROR, "invalid global_config location: %p\n",
                        global_config);
                return;
        }

        if ((flags & RUNNER_FREE_ALL_MASK) != RUNNER_FREE_ALL) {
                pr_info(WARNING,
                        "Nothing occurred in this functions... (flags: 0x%X)\n",
                        flags);
                return;
        }

        if (global_config->op.free != NULL) {
                (global_config->op.free)();
        }

        if (global_config->setting != NULL) {
                json_object_put(global_config->setting);
                global_config->setting = NULL;
                pr_info(INFO,
                        "setting deallcated success: expected (nil) ==> %p\n",
                        global_config->setting);
        }
        global_config->driver[0] = '\0';

        free(global_config);
        global_config = NULL;
}

/**
 * @brief json 문자열을 읽어서 runner에 대한 설정을 진행하도록 합니다.
 *
 * @param json_str 설정에 사용되는 JSON 형태의 문자열이 들어오게 됩니다.
 *
 * @return ret 성공한 경우에 0이 반환되고, 실패한 경우에는 에러 번호가 출력됩니다.
 *
 * @exception 하나라도 데이터가 적합하게 존재하지 않는 경우에는 관련된 에러 메시지와 함께 에러 번호가 출력됩니다.
 */
int runner_init(const char *json_str)
{
        struct json_object *json_obj, *tmp;
        int ret = 0;
        assert(json_str != NULL);

        global_config =
                (struct runner_config *)malloc(sizeof(struct runner_config));
        assert(global_config != NULL);

        memset(global_config, 0, sizeof(struct runner_config));

        if (global_config->setting != NULL) {
                json_object_put(global_config->setting);
        }
        json_obj = json_tokener_parse(json_str);

        if (!json_object_object_get_ex(json_obj, "driver", &tmp)) {
                pr_info(ERROR, "Not exist error (key: %s)\n", "driver");
                ret = -EACCES;
                goto exception;
        };
        strcpy(global_config->driver,
               json_object_to_json_string_ext(tmp, JSON_C_TO_STRING_PLAIN));
        generic_strip_string(global_config->driver, '\"');
        pr_info(INFO, "driver ==> %s\n", global_config->driver);
        if (!json_object_object_get_ex(json_obj, "setting", &tmp)) {
                pr_info(ERROR, "Not exist error (key: %s)\n", "setting");
                ret = -EACCES;
                goto exception;
        };

        global_config->setting = tmp;

        ret = generic_driver_init(global_config->driver, global_config);
        if (ret < 0) {
                pr_info(ERROR, "Initialize failed... (errno: %d)\n", ret);
                goto exception;
        }

        pr_info(INFO, "Successfully binding driver: %s\n",
                global_config->driver);
        if (json_obj != NULL) {
                json_object_put(json_obj);
                json_obj = NULL;
        }
        global_config->setting = NULL;
        return ret;
exception:
        if (json_obj != NULL) {
                json_object_put(json_obj);
                json_obj = NULL;
        }
        global_config->setting = NULL;
        __runner_free(RUNNER_FREE_ALL);
        return ret;
}

/**
 * @brief 실제로 프로그램을 각각의 프로세스와 cgroup에 할당을 한 후에 실행을 하는 부분에 해당합니다.
 *
 * @return driver에서의 수행한 결과가 반환됩니다. 정상 종료의 경우에는 0이 반환됩니다.
 */
int runner_run(void)
{
        return (global_config->op.runner)();
}

/**
 * @brief __runner_free의 wrapping 함수에 해당하는 함수입니다.
 */
void runner_free(void)
{
        int flags = RUNNER_FREE_ALL;
        __runner_free(flags);
        pr_info(INFO, "runner free success (flags: 0x%X)\n", flags);
}

static int runner_get_result_string(char **buffer)
{
        *buffer = (char *)malloc(RESULT_STRING_SIZE);
        if (!*buffer) {
                pr_info(WARNING, "Memory allocation failed. (%s)\n", "buffer");
                return -EINVAL;
        }
        return 0;
}

void runner_put_result_string(char *buffer)
{
        if (buffer != NULL) {
                free(buffer);
        }
}

char *runner_get_interval_result(const char *key)
{
        char *buffer = NULL;
        int ret = 0;

        ret = runner_get_result_string(&buffer);
        if (ret) {
                goto exception;
        }

        ret = (global_config->op.get_interval)(key, buffer);
        if (ret) {
                goto exception;
        }

        pr_info(INFO, "Current buffer(key: %s) contents in %p ==> \"%s\"\n",
                key, buffer, buffer);

        return buffer;
exception:
        runner_put_result_string(buffer);
        buffer = NULL;
        return buffer;
}

char *runner_get_total_result(const char *key)
{
        char *buffer = NULL;
        int ret = 0;

        ret = runner_get_result_string(&buffer);
        if (ret) {
                goto exception;
        }

        ret = (global_config->op.get_total)(key, buffer);
        if (ret) {
                goto exception;
        }

        pr_info(INFO, "Current buffer(key: %s) contents ==> \"%s\"\n", key,
                buffer);

        return buffer;
exception:
        runner_put_result_string(buffer);
        buffer = NULL;
        return buffer;
}

/**
 * @brief global_config의 주소를 반환하여 외부에서 global config
 *
 * @return global_config의 내용의 변경이 불가능한 상수 포인터를 반환합니다.
 * @note 이는 굉장히 제한적인 디버깅과 같은 환경에서만 사용해야 합니다!!
 */
const struct runner_config *runner_get_global_config(void)
{
        return global_config;
}
