/**
 * @file tr-driver.c
 * @brief trace-replay를 동작시키는 driver 구현부에 해당합니다.
 * @author BlaCkinkGJ (ss5kijun@gmail.com)
 * @version 0.1
 * @date 2020-08-05
 */

#include <stdlib.h>
#include <errno.h>
#include <search.h>
#include <assert.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/syscall.h>

#include <json.h>
#include <jemalloc/jemalloc.h>

#include <generic.h>
#include <runner.h>
#include <driver/tr-driver.h>
#include <log.h>
#include <sync.h>
#include <trace_replay.h>

enum { TR_NONE_SCHEDULER = 0,
       TR_KYBER_SCHEDULER,
       TR_BFQ_SCHEDULER,
};

/**
 * @brief 현재 tr-driver에서 지원하는 I/O 스케줄러가 들어가게 됩니다.
 * @warning kyber는 SCSI를 지원하지 않음을 유의해주시길 바랍니다.
 */
static const char *tr_valid_scheduler[] = {
        [TR_NONE_SCHEDULER] = "none",
        [TR_KYBER_SCHEDULER] = "kyber",
        [TR_BFQ_SCHEDULER] = "bfq",
        NULL,
};

static struct tr_info *global_info_head =
        NULL; /**< trace-replay의 각각을 실행시킬 때 필요한 정보를 담고있는 구조체 리스트의 헤드입니다. */

/**
 * @brief 자식이 생성되자마자 부모가 자식을 죽이는 명령을 보낸 경우에 각종 자식이 가진 자원을 정리합니다.
 *
 * @param signum 현재 받은 시그널에 해당합니다.
 * @note 이 함수는 SIGTEM을 캡쳐합니다. (SIGKILL은 캡처되지 않으므로 사용해서는 안됩니다!)
	 */
static void tr_kill_handle(int signum)
{
        struct tr_info *current = global_info_head;

        (void)signum;
        pr_info(INFO, "TERMINATE SEQUENCE DETECTED(signum: %d, pid: %d)\n",
                signum, getpid());
        if (current) {
                assert(NULL != current->global_config);
                runner_config_free(current->global_config, RUNNER_FREE_ALL);
        }
        _exit(EXIT_SUCCESS);
}

/**
 * @brief 실질적으로 trace-replay 관련 구조체의 동적 할당된 내용을 해제하는 부분에 해당합니다.
 * @note http://www.ascii-art.de/ascii/def/dr_who.txt
 */
static void __tr_free(void)
{
        while (global_info_head != NULL) {
                struct tr_info *current = global_info_head;
                struct tr_info *next = global_info_head->next;

                if (getpid() == current->ppid &&
                    0 != current->pid) { /* 자식 프로세스를 죽입니다. */
                        int status = 0;
                        /********************************************
                                                 Exterminate!
                                                /
                                           ___
                                   D>=G==='   '.
                                         |======|
                                         |======|
                                     )--/]IIIIII]
                                        |_______|
                                        C O O O D
                                       C O  O  O D
                                      C  O  O  O  D
                                      C__O__O__O__D
                                     [_____________]
                         ********************************************/
                        kill(current->pid, SIGTERM);
                        if (waitpid(current->pid, &status, 0) < 0) {
                                pr_info(WARNING,
                                        "waitpid error (pid: %d, status: 0x%X)\n",
                                        current->pid, status);
                                _exit(EXIT_FAILURE);
                        }
                        /* IPC 객체를 삭제합니다. */
                        tr_shm_free(current, TR_IPC_FREE);
                        tr_mq_free(current, TR_IPC_FREE);
                }

                pr_info(INFO, "Delete target %p\n", current);
                free(current);

                global_info_head = next;
        }
        pr_info(INFO, "Do trace-replay free success ==> %p\n",
                global_info_head);
}

/**
 * @brief 현재 입력되는 scheduler가 driver가 지원하는 지를 확인하도록 합니다.
 *
 * @param scheduler 검사하고자 하는 스케쥴러 문자열을 가진 문자열 포인터입니다.
 *
 * @return 가지고 있는 경우 0이 반환되고, 그렇지 않은 경우 -EINVAL이 반환됩니다.
 */
int tr_valid_scheduler_test(const char *scheduler)
{
        const char *index = tr_valid_scheduler[0];
        while (index != NULL) {
                if (strcmp(scheduler, index) == 0) {
                        return 0;
                }
                index++;
        }
        return -EINVAL;
}

