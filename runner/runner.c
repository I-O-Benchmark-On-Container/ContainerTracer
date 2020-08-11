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
void runner_config_free(struct runner_config *config, const int flags)
{
        if (NULL == config) {
                pr_info(ERROR, "invalid config location: %p\n", config);
                return;
        }

        if (RUNNER_FREE_ALL != (flags & RUNNER_FREE_ALL_MASK)) {
                pr_info(WARNING,
                        "Nothing occurred in this functions... (flags: 0x%X)\n",
                        flags);
                return;
        }

        if ((flags & RUNNER_FREE_DRIVER_MASK) && (NULL != config->op.free)) {
                (config->op.free)();
        }

        if (NULL != config->setting) {
                json_object_put(config->setting);
                config->setting = NULL;
                pr_info(INFO,
                        "setting deallcated success: expected (nil) ==> %p\n",
                        config->setting);
        }
        config->driver[0] = '\0';

        free(config);
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
        assert(NULL != json_str);

        global_config =
                (struct runner_config *)malloc(sizeof(struct runner_config));
        assert(NULL != global_config);

        memset(global_config, 0, sizeof(struct runner_config));

        json_obj = json_tokener_parse(json_str);
        if (!json_obj) {
                pr_info(ERROR, "Parse failed: \"%s\"\n", json_str);
                ret = -EINVAL;
                goto exception;
        }

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
        if (0 > ret) {
                pr_info(ERROR, "Initialize failed... (errno: %d)\n", ret);
                goto exception;
        }

        pr_info(INFO, "Successfully binding driver: %s\n",
                global_config->driver);
        if (NULL != json_obj) {
                json_object_put(json_obj);
                json_obj = NULL;
        }
        global_config->setting = NULL;
        return ret;
exception:
        errno = ret;
        if (NULL != json_obj) {
                json_object_put(json_obj);
                json_obj = NULL;
        }
        global_config->setting = NULL;
        runner_config_free(global_config, RUNNER_FREE_ALL);
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
        runner_config_free(global_config, RUNNER_FREE_ALL);
        global_config = NULL;
        pr_info(INFO, "runner free success (flags: 0x%X)\n", RUNNER_FREE_ALL);
}

/**
 * @brief 수행 결과를 담는 buffer를 만드는 함수입니다.
 *
 * @param buffer 할당을 받고자 하는 대상이 되는 버퍼에 해당합니다.
 * @param size 버퍼의 크기를 지칭합니다.
 *
 * @return buffer의 할당 성공 여부를 반환합니다.
 */
static int runner_get_result_string(char **buffer, size_t size)
{
        *buffer = (char *)malloc(size * sizeof(char));
        if (!*buffer) {
                pr_info(WARNING, "Memory allocation failed. (%s)\n", "buffer");
                return -EINVAL;
        }
        return 0;
}

/**
 * @brief `runner_get_result_string`에서 할당 받은 buffer를 해제합니다.
 *
 * @param buffer 해제 대상이 되는 동적 할당된 버퍼입니다.
 */
void runner_put_result_string(char *buffer)
{
        if (NULL != buffer) {
                free(buffer);
        }
}

/**
 * @brief 임의의 드라이버에 대한 실시간 수행 출력 결과를 반환합니다.
 *
 * @param key 출력하고자 하는 대상을 지칭하는 key입니다.
 *
 * @return buffer json 문자열로 구성된 실시간 수행 결과를 담고 있습니다.
 * 만약 할당을 실패한 경우에는 NULL이 반환됩니다.
 *
 * @warning buffer는 동적 할당된 내용이므로 반드시 외부에서
 * `runner_put_result_string`으로 해제해줘야 합니다.
 */
char *runner_get_interval_result(const char *key)
{
        char *buffer = NULL;
        int ret = 0;

        ret = runner_get_result_string(&buffer, INTERVAL_RESULT_STRING_SIZE);
        if (0 > ret) {
                goto exception;
        }

        ret = (global_config->op.get_interval)(key, buffer);
        if (0 > ret) {
                goto exception;
        }

        return buffer;
exception:
        errno = ret;
        perror("Error detected while running");
        runner_put_result_string(buffer);
        buffer = NULL;
        return buffer;
}

/**
 * @brief 임의의 드라이버에 대한 전체 수행 출력 결과를 반환합니다.
 *
 * @param key 출력하고자 하는 대상을 지칭하는 key입니다.
 *
 * @return buffer json 문자열로 구성된 전체 수행 결과를 담고 있습니다.
 * 만약 할당을 실패한 경우에는 NULL이 반환됩니다.
 *
 * @warning buffer는 동적 할당된 내용이므로 반드시 외부에서
 * `runner_put_result_string`으로 해제해줘야 합니다.
 */
char *runner_get_total_result(const char *key)
{
        char *buffer = NULL;
        int ret = 0;

        ret = runner_get_result_string(&buffer, TOTAL_RESULT_STRING_SIZE);
        if (0 > ret) {
                goto exception;
        }

        ret = (global_config->op.get_total)(key, buffer);
        if (0 > ret) {
                goto exception;
        }

        return buffer;
exception:
        errno = ret;
        perror("Error detected while running");
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
