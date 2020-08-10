/**< system header */
#include <stdlib.h>
#include <unity.h>
#include <time.h>
#include <unistd.h>

/**< external header */
#include <jemalloc/jemalloc.h>
#include <json.h>

/**< user header */
#include <generic.h>
#include <runner.h>
#include <trace_replay.h>

#define TEST_DISK_PATH "sdb"
void setUp(void)
{
        const char *json = "{\"driver\":\"trace-replay\", \
				  \"setting\": { \
				    \"trace_replay_path\":\"./build/release/trace-replay\", \
					\"device\": \"" TEST_DISK_PATH "\", \
					\"nr_tasks\": 4, \
					\"time\": 10, \
					\
					\"q_depth\": 32, \
					\"nr_thread\": 4, \
					\
				    \"prefix_cgroup_name\": \"tester.trace.\", \
					\"scheduler\": \"bfq\", \
					\"task_option\": [ {\"cgroup_id\": \"cgroup-1\" , \"weight\":100, \"trace_data_path\":\"./sample/sample1.dat\"}, {\"cgroup_id\": \"cgroup-2\" ,\"weight\":250, \"trace_data_path\":\"./sample/sample2.dat\"}, {\"cgroup_id\": \"cgroup-3\" ,\"weight\":500, \"trace_data_path\":\"./sample/sample1.dat\"}, {\"cgroup_id\": \"cgroup-4\" ,\"weight\":1000, \"trace_data_path\":\"./sample/sample2.dat\"}],\
					} \
				}";
        const struct runner_config *config = NULL;
        TEST_ASSERT_EQUAL(0, runner_init(json));
        TEST_ASSERT_NOT_NULL(config = runner_get_global_config());
        TEST_ASSERT_EQUAL_STRING(config->driver, "trace-replay");
        TEST_ASSERT_EQUAL(TRACE_REPLAY_DRIVER,
                          get_generic_driver_index(config->driver));
        TEST_ASSERT_EQUAL(0, runner_run());
}

void tearDown(void)
{
        runner_free();
}

void test(void)
{
        const char *key[] = { "cgroup-1", "cgroup-2", "cgroup-3", "cgroup-4" };

        char *buffer;
        int flags = 0x0;
        while (1) {
                pr_info(INFO, "key: 0x%X\n", flags);
                if (flags == 0xF) {
                        break;
                }
                for (int i = 0; i < 4; i++) {
                        struct json_object *object, *tmp;
                        int type;
                        if (flags == 0xF) {
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
                usleep(100);
        }
        for (int i = 0; i < 4; i++) {
                buffer = runner_get_total_result(key[i]);
                TEST_ASSERT_NOT_NULL(buffer);
                pr_info(INFO, "total: %s\n", buffer);
                runner_put_result_string(buffer);
        }
}

int main(void)
{
        printf("Performing this test can completely erase the contents of your /dev/" TEST_DISK_PATH
               "/ disk. Nevertheless, will you proceed?(y/n)");
        char ch = getchar();
        if (ch == 'y') {
                UNITY_BEGIN();

                RUN_TEST(test);

                return UNITY_END();
        }
        printf("Exit test...\n");
        return 0;
}
