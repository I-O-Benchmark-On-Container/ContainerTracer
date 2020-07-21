#ifndef _LOG_H
#define _LOG_H
#include <time.h>
#include <stdio.h>

#define INFO "INFO"
#define WARNING "WARNING"
#define ERROR "ERROR"

#define pr_info(level, msg, ...)                                               \
        fprintf(stderr, "(" level ")[{%lfs} %s(%s):%d] " msg,                  \
                ((double)clock() / CLOCKS_PER_SEC), __FILE__, __func__,        \
                __LINE__, ##__VA_ARGS__)

#endif
