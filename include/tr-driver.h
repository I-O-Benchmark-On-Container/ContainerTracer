/**
 * @file tr-driver.h
 * @brief trace-replay를 동작시키는 driver의 선언부에 해당합니다.
 * @author BlaCkinkGJ (ss5kijun@gmail.com)
 * @version 0.1
 * @date 2020-08-05
 */
#ifndef _TR_DRIVER_H
#define _TR_DRIVER_H

#include <runner.h>
#include <json.h>

int tr_init(struct runner_op *op);
int tr_runner(struct json_object *driver_json_setting);
int tr_reader(char *buffer);
void tr_free(void);

#endif