/**
 * @brief 전역 옵션을 설정한 후에 각각의 프로세스 별로 할당할 옵션을 설정하도록 합니다.
 *
 * @param object 전역 runner_config의 포인터가 들어가야 합니다.
 *
 * @return 정상적으로 초기화가 된 경우 0을 그렇지 않은 경우 적절한 오류 번호를 반환합니다. 
 * @warning 자식 프로세스에서 이 함수가 절대로 실행되서는 안됩니다.
 */
int tr_init(void *object)
{
        struct runner_config *config = (struct runner_config *)object;
        struct generic_driver_op *op = &config->op;
        struct json_object *setting = config->setting;
        struct json_object *tmp = NULL;
        struct tr_info *current = NULL, *prev = NULL;

        int ret = 0, i = 0;
        int nr_tasks = -1;

        op->runner = tr_runner;
        op->get_interval = tr_get_interval;
        op->get_total = tr_get_total;
        op->free = tr_free;

        assert(NULL != op->runner);
        assert(NULL != op->get_interval);
        assert(NULL != op->get_total);
        assert(NULL != op->free);

        if (getuid() != 0) {
                pr_info(ERROR,
                        "You have to run this program with superuser privilege\n");
                return -EINVAL;
        }

        if (!json_object_object_get_ex(setting, "trace_replay_path", &tmp)) {
                pr_info(ERROR, "Not exist error (key: %s)\n",
                        "trace_replay_path");
                ret = -EACCES;
                goto exception;
        }

        if (!json_object_object_get_ex(setting, "nr_tasks", &tmp)) {
                pr_info(ERROR, "Not exist error (key: %s)\n", "nr_tasks");
                ret = -EACCES;
                goto exception;
        }
        nr_tasks = json_object_get_int(tmp);
        pr_info(INFO, "Generate %d tasks\n", nr_tasks);

        if (!hcreate(nr_tasks + 1)) {
                pr_info(ERROR, "hash search table create failed. (size: %d)\n",
                        nr_tasks + 1);
                ret = -ENOMEM;
                goto exception;
        }

        for (i = 0; i < nr_tasks; i++) {
                current = tr_info_init(setting, i);
                if (!current) { /* 할당 실패가 벌어진 경우 */
                        ret = -ENOMEM;
                        goto exception;
                }
                current->global_config = object;
                if (!current->global_config) {
                        ret = -EFAULT;
                        goto exception;
                }

                if (global_info_head == NULL) { /* 초기화 과정 */
                        prev = global_info_head = current;
                } else {
                        prev->next = current;
                        prev = current;
                }
        }

        return ret;
exception:
        __tr_free();
        return ret;
}

/**
 * @brief control group에 자식 프로세스가 작동할 수 있도록 설정합니다.
 *
 * @param current 현재 프로세스의 정보를 가진 구조체를 가리킵니다.
 *
 * @return 정상 종료한 경우에는 0 그 이외에는 적절한 errno이 반환됩니다.
 */
static int tr_set_cgroup_state(struct tr_info *current)
{
        int ret = 0;
        char cmd[PATH_MAX];

        /* cgroup을 생성하는 과정에 해당합니다. */
        snprintf(cmd, PATH_MAX, "mkdir /sys/fs/cgroup/blkio/%s%d",
                 current->prefix_cgroup_name, current->pid);
        pr_info(INFO, "Do command: \"%s\"\n", cmd);
        ret = system(cmd);
        if (ret) {
                pr_info(ERROR, "Cannot create the directory: \"%s\"\n", cmd);
                return -EINVAL;
        }

        ret = strcmp(current->scheduler, tr_valid_scheduler[TR_BFQ_SCHEDULER]);
        if (ret == 0) { /* BFQ 스케쥴러이면 weight를 설정해줍니다. */
                snprintf(cmd, PATH_MAX,
                         "echo %d > /sys/fs/cgroup/blkio/%s%d/blkio.%s.weight",
                         current->weight, current->prefix_cgroup_name,
                         current->pid, current->scheduler);
                pr_info(INFO, "Do command: \"%s\"\n", cmd);
                ret = system(cmd);
                if (ret) {
                        pr_info(ERROR, "Cannot set weight: \"%s\"\n", cmd);
                        return -EINVAL;
                }
        }

        snprintf(cmd, PATH_MAX, "echo %d > /sys/fs/cgroup/blkio/%s%d/tasks",
                 current->pid, current->prefix_cgroup_name, current->pid);
        pr_info(INFO, "Do command: \"%s\"\n", cmd);
        ret = system(cmd);
        if (ret) {
                pr_info(ERROR, "Cannot hang (pid: %d) to control group: %s\n",
                        current->pid, cmd);
                return -EINVAL;
        }

        pr_info(INFO, "CGROUP READY (PID: %d)\n", current->pid);

        return 0;
}

