/**
 * @file tr-driver.h
 * @brief trace-replay를 동작시키는 driver의 선언부에 해당합니다.
 * @author BlaCkinkGJ (ss5kijun@gmail.com)
 * @version 0.1
 * @date 2020-08-05
 */
#ifndef _TR_DRIVER_H
#define _TR_DRIVER_H

#include <generic.h>
#include <json.h>
#include <linux/limits.h>
#include <unistd.h>
#include <sys/types.h>

enum { TR_ERROR_PRINT, /**< json의 에러를 사용자에게 노출되도록 합니다.*/
       TR_PRINT_NONE, /**< json의 에러를 사용자에게 노출되지 않도록 합니다. */
};

/**
 * @brief 실제 각각의 프로세스에 대한 정보가 들어가는 구조체에 해당합니다.
 */
struct tr_info {
        pid_t pid; /**< fork로 자식 프로세스에서 trace-replay를 수행했을 때 해당 자식의 pid에 해당합니다. */

        unsigned int time;
        unsigned int q_depth;
        unsigned int nr_thread;

        unsigned int
                weight; /**< 일반적으로, bfq의 weight를 설정할 수 있습니다. */

        int qid; /**< message queue ID */
        int shmid; /**< shared memory ID */
        int semid; /**< semaphore ID */

        char prefix_cgroup_name[NAME_MAX];
        char scheduler[NAME_MAX];
        char name[NAME_MAX];

        struct tr_info *next;
};

int tr_init(void *object);
int tr_runner(void);
int tr_get_interval(const char *key, char *buffer);
int tr_get_total(const char *key, char *buffer);
void tr_free(void);

#endif
