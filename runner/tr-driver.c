/**
 * @file tr-driver.c
 * @brief trace-replay를 동작시키는 driver 구현부에 해당합니다.
 * @author BlaCkinkGJ (ss5kijun@gmail.com)
 * @version 0.1
 * @date 2020-08-05
 */

/**< system header */
#include <stdlib.h>
#include <errno.h>
#include <search.h>
#include <assert.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

/**< external header */
#include <json.h>
#include <jemalloc/jemalloc.h>

/**< user header */
#include <generic.h>
#include <runner.h>
#include <tr-driver.h>
#include <log.h>
#include <sync.h>
#include <trace_replay.h>

enum { TR_NONE_SCHEDULER = 0,
       TR_KYBER_SCHEDULER,
       TR_BFQ_SCHEDULER,
};

static const char *tr_valid_scheduler[] = {
        "none",
        "kyber",
        "bfq",
        NULL,
}; /**< 현재 드라이버에서 사용 가능한 스케쥴러가 들어갑니다. */

static char *global_program_path =
        NULL; /**< trace-replay 실행 파일의 경로가 들어갑니다. */
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

#ifdef TR_DEBUG
/**
 * @brief 현재 info의 내용을 출력합니다. 
 *
 * @param info 출력하고자 하는 info의 포인터에 해당합니다.
 */
static void tr_debug(const struct tr_info *info)
{
        pr_info(INFO,
                "\n"
                "\t\t[[ current %p ]]\n"
                "\t\tpid: %d\n"
                "\t\ttime: %u\n"
                "\t\tq_depth: %u\n"
                "\t\tnr_thread: %u\n"
                "\t\tweight: %u\n"
                "\t\tqid: %d\n"
                "\t\tshmid: %d\n"
                "\t\tsemid: %d\n"
                "\t\tprefix_cgroup_name: %s\n"
                "\t\tscheduler: %s\n"
                "\t\tname: %s\n"
                "\t\ttrace_data_path: %s\n"
                "\t\tdevice: %s\n"
                "\t\tglobal_config: %p\n"
                "\t\tnext: %p\n",
                info, info->pid, info->time, info->q_depth, info->nr_thread,
                info->weight, info->qid, info->shmid, info->semid,
                info->prefix_cgroup_name, info->scheduler, info->cgroup_id,
                info->trace_data_path, info->device, info->global_config,
                info->next);
}
#endif

/**
 * @brief 실질적으로 trace-replay 관련 구조체의 동적 할당된 내용을 해제하는 부분에 해당합니다.
 * @ref http://www.ascii-art.de/ascii/def/dr_who.txt
 */
static void __tr_free(void)
{
        if (NULL != global_program_path) {
                free(global_program_path);
                global_program_path = NULL;
        }

        while (global_info_head != NULL) {
                struct tr_info *current = global_info_head;
                struct tr_info *next = global_info_head->next;

                if (getpid() == current->ppid &&
                    0 != current->pid) { /**< 자식 프로세스를 죽입니다. */
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
                }

                pr_info(INFO, "Delete target %p\n", current);
                free(current);

                global_info_head = next;
        }
        pr_info(INFO, "Do trace-replay free success ==> %p\n",
                global_info_head);
}

/**
 * @brief 정수 형태의 `info->(member)`에 json에서 값을 읽어서 값을 주도록 합니다. 
 *
 * @param setting 탐색을 할 특정 위치를 지칭합니다.
 * @param key json에서 가져오고자 하는 데이터의 key 또는 field에 해당합니다.
 * @param member 실제 값이 삽입되는 위치에 해당합니다.
 * @param is_print 에러를 사용자에게 출력을 할 지 말지를 선택합니다.
 *
 * @return 성공적으로 입력이 되었다면 0이 반환되고, 그렇지 않은 경우에는 -EINVAL이 반환됩니다.
 */
