/**
 * @file runner.h
 * @brief runner에 대한 선언적 내용이 들어가게 됩니다.
 * @author BlaCkinkGJ (ss5kijun@gmail.com)
 * @version 0.1
 * @date 2020-08-04
 */
#ifndef _RUNNER_H
#define _RUNNER_H

#include <linux/limits.h>
#include <json.h>

struct runner_op {
        int (*runner)(struct json_object *);
        int (*reader)(char *buffer);
        void (*free)(void);
};

struct runner_config {
        char driver[PATH_MAX];
        struct json_object *driver_json_setting;
        struct runner_op op;
};

int runner_init(const char *json_str);
int runner_run(struct json_object *);
void runner_reader(char *buffer);
const struct runner_config *runner_get_global_config(void);

#endif