/**
 * @brief 자식 프로세스에 trace-replay를 올려서 동작시키는 함수입니다.
 *
 * @param current 자식 프로세스에 돌릴 trace-replay 설정에 해당합니다.
 *
 * @return 성공한 경우에는 0, 그렇지 않은 경우 음수 값이 들어갑니다.
 */
static int tr_do_exec(struct tr_info *current)
{
        struct tr_info info;
        char filename[PATH_MAX];

        char q_depth_str[PAGE_SIZE / 4];
        char nr_thread_str[PAGE_SIZE / 4];
        char time_str[PAGE_SIZE / 4];
        char device_path[PAGE_SIZE / 4];

        char trace_repeat_str[PAGE_SIZE / 4];
        char wss_str[PAGE_SIZE / 4];
        char utilization_str[PAGE_SIZE / 4];
        char iosize_str[PAGE_SIZE / 4];

        assert(NULL != current);
        if (current) {
                memcpy(&info, current, sizeof(struct tr_info));
                assert(NULL != current->global_config);
                runner_config_free(current->global_config, RUNNER_FREE_ALL);
        }
        sprintf(filename, "%s_%d_%d_%s.txt", info.scheduler, info.ppid,
                info.weight, info.cgroup_id);
        sprintf(q_depth_str, "%u", info.q_depth);
        sprintf(nr_thread_str, "%u", info.nr_thread);
        sprintf(time_str, "%u", info.time);
        sprintf(device_path, "/dev/%s", info.device);

        sprintf(trace_repeat_str, "%u", info.trace_repeat);
        sprintf(wss_str, "%u", info.wss);
        sprintf(utilization_str, "%u", info.utilization);
        sprintf(iosize_str, "%u", info.iosize);

        pr_info(INFO, "trace replay save location: \"%s\"\n", filename);
        WAIT_PARENT();
#ifdef DEBUG
        tr_print_info(&info);
#endif
        if (-1 == access(info.trace_replay_path, F_OK)) {
                pr_info(ERROR, "trace replay doesn't exist: \"%s\"",
                        info.trace_replay_path);
                return -EACCES;
        }
        return execlp(info.trace_replay_path, info.trace_replay_path,
                      q_depth_str, nr_thread_str, filename, time_str,
                      trace_repeat_str, device_path, info.trace_data_path,
                      wss_str, utilization_str, iosize_str, (char *)0);
}

/**
 * @brief trace-replay를 실행시키도록 합니다.
 *
 * @return 정상적으로 동작이 된 경우 0을 그렇지 않은 경우 적절한 오류 번호를 반환합니다. 
 */