static int tr_info_int_value_set(struct json_object *setting, const char *key,
                                 unsigned int *member, int is_print)
{
        struct json_object *tmp = NULL;
        if (!json_object_object_get_ex(setting, key, &tmp)) {
                if (TR_ERROR_PRINT == is_print) {
                        pr_info(ERROR, "Not exist error (key: %s)\n", key);
                }
                return -EINVAL;
        }
        *member = json_object_get_int(tmp);
        return 0;
}

/**
 * @brief 문자열 형태의 `info->(member)`에 json에서 값을 읽어서 값을 주도록 합니다. 
 *
 * @param setting 탐색을 할 특정 위치를 지칭합니다.
 * @param key json에서 가져오고자 하는 데이터의 key 또는 field에 해당합니다.
 * @param member 실제 값이 삽입되는 위치에 해당합니다.
 * @param is_print 에러를 사용자에게 출력을 할 지 말지를 선택합니다.
 *
 * @return 성공적으로 입력이 되었다면 0이 반환되고, 그렇지 않은 경우에는 -EINVAL이 반환됩니다.
 */
static int tr_info_str_value_set(struct json_object *setting, const char *key,
                                 char *member, int is_print)
{
        struct json_object *tmp = NULL;
        if (!json_object_object_get_ex(setting, key, &tmp)) {
                if (TR_ERROR_PRINT == is_print) {
                        pr_info(ERROR, "Not exist error (key: %s)\n", key);
                }
                return -EINVAL;
        }
        strcpy(member, json_object_get_string(tmp));
        generic_strip_string(member, '\"');
        return 0;
}

/**
 * @brief 현재 입력되는 scheduler가 driver가 지원하는 지를 확인하도록 합니다.
 *
 * @param scheduler 검사하고자 하는 스케쥴러 문자열을 가진 문자열 포인터입니다.
 *
 * @return 가지고 있는 경우 0이 반환되고, 그렇지 않은 경우 -EINVAL이 반환됩니다.
 */
static int valid_scheduler_test(const char *scheduler)
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
 * @brief 각각의 프로세스의 동작 옵션을 설정을 해주도록 합니다.
 *
 * @param setting 설정 값을 가지고 있는 json 객체의 포인터에 해당합니다.
 * @param index json의 task_option의 배열 순번에 대한 정보를 가집니다.
 * @param info 실제 값이 들어가는 위치에 해당합니다.
 *
 * @return 정상적으로 초기화가 된 경우 0을 그렇지 않은 경우 적절한 오류 번호를 반환합니다. 
 */
static int __tr_info_init(struct json_object *setting, int index,
                          struct tr_info *info)
{
        struct json_object *tmp;
        int ret = 0;

        ENTRY item; /**< hsearch를 위해서 사용되는 변수입니다. */
        ENTRY *result;

        assert(NULL != info);

        if (!json_object_object_get_ex(setting, "task_option", &tmp)) {
                pr_info(ERROR, "Not exist error (key: %s)\n", "task_option");
                ret = -EINVAL;
                goto exception;
        }

        tmp = json_object_array_get_idx(tmp, index);
        if (NULL == tmp) {
                pr_info(ERROR, "Array out-of-bound error (index: %d)\n", index);
                ret = -EINVAL;
                goto exception;
        }

        tr_info_int_value_set(tmp, "time", &info->time, TR_PRINT_NONE);
        tr_info_int_value_set(tmp, "q_depth", &info->q_depth, TR_PRINT_NONE);
        tr_info_int_value_set(tmp, "nr_thread", &info->nr_thread,
                              TR_PRINT_NONE);
        tr_info_str_value_set(tmp, "prefix_cgroup_name",
                              info->prefix_cgroup_name, TR_PRINT_NONE);
        tr_info_str_value_set(tmp, "scheduler", info->scheduler, TR_PRINT_NONE);
        tr_info_str_value_set(tmp, "trace_replay_path", info->trace_replay_path,
                              TR_PRINT_NONE);
        tr_info_str_value_set(tmp, "device", info->device, TR_PRINT_NONE);
        ret = valid_scheduler_test(info->scheduler);
        if (0 != ret) {
                pr_info(ERROR, "Unsupported scheduler (name: %s)\n",
                        info->scheduler);
                goto exception;
        }

