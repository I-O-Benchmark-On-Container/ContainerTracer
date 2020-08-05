/**
 * @file tr-driver.c
 * @brief trace-replay를 동작시키는 driver 구현부에 해당합니다.
 * @author BlaCkinkGJ (ss5kijun@gmail.com)
 * @version 0.1
 * @date 2020-08-05
 */
#include <generic.h>
#include <errno.h>
#include <log.h>

int tr_init(struct runner_op *op)
{
        op->runner = tr_runner;
        op->reader = tr_reader;
        op->free = tr_free;

        if (!op->runner || !op->reader || !op->free) {
                pr_info(ERROR,
                        "invalid function pointer detected (runner: %p, reader: %p, free: %p)\n",
                        (void *)op->runner, (void *)op->reader,
                        (void *)op->free);
                return -EFAULT;
        }

        return 0;
}

int tr_runner(struct json_object *driver_json_setting)
{
        (void)driver_json_setting;
        return 0;
}

int tr_reader(char *buffer)
{
        (void)buffer;
        return 0;
}

void tr_free(void)
{
        /* NOT IMPLEMENTED*/
}
