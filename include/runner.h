/**
 * @file runner.h
 * @brief runner에 대한 선언적 내용이 들어가게 됩니다.
 * @author BlaCkinkGJ (ss5kijun@gmail.com)
 * @version 0.1
 * @date 2020-08-04
 */
#ifndef _RUNNER_H
#define _RUNNER_H

/**< system header */
#include <linux/limits.h>
#include <sys/user.h>

/**< external header */
#include <json.h>

/**< user header */
#include <generic.h>

#define RESULT_STRING_SIZE (PAGE_SIZE)

enum { RUNNER_FREE_ALL_MASK = 0xFFFF,
};

enum { RUNNER_FREE_ALL =
               0xFFFF, /**< 동적으로 할당된 모든 설정 내역을 지울 때 사용합니다. */
};

struct runner_config {
        char driver[PATH_MAX];
        struct generic_driver_op op;
        struct json_object *setting;
};

int runner_init(const char *json_str);
int runner_run(void);
char *runner_get_interval_result(const char *key);
char *runner_get_total_result(const char *key);
void runner_put_result_string(char *buffer);
void runner_free(void);
const struct runner_config *runner_get_global_config(void);

#endif