        ret = tr_info_int_value_set(tmp, "weight", &info->weight,
                                    TR_ERROR_PRINT);
        if (0 != ret) {
                goto exception;
        }

        ret = tr_info_str_value_set(tmp, "trace_data_path",
                                    info->trace_data_path, TR_ERROR_PRINT);
        if (0 != ret) {
                goto exception;
        }

        ret = tr_info_str_value_set(tmp, "cgroup_id", info->cgroup_id,
                                    TR_ERROR_PRINT);
        if (0 != ret) {
                goto exception;
        }

        if (-1 == (ret = access(info->trace_data_path, F_OK))) {
                pr_info(ERROR, "Trace data file not exist: %s\n",
                        info->trace_data_path);
                goto exception;
        } else {
                pr_info(INFO, "Trace data file exist: %s\n",
                        info->trace_data_path);
        }
        info->ppid = getpid();

        item.key = info->cgroup_id;
        if (NULL != (result = hsearch(item, FIND))) {
                pr_info(ERROR, "Duplicate c-group name detected (name: %s)\n",
                        result->key);
                ret = -EINVAL;
                goto exception;
        }

        item.key = info->cgroup_id;
        item.data = info;
        hsearch(item, ENTER);

exception:
        return ret;
}

/**
 * @brief 각 프로세스에 들어가는 info 객체를 __생성__하고 구축하여 반환합니다.
 *
 * @param setting 설정 값을 가지고 있는 json 객체의 포인터에 해당합니다.
 * @param index json의 task_option의 배열 순번에 대한 정보를 가집니다.
 *
 * @return 정상적으로 초기화가 된 경우 0을 그렇지 않은 경우 적절한 오류 번호를 반환합니다. 
 */
static struct tr_info *tr_info_init(struct json_object *setting, int index)
{
        struct tr_info *info;
        int ret = 0;

        info = (struct tr_info *)malloc(sizeof(struct tr_info));
        if (!info) {
                pr_info(ERROR, "Memory allocation fail (name: %s)\n", "info");
                ret = -ENOMEM;
                goto exception;
        }

        memset(info, 0, sizeof(struct tr_info));

        ret |= tr_info_int_value_set(setting, "time", &info->time,
                                     TR_ERROR_PRINT);
        ret |= tr_info_int_value_set(setting, "q_depth", &info->q_depth,
                                     TR_ERROR_PRINT);
        ret |= tr_info_int_value_set(setting, "nr_thread", &info->nr_thread,
                                     TR_ERROR_PRINT);
        ret |= tr_info_str_value_set(setting, "prefix_cgroup_name",
                                     info->prefix_cgroup_name, TR_ERROR_PRINT);
        ret |= tr_info_str_value_set(setting, "scheduler", info->scheduler,
                                     TR_ERROR_PRINT);
        ret |= tr_info_str_value_set(setting, "device", info->device,
                                     TR_ERROR_PRINT);
        ret |= tr_info_str_value_set(setting, "trace_replay_path",
                                     info->trace_replay_path, TR_ERROR_PRINT);
        if (0 != ret) {
                pr_info(ERROR, "error detected (errno: %d)\n", ret);
                goto exception;
        }

        ret = valid_scheduler_test(info->scheduler);
        if (0 != ret) {
                pr_info(ERROR, "Unsupported scheduler (name: %s)\n",
                        info->scheduler);
                goto exception;
        }

        ret = __tr_info_init(setting, index, info);
        if (0 != ret) {
                pr_info(ERROR, "error detected (errno: %d)\n", ret);
                goto exception;
        }

        info->next = NULL;

        return info;
exception:
        if (NULL != info) {
                free(info);
                info = NULL;
        }
        return info;
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

        global_program_path = (char *)malloc(sizeof(char) * PATH_MAX);
        if (!global_program_path) {
                pr_info(ERROR, "Memory allocation fail (name: %s)\n",
                        "global_program_path");
                ret = -ENOMEM;
                goto exception;
        }

