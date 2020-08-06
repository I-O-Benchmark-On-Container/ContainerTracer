/**
 * @file generic.h
 * @brief driver의 매핑 정보 및 runner를 총괄하는 헤더입니다. 관련 정의 내용은 `runner/generic.c`에 존재합니다.
 * @author BlaCkinkGJ (ss5kijun@gmail.com)
 * @version 0.1
 * @date 2020-08-05
 */
#ifndef _GENERIC_H
#define _GENERIC_H

#define MAX_DRIVERS 10

/**< system header */
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <linux/limits.h>

/**< external header */
#include <json.h>

/**< user header */

enum { TRACE_REPLAY_DRIVER = 0,
};

struct generic_driver_op {
        int (*runner)(void);
        int (*get_interval)(const char *key, char *buffer);
        int (*get_total)(const char *key, char *buffer);
        void (*free)(void);
};

int get_generic_driver_index(const char *name);
int generic_driver_init(const char *name, void *object);

/**
 * @brief json의 문자열 값을 받는 경우에는 \" 값이 있을 수 있습니다. 따라서, 그 값을 전부 없애주는 데 사용합니다.
 *
 * @param str 입력되는 문자열에 해당합니다.
 * @param ch 지우고자 하는 단어에 해당합니다.
 *
 * @note 이 함수는 입력된 문자열을 선형적으로 읽기에 성능에 부정적인 영향을 끼칠 수 있습니다.
 */
static inline void generic_strip_string(char *str, char ch)
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

#endif
