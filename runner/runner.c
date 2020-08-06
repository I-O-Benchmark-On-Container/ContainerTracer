/**
 * @file runner.c
 * @brief runner.h에서 선언된 내용들에 대한 정의가 들어갑니다.
 * @author BlaCkinkGJ (ss5kijun@gmail.com)
 * @version 0.1
 * @date 2020-08-04
 */
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <log.h>
#include <json.h>
#include <assert.h>
#include <linux/limits.h>

#include <generic.h>

static struct runner_config global_config;

static void runner_strip_string(char *str, char ch)
{
        int i = 0, j = 0;
        while (i < PATH_MAX && str[i] != '\0') {
                i++;
                if (str[i] == ch) {
                        continue;
                }
                str[j] = str[i];
                j++;
        }
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
        json_obj = json_tokener_parse(json_str);

        if (!json_object_object_get_ex(json_obj, "driver", &tmp)) {
                pr_info(ERROR, "Not exist error (key: %s)", "driver");
                ret = -EACCES;
                goto exit;
        };
        strcpy(global_config.driver,
               json_object_to_json_string_ext(tmp, JSON_C_TO_STRING_PLAIN));
        runner_strip_string(global_config.driver, '\"');
        pr_info(INFO, "driver ==> %s\n", global_config.driver);
        if (!json_object_object_get_ex(json_obj, "setting", &tmp)) {
                pr_info(ERROR, "Not exist error (key: %s)", "setting");
                ret = -EACCES;
                goto exit;
        };
        global_config.driver_json_setting = tmp;
        if (!global_config.driver_json_setting) {
                pr_info(ERROR, "global_config.driver_json_setting is %p\n",
                        (void *)global_config.driver_json_setting);
                ret = -EINVAL;
                goto exit;
        }

        global_config.op.reader = NULL;
        global_config.op.runner = NULL;
        global_config.op.free = NULL;

        ret = generic_driver_init(global_config.driver, &global_config.op);
        if (ret < 0) {
                pr_info(ERROR, "Initialize failed... (errno: %d)\n", ret);
                goto exit;
        }

        assert(global_config.op.reader != NULL);
        assert(global_config.op.runner != NULL);
        assert(global_config.op.free != NULL);

        pr_info(INFO, "Successfully binding driver: %s\n",
                global_config.driver);

exit:
        return ret;
}

/**
 * @brief global_config의 주소를 반환하여 외부에서 global config
 *
 * @return global_config의 내용의 변경이 불가능한 상수 포인터를 반환합니다.
 */
const struct runner_config *runner_get_global_config(void)
{
        return &global_config;
}