int tr_runner(void)
{
        int ret = 0;
        struct tr_info *current = global_info_head;
        char cmd[PATH_MAX];
        pid_t pid;

        snprintf(cmd, PATH_MAX, "rmdir /sys/fs/cgroup/blkio/%s*",
                 current->prefix_cgroup_name);
        pr_info(INFO, "Do command: \"%s\"\n", cmd);
        ret = system(cmd); /* 이 오류는 무시해도 상관없습니다. */
        if (ret) {
                pr_info(WARNING, "Deletion sequence ignore: \"%s\"\n", cmd);
        }

        snprintf(cmd, PATH_MAX, "echo %s >> /sys/block/%s/queue/scheduler",
                 current->scheduler, current->device);
        pr_info(INFO, "Do command: \"%s\"\n", cmd);
        ret = system(cmd);
        if (ret) {
                pr_info(ERROR, "Scheduler setting failed (scheduler: %s)\n",
                        current->scheduler);
                ret = -EINVAL;
                goto exit;
        }

        TELL_WAIT(); /* 동기화를 준비하는 과정입니다. */
        tr_info_list_traverse(current, global_info_head)
        {
                if (0 > (pid = fork())) {
                        pr_info(ERROR, "Fork failed. (pid: %d)\n", pid);
                        ret = -EFAULT;
                        goto exit;
                } else if (0 == pid) { /* 자식 프로세스 */
                        struct sigaction act;
                        memset(&act, 0, sizeof(act));
                        act.sa_handler = tr_kill_handle;
                        sigaction(SIGTERM, &act, NULL);
                        if (0 != (ret = tr_do_exec(current))) {
                                pr_info(ERROR,
                                        "Cannot execute program (errno: %d)\n",
                                        ret);
                                perror("Execution error detected");
                                tr_kill_handle(SIGKILL); /* execute 실패 */
                                _exit(EXIT_FAILURE);
                        }
                        pr_info(WARNING,
                                "Child process reaches the restriction area. (pid: %d)\n",
                                getpid());
                        tr_kill_handle(SIGTERM);
                        _exit(EXIT_SUCCESS);
                }

                /* 부모 프로세스 */
                current->pid = pid;
                tr_set_cgroup_state(current);

                /* IPC 객체를 생성합니다. */
                if (0 > (ret = tr_shm_init(current))) {
                        goto exit;
                }
                if (0 > (ret = tr_mq_init(current))) {
                        goto exit;
                }
        }

        /* 모든 자식을 깨우도록 합니다. */
        tr_info_list_traverse(current, global_info_head)
        {
                TELL_CHILD();
        }

exit:
        return ret;
}

/**
 * @brief trace-replay가 실행 중일 때의 정보를 반환합니다.
 *
 * @param key 임의의 cgroup_id에 해당합니다.
 * @param buffer 값이 반환되는 위치에 해당합니다.
 *
 * @return ret 정상적으로 종료되는 경우에는 log.type 정보가 반환되고, 그렇지 않은 경우 적절한 음수 값이 반환됩니다.
 * @note buffer를 재활용을 많이 하기 때문에 buffer의 내용은 반드시 미리 runner에서 할당이 되어 있어야 합니다.
 */
int tr_get_interval(const char *key, char *buffer)
{
        ENTRY query = { .key = NULL, .data = NULL };
        ENTRY *result = NULL;

        struct tr_info *info = NULL;
        struct realtime_log log = { 0 };

        int ret;

        strcpy(buffer, key);

        query.key = buffer;
        if (NULL == (result = hsearch(query, FIND))) {
                pr_info(ERROR, "Cannot find item (key: %s)\n", buffer);
                ret = -EINVAL;
                goto exit;
        }
        info = (struct tr_info *)result->data;
        if (NULL != info) {
                if (0 > (ret = tr_mq_get(info, (void *)&log))) {
                        goto exit;
                }
                tr_realtime_serializer(info, &log, buffer);
        } else {
                pr_info(ERROR, "`info` doesn't exist: %p\n", info);
                ret = -EACCES;
                goto exit;
        }

exit:
        ret = log.type;
        return ret;
}

/**
 * @brief trace-replay가 동작 완료한 후의 정보를 반환합니다.
 *
 * @param key 임의의 cgroup_id에 해당합니다.
 * @param buffer 값이 반환되는 위치에 해당합니다.
 *
 * @return 정상적으로 동작이 된 경우 0을 그렇지 않은 경우 적절한 오류 번호를 반환합니다. 
 */
int tr_get_total(const char *key, char *buffer)
{
        ENTRY query = { .key = NULL, .data = NULL };
        ENTRY *result = NULL;

        struct tr_info *info = NULL;
        struct total_results results = { 0 };

        int ret;

        strcpy(buffer, key);

        query.key = buffer;
        if (NULL == (result = hsearch(query, FIND))) {
                pr_info(ERROR, "Cannot find item (key: %s)\n", buffer);
                ret = -EINVAL;
                goto exit;
        }
        info = (struct tr_info *)result->data;
        if (NULL != info) {
                if (0 > (ret = tr_shm_get(info, (void *)&results))) {
                        goto exit;
                }
                tr_total_serializer(info, &results, buffer);
        } else {
                pr_info(ERROR, "`info` doesn't exist: %p\n", info);
                ret = -EACCES;
                goto exit;
        }

exit:
        return ret;
}

/**
 * @brief trace-replay 관련 동적 할당 정보를 해제합니다.
 */
void tr_free(void)
{
        __tr_free();
        hdestroy();
}