        if (!json_object_object_get_ex(setting, "trace_replay_path", &tmp)) {
                pr_info(ERROR, "Not exist error (key: %s)\n",
                        "trace_replay_path");
                ret = -EACCES;
                goto exception;
        }
        strcpy(global_program_path, json_object_get_string(tmp));
        pr_info(INFO, "Trace-replay path: %s\n", global_program_path);
        if (-1 == (ret = access(global_program_path, F_OK))) {
                pr_info(ERROR, "trace-replay not exist: %s\n",
                        global_program_path);
                goto exception;
        } else {
                pr_info(INFO, "trace-replay exist: %s\n", global_program_path);
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
                if (!current) { /**< 할당 실패가 벌어진 경우 */
                        ret = -ENOMEM;
                        goto exception;
                }
                current->global_config = object;
                if (!current->global_config) {
                        ret = -EFAULT;
                        goto exception;
                }

                if (global_info_head == NULL) { /**< 초기화 과정 */
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

        /**< cgroup을 생성하는 과정에 해당합니다. */
        snprintf(cmd, PATH_MAX, "mkdir /sys/fs/cgroup/blkio/%s%d",
                 current->prefix_cgroup_name, current->pid);
        pr_info(INFO, "Do command: \"%s\"\n", cmd);
        ret = system(cmd);
        if (ret) {
                pr_info(ERROR, "Cannot create the directory: \"%s\"\n", cmd);
                return -EINVAL;
        }

        ret = strcmp(current->scheduler, tr_valid_scheduler[TR_NONE_SCHEDULER]);
        if (ret != 0) { /**< None 스케줄러가 아니면 weight를 설정해줍니다. */
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

int tr_do_exec(struct tr_info *info)
{
        char filename[PATH_MAX];
        WAIT_PARENT();
        snprintf(filename, PATH_MAX, "%s_%d_%d_%s.txt", info->scheduler,
                 info->ppid, info->weight, info->cgroup_id);
        pr_info(INFO, "trace replay save location: \"%s\"\n", filename);
        return 0;
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
        system(cmd); /**< 이 오류는 무시해도 상관없습니다. */

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

        TELL_WAIT(); /**< 동기화를 준비하는 과정입니다. */
        tr_info_list_traverse(current, global_info_head)
        {
#ifdef TR_DEBUG
                tr_debug(current);
#endif
                if (0 > (pid = fork())) {
                        pr_info(ERROR, "Fork failed. (pid: %d)\n", pid);
                        ret = -EFAULT;
                        goto exit;
                } else if (0 == pid) { /**< 자식 프로세스 */
                        struct sigaction act;
                        memset(&act, 0, sizeof(act));
                        act.sa_handler = tr_kill_handle;
                        sigaction(SIGTERM, &act, NULL);
                        if (0 != (ret = tr_do_exec(current))) {
                                tr_kill_handle(SIGKILL); /**< execute 실패 */
                                pr_info(ERROR,
                                        "Cannot execute program (errno: %d)\n",
                                        ret);
                                _exit(EXIT_FAILURE);
                        }
                        tr_kill_handle(SIGTERM);
                        _exit(EXIT_SUCCESS);
                }
                /**< 부모 프로세스 */
                current->pid = pid;
                tr_set_cgroup_state(current);
        }

        /**< 모든 자식을 깨우도록 합니다. */
        tr_info_list_traverse(current, global_info_head)
        {
                TELL_CHILD();
        }

exit:
        return ret;
}

/**
 * @brief json으로 구조체에 있는 내용을 변환합니다.
 *
 * @param info 현재 tr_info 구조체의 포인터입니다.
 * @param log 현재 출력 로그에 해당합니다.
 * @param buffer 수행 결과 만들어진 json 내용이 들어가는 부분에 해당합니다.
 *
 * @warning buffer는 반드시 이 함수를 부르기 전에 할당되어야하며, 그 크기는 INTERVAL_RESULT_STRING_SIZE 보다 커야 합니다.
 * @todo 리팩토링이 필요합니다.
 */
static void tr_realtime_serializer(const struct tr_info *info,
                                   struct realtime_log *log, char *buffer)
{
        struct json_object *object, *meta, *data;
        assert(NULL != info);
        assert(NULL != log);
        assert(NULL != buffer);

        object = json_object_new_object();

        meta = json_object_new_object();
        json_object_object_add(meta, "pid", json_object_new_int(info->pid));
        json_object_object_add(meta, "time", json_object_new_int(info->time));
        json_object_object_add(meta, "q_depth",
                               json_object_new_int(info->q_depth));
        json_object_object_add(meta, "nr_thread",
                               json_object_new_int(info->nr_thread));
        json_object_object_add(meta, "weight",
                               json_object_new_int(info->weight));
        json_object_object_add(meta, "qid", json_object_new_int(info->qid));
        json_object_object_add(meta, "shmid", json_object_new_int(info->shmid));
        json_object_object_add(meta, "semid", json_object_new_int(info->semid));
        json_object_object_add(
                meta, "prefix_cgroup_name",
                json_object_new_string(info->prefix_cgroup_name));
        json_object_object_add(meta, "scheduler",
                               json_object_new_string(info->scheduler));
        json_object_object_add(meta, "cgroup_id",
                               json_object_new_string(info->cgroup_id));
        json_object_object_add(meta, "trace_data_path",
                               json_object_new_string(info->trace_data_path));
        json_object_object_add(meta, "device", json_object_new_int(info->pid));
        json_object_object_add(object, "meta", meta);

        data = json_object_new_object();
        json_object_object_add(data, "type", json_object_new_int(log->type));
        json_object_object_add(data, "time", json_object_new_double(log->time));
        json_object_object_add(data, "remaining",
                               json_object_new_double(log->remaining));
        json_object_object_add(
                data, "remaining_percentage",
                json_object_new_double(log->remaining_percentage));
        json_object_object_add(data, "avg_bw",
                               json_object_new_double(log->avg_bw));
        json_object_object_add(data, "cur_bw",
                               json_object_new_double(log->cur_bw));
        json_object_object_add(data, "lat", json_object_new_double(log->lat));
        json_object_object_add(data, "time_diff",
                               json_object_new_double(log->time_diff));
        json_object_object_add(object, "data", data);

        strncpy(buffer, json_object_to_json_string(object),
                INTERVAL_RESULT_STRING_SIZE);

        json_object_put(object);
}

/**
 * @brief json으로 구조체에 있는 내용을 변환합니다.
 *
 * @param info 현재 tr_info 구조체의 포인터입니다.
 * @param results 현재 info에 해당하는 전체 내용에 해당합니다.
 * @param buffer 수행 결과 만들어진 json 내용이 들어가는 부분에 해당합니다.
 *
 * @warning buffer는 반드시 이 함수를 부르기 전에 할당되어야하며, 그 크기는 TOTAL_RESULT_STRING_SIZE 보다 커야 합니다.
 * @todo 리팩토링이 필요합니다.
 */
static void tr_total_serializer(const struct tr_info *info,
                                struct total_results *total, char *buffer)
{
        struct json_object *object;
        struct json_object *meta;
        struct json_object *total_results, *config, *results;
        struct json_object *traces, **trace;
        struct json_object *per_traces, **per_trace;
        struct json_object **synthetic, **stats;
        struct json_object *aggr_result;

        int i = 0;

        trace = (struct json_object **)malloc(sizeof(struct json_object *) *
                                              MAX_THREADS);
        assert(NULL != trace);
        per_trace = (struct json_object **)malloc(sizeof(struct json_object *) *
                                                  MAX_THREADS);
        assert(NULL != per_trace);
        synthetic = (struct json_object **)malloc(sizeof(struct json_object *) *
                                                  MAX_THREADS);
        assert(NULL != synthetic);
        stats = (struct json_object **)malloc(sizeof(struct json_object *) *
                                              MAX_THREADS);
        assert(NULL != stats);

        assert(NULL != info);
        assert(NULL != total);
        assert(NULL != buffer);

        object = json_object_new_object();

        /**< info 내용을 meta에 집어넣습니다. */
        meta = json_object_new_object();
        json_object_object_add(meta, "pid", json_object_new_int(info->pid));
        json_object_object_add(meta, "time", json_object_new_int(info->time));
        json_object_object_add(meta, "q_depth",
                               json_object_new_int(info->q_depth));
        json_object_object_add(meta, "nr_thread",
                               json_object_new_int(info->nr_thread));
        json_object_object_add(meta, "weight",
                               json_object_new_int(info->weight));
        json_object_object_add(meta, "qid", json_object_new_int(info->qid));
        json_object_object_add(meta, "shmid", json_object_new_int(info->shmid));
        json_object_object_add(meta, "semid", json_object_new_int(info->semid));
        json_object_object_add(
                meta, "prefix_cgroup_name",
                json_object_new_string(info->prefix_cgroup_name));
        json_object_object_add(meta, "scheduler",
                               json_object_new_string(info->scheduler));
        json_object_object_add(meta, "cgroup_id",
                               json_object_new_string(info->cgroup_id));
        json_object_object_add(meta, "trace_data_path",
                               json_object_new_string(info->trace_data_path));
        json_object_object_add(meta, "device", json_object_new_int(info->pid));

        json_object_object_add(object, "meta", meta);

        /**< total 정보를 기입합니다. */
        total_results = json_object_new_object();

        /**< config 정보 설정 */
        config = json_object_new_object();
        json_object_object_add(config, "qdepth",
                               json_object_new_int(total->config.qdepth));
        json_object_object_add(config, "timeout",
                               json_object_new_double(total->config.timeout));
        json_object_object_add(config, "nr_trace",
                               json_object_new_int(total->config.nr_trace));
        json_object_object_add(config, "nr_thread",
                               json_object_new_int(total->config.nr_thread));
        json_object_object_add(config, "per_thread",
                               json_object_new_int(total->config.per_thread));
        json_object_object_add(
                config, "result_file",
                json_object_new_string(total->config.result_file));

        traces = json_object_new_array();

        for (i = 0; i < total->config.nr_thread; i++) {
                struct json_object *_trace;
                _trace = json_object_new_object();
                json_object_object_add(
                        _trace, "start_partition",
                        json_object_new_double(
                                total->config.traces[i].start_partition));
                json_object_object_add(
                        _trace, "total_size",
                        json_object_new_double(
                                total->config.traces[i].total_size));
                json_object_object_add(
                        _trace, "start_page",
                        json_object_new_int64(
                                total->config.traces[i].start_page));
                json_object_object_add(
                        _trace, "total_pages",
                        json_object_new_int64(
                                total->config.traces[i].total_pages));
                trace[i] = _trace;
                json_object_array_add(traces, trace[i]);
        }
        json_object_object_add(config, "traces", traces);
        json_object_object_add(total_results, "config", config);

        /**< result 정보 설정 */
        results = json_object_new_object();

        per_traces = json_object_new_array();
        for (i = 0; i < total->config.nr_thread; i++) {
                struct json_object *_per_trace;
                _per_trace = json_object_new_object();
                json_object_object_add(
                        _per_trace, "name",
                        json_object_new_string(
                                total->results.per_trace[i].name));
                json_object_object_add(
                        _per_trace, "issynthetic",
                        json_object_new_int(
                                total->results.per_trace[i].issynthetic));
                synthetic[i] = json_object_new_object();
                json_object_object_add(
                        synthetic[i], "working_set_size",
                        json_object_new_int(
                                total->results.per_trace[i]
                                        .synthetic.working_set_size));
                json_object_object_add(
                        synthetic[i], "utilization",
                        json_object_new_int(total->results.per_trace[i]
                                                    .synthetic.utilization));
                json_object_object_add(
                        synthetic[i], "touched_working_set_size",
                        json_object_new_int(
                                total->results.per_trace[i]
                                        .synthetic.touched_working_set_size));
                json_object_object_add(
                        synthetic[i], "io_size",
                        json_object_new_int(
                                total->results.per_trace[i].synthetic.io_size));

                json_object_object_add(_per_trace, "synthetic", synthetic[i]);

                stats[i] = json_object_new_object();
                json_object_object_add(
                        stats[i], "exec_time",
                        json_object_new_double(
                                total->results.per_trace[i].stats.exec_time));
                json_object_object_add(
                        stats[i], "avg_lat",
                        json_object_new_double(
                                total->results.per_trace[i].stats.avg_lat));
                json_object_object_add(
                        stats[i], "avg_lat_var",
                        json_object_new_double(
                                total->results.per_trace[i].stats.avg_lat_var));
                json_object_object_add(
                        stats[i], "lat_min",
                        json_object_new_double(
                                total->results.per_trace[i].stats.lat_min));
                json_object_object_add(
                        stats[i], "lat_max",
                        json_object_new_double(
                                total->results.per_trace[i].stats.lat_max));
                json_object_object_add(
                        stats[i], "iops",
                        json_object_new_double(
                                total->results.per_trace[i].stats.iops));
                json_object_object_add(
                        stats[i], "total_bw",
                        json_object_new_double(
                                total->results.per_trace[i].stats.total_bw));
                json_object_object_add(
                        stats[i], "read_bw",
                        json_object_new_double(
                                total->results.per_trace[i].stats.read_bw));
                json_object_object_add(
                        stats[i], "write_bw",
                        json_object_new_double(
                                total->results.per_trace[i].stats.write_bw));
                json_object_object_add(
                        stats[i], "total_traffic",
                        json_object_new_double(total->results.per_trace[i]
                                                       .stats.total_traffic));
                json_object_object_add(
                        stats[i], "read_traffic",
                        json_object_new_double(
                                total->results.per_trace[i].stats.read_traffic));
                json_object_object_add(
                        stats[i], "write_traffic",
                        json_object_new_double(total->results.per_trace[i]
                                                       .stats.write_traffic));
                json_object_object_add(
                        stats[i], "read_ratio",
                        json_object_new_double(
                                total->results.per_trace[i].stats.read_ratio));
                json_object_object_add(
                        stats[i], "total_avg_req_size",
                        json_object_new_double(
                                total->results.per_trace[i]
                                        .stats.total_avg_req_size));
                json_object_object_add(
                        stats[i], "read_avg_req_size",
                        json_object_new_double(
                                total->results.per_trace[i]
                                        .stats.read_avg_req_size));
                json_object_object_add(
                        stats[i], "write_avg_req_size",
                        json_object_new_double(
                                total->results.per_trace[i]
                                        .stats.write_avg_req_size));
                json_object_object_add(_per_trace, "stats", stats[i]);

                json_object_object_add(
                        _per_trace, "trace_reset_count",
                        json_object_new_int(
                                total->results.per_trace[i].trace_reset_count));
                per_trace[i] = _per_trace;
                json_object_array_add(per_traces, per_trace[i]);
        }
        json_object_object_add(results, "per_trace", per_traces);

        aggr_result = json_object_new_object();
        json_object_object_add(
                aggr_result, "exec_time",
                json_object_new_double(
                        total->results.aggr_result.stats.exec_time));
        json_object_object_add(
                aggr_result, "avg_lat",
                json_object_new_double(
                        total->results.aggr_result.stats.avg_lat));
        json_object_object_add(
                aggr_result, "avg_lat_var",
                json_object_new_double(
                        total->results.aggr_result.stats.avg_lat_var));
        json_object_object_add(
                aggr_result, "lat_min",
                json_object_new_double(
                        total->results.aggr_result.stats.lat_min));
        json_object_object_add(
                aggr_result, "lat_max",
                json_object_new_double(
                        total->results.aggr_result.stats.lat_max));
        json_object_object_add(
                aggr_result, "iops",
                json_object_new_double(total->results.aggr_result.stats.iops));
        json_object_object_add(
                aggr_result, "total_bw",
                json_object_new_double(
                        total->results.aggr_result.stats.total_bw));
        json_object_object_add(
                aggr_result, "read_bw",
                json_object_new_double(
                        total->results.aggr_result.stats.read_bw));
        json_object_object_add(
                aggr_result, "write_bw",
                json_object_new_double(
                        total->results.aggr_result.stats.write_bw));
        json_object_object_add(
                aggr_result, "total_traffic",
                json_object_new_double(
                        total->results.aggr_result.stats.total_traffic));
        json_object_object_add(
                aggr_result, "read_traffic",
                json_object_new_double(
                        total->results.aggr_result.stats.read_traffic));
        json_object_object_add(
                aggr_result, "write_traffic",
                json_object_new_double(
                        total->results.aggr_result.stats.write_traffic));
        json_object_object_add(
                aggr_result, "read_ratio",
                json_object_new_double(
                        total->results.aggr_result.stats.read_ratio));
        json_object_object_add(
                aggr_result, "total_avg_req_size",
                json_object_new_double(
                        total->results.aggr_result.stats.total_avg_req_size));
        json_object_object_add(
                aggr_result, "read_avg_req_size",
                json_object_new_double(
                        total->results.aggr_result.stats.read_avg_req_size));
        json_object_object_add(
                aggr_result, "write_avg_req_size",
                json_object_new_double(
                        total->results.aggr_result.stats.write_avg_req_size));
        json_object_object_add(results, "aggr_result", aggr_result);

        json_object_object_add(total_results, "results", results);
        json_object_object_add(object, "data", total_results);

        strncpy(buffer, json_object_to_json_string(object),
                TOTAL_RESULT_STRING_SIZE);

        json_object_put(object);

        free(trace);
        free(per_trace);
        free(synthetic);
        free(stats);
}
/**
 * @brief trace-replay가 실행 중일 때의 정보를 반환합니다.
 *
 * @param key 임의의 cgroup_id에 해당합니다.
 * @param buffer 값이 반환되는 위치에 해당합니다.
 *
 * @return 정상적으로 동작이 된 경우 0을 그렇지 않은 경우 적절한 오류 번호를 반환합니다. 
 * @note buffer를 재활용을 많이 하기 때문에 buffer의 내용은 반드시 미리 runner에서 할당이 되어 있어야 합니다.
 */
int tr_get_interval(const char *key, char *buffer)
{
        ENTRY query = { .key = NULL, .data = NULL };
        ENTRY *result = NULL;

        struct tr_info *info = NULL;

        strncpy(buffer, key, INTERVAL_RESULT_STRING_SIZE);

        query.key = buffer;
        if (NULL == (result = hsearch(query, FIND))) {
                pr_info(ERROR, "Cannot find item (key: %s)\n", buffer);
                return -EINVAL;
        }
        info = (struct tr_info *)result->data;
        if (NULL != info) {
                struct realtime_log log = { 0 }; /**< TODO: 반드시 삭제하세요. */
                tr_realtime_serializer(info, &log, buffer);
        }

        return 0;
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

        strncpy(buffer, key, TOTAL_RESULT_STRING_SIZE);

        query.key = buffer;
        if (NULL == (result = hsearch(query, FIND))) {
                pr_info(ERROR, "Cannot find item (key: %s)\n", buffer);
                return -EINVAL;
        }
        info = (struct tr_info *)result->data;
        if (NULL != info) {
                struct total_results results = {
                        0
                }; /**< TODO: 반드시 삭제하세요. */
                results.config.nr_thread = 10;
                tr_total_serializer(info, &results, buffer);
        }

        return 0;
}

/**
 * @brief trace-replay 관련 동적 할당 정보를 해제합니다.
 */
void tr_free(void)
{
        __tr_free();
        hdestroy();
}
