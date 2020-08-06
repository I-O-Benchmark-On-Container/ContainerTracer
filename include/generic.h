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

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <runner.h>
#include <tr-driver.h>

enum { TRACE_REPLAY_DRIVER = 0,
};

int get_generic_driver_index(const char *name);
int generic_driver_init(const char *name, struct runner_op *op);

#endif
