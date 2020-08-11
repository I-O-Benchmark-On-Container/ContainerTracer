#ifndef _LOG_H
#define _LOG_H
#include <time.h>
#include <unistd.h>
#include <stdio.h>

#define INFO "INFO"
#define WARNING "WARNING"
#define ERROR "ERROR"

/**
 * @details 각각 로그 출력 범위는 아래와 같습니다.
 * - `LOG_INFO`: INFO, WARNING, ERROR에 해당하는 로그를 출력합니다.
 * - `LOG_WARNING`: WARNING, ERROR에 해당하는 로그를 출력합니다.
 * - `LOG_ERROR`: ERROR에 해당하는 로그만 출력합니다.
 * - `UNDEFINED`: 아무것도 출력하지 않습니다.
 */
// #define LOG_INFO
//#define LOG_WARNING
//#define LOG_ERROR

#if defined(LOG_INFO)
#define pr_info(level, msg, ...)                                               \
        fprintf(stderr, "(" level ")[{%lfs} %s(%s):%d] (pid: %d) " msg,        \
                ((double)clock() / CLOCKS_PER_SEC), __FILE__, __func__,        \
                __LINE__, getpid(), ##__VA_ARGS__)
#elif defined(LOG_WARNING)
#include <string.h>
#define pr_info(level, msg, ...)                                               \
        do {                                                                   \
                if (!strcmp(level, WARNING) || !strcmp(level, ERROR))          \
                        fprintf(stderr,                                        \
                                "(" level                                      \
                                ")[{%lfs} %s(%s):%d] (pid: %d) " msg,          \
                                ((double)clock() / CLOCKS_PER_SEC), __FILE__,  \
                                __func__, __LINE__, getpid(), ##__VA_ARGS__);  \
        } while (0)
#elif defined(LOG_ERROR)
#include <string.h>
#define pr_info(level, msg, ...)                                               \
        do {                                                                   \
                if (!strcmp(level, ERROR))                                     \
                        fprintf(stderr,                                        \
                                "(" level                                      \
                                ")[{%lfs} %s(%s):%d] (pid: %d) " msg,          \
                                ((double)clock() / CLOCKS_PER_SEC), __FILE__,  \
                                __func__, __LINE__, getpid(), ##__VA_ARGS__);  \
        } while (0)
#else
#define pr_info(level, msg, ...)                                               \
        do {                                                                   \
                (void)level;                                                   \
                (void)msg;                                                     \
        } while (0)
#endif

#endif
