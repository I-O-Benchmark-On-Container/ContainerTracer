/**< system header */
#include <stdlib.h>
#include <unity.h>
#include <time.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#define __s8

/**< external header */
#include <jemalloc/jemalloc.h>
#include <json.h>

/**< user header */
#include <generic.h>
#include <runner.h>
#include <trace_replay.h>

#define TRACE_REPLAY_PATH "./build/release/trace-replay"
#define TEST_DISK_PATH "sdb" /**< 대상 device의 이름입니다. /dev/ 포함 금지! */
#define SCHEDULER "none" /**< "none, bfq" in SCSI; "none, kyber, bfq" in NVMe */

#define TIME (5) /**< seconds */
#define Q_DEPTH (get_nprocs()) /**< CPU의 갯수 만큼 Q_DEPTH를 설정합니다. */
#define PREFIX_CGROUP_NAME "tester.trace." /**< cgroup 폴더 생성 규칙입니다. */

const char *key[] = { "cgroup-1", "cgroup-2", "cgroup-3",
                      "cgroup-4", "cgroup-5", "cgroup-6" };
const int weight[] = { 100, 250, 500, 1000, 2000, 4000 };
const char *test_path[] = {
        "./sample/sample1.dat", "./sample/sample2.dat",
        "./sample/sample1.dat", "rand_read",
        "rand_mixed",           "seq_mixed",
};
const unsigned long key_len = sizeof(key) / sizeof(char *);

#define NR_TASKS (key_len)
#define NR_THREAD ((int)(Q_DEPTH / NR_TASKS))

#define FLAGS_MASK (0x3F)

char *json, *task_options;
void setUp(void)
{
        unsigned long i;
        const struct runner_config *config = NULL;

        json = (char *)malloc(PAGE_SIZE * sizeof(char));
        task_options = (char *)malloc(PAGE_SIZE * sizeof(char));

        for (i = 0; i < key_len / 2; i++) {
                char temp[PAGE_SIZE] = { 0 };
                sprintf(temp,
                        "{\"cgroup_id\": \"%s\" , \"weight\":%d, \"trace_data_path\":\"%s\"},",
                        key[i], weight[i], test_path[i]);

                strcat(task_options, temp);
        }

        for (i = key_len / 2; i < key_len; i++) {
                char temp[PAGE_SIZE] = { 0 };
                sprintf(temp,
                        "{\"cgroup_id\": \"%s\" , \"weight\":%d, \"trace_data_path\":\"%s\","
                        "\"wss\": %d, \"utilization\": %d, \"iosize\": %d},",
                        key[i], weight[i], test_path[i], 128, 10, 4);

                strcat(task_options, temp);
        }

        sprintf(json,
                "{\"driver\":\"trace-replay\","
                "\"setting\": {"
                "\"trace_replay_path\":\"%s\","
                "\"device\": \"%s\","
                "\"nr_tasks\": %lu,"
                "\"time\": %d, "

                "\"q_depth\": %d,"
                "\"nr_thread\": %d,"

                "\"prefix_cgroup_name\": \"%s\", "
                "\"scheduler\": \"%s\", "
                "\"task_option\": [ %s"
                "],"
                "}"
                "}",
                TRACE_REPLAY_PATH, TEST_DISK_PATH, NR_TASKS, TIME, Q_DEPTH,
                NR_THREAD, PREFIX_CGROUP_NAME, SCHEDULER, task_options);
        pr_info(INFO, "Current JSON: %s\n", json);
        TEST_ASSERT_EQUAL(0, runner_init(json)); /**< config 설정 과정 */
        TEST_ASSERT_NOT_NULL(config = runner_get_global_config());
        TEST_ASSERT_EQUAL_STRING(config->driver, "trace-replay");
        TEST_ASSERT_EQUAL(TRACE_REPLAY_DRIVER,
                          get_generic_driver_index(config->driver));
        TEST_ASSERT_EQUAL(0, runner_run()); /**< trace-replay 실행 시점 */
}

void tearDown(void)
{
        runner_free();

        free(json);
        free(task_options);
        json = task_options = NULL;
}

void test(void)
{
        char *buffer;
        int flags = 0x0;
        while (1) {
                pr_info(INFO, "key: 0x%X\n", flags);
                if (flags == FLAGS_MASK) {
                        break;
                }
                /**< 전체 키의 내용을 읽습니다. */
                for (unsigned long i = 0; i < key_len; i++) {
                        struct json_object *object, *tmp;
                        int type;
                        if (flags == FLAGS_MASK) {
                                break;
                        }

                        buffer = runner_get_interval_result(key[i]);
                        TEST_ASSERT_NOT_NULL(buffer);

                        pr_info(INFO, "Current log = %s\n", buffer);
                        object = json_tokener_parse(buffer);
                        TEST_ASSERT_EQUAL(TRUE, json_object_object_get_ex(
                                                        object, "data", &tmp));
                        TEST_ASSERT_EQUAL(TRUE, json_object_object_get_ex(
                                                        tmp, "type", &tmp));
                        type = json_object_get_int(tmp);
                        pr_info(INFO, "Current log.type = %d (target = %d)\n",
                                type, FIN);

                        /**< FIN 명령을 받으면 종료합니다. */
                        if (FIN == type) {
                                pr_info(INFO, "last message: %s\n", buffer);
                                runner_put_result_string(buffer);
                                json_object_put(object);
                                flags |= (0x1 << i);
                                continue;
                        }

                        pr_info(INFO, "interval: %s\n", buffer);
                        runner_put_result_string(buffer);
                        json_object_put(object);
                }
        }

        /**< 키의 수만큼 실행됩니다. */
        for (unsigned long i = 0; i < key_len; i++) {
                buffer = runner_get_total_result(key[i]);
                TEST_ASSERT_NOT_NULL(buffer);
                pr_info(INFO, "total: %s\n", buffer);
                runner_put_result_string(buffer);
        }
}

int main(void)
{
        char ch;
        printf("Performing this test can completely erase\n"
               "the contents of your /dev/" TEST_DISK_PATH "/ disk.\n"
               "Nevertheless, will you proceed?(y/n)");
        scanf("%c", &ch);
        if (ch == 'y') {
                UNITY_BEGIN();

                RUN_TEST(test);

                return UNITY_END();
        }
        printf("Exit test...\n");
        return 0;
}
