/**< system header */
#include <stdlib.h>
#include <unity.h>

/**< external header */
#include <jemalloc/jemalloc.h>

/**< user header */
#include <generic.h>
#include <runner.h>

void setUp(void)
{
        const char *json = "{\"driver\":\"trace-replay\", \
				  \"setting\": { \
				    \"trace_replay_path\":\"./build/release/trace-replay\", \
					\"nr_tasks\": 4, \
					\"time\": 60, \
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

void test_interval_result(void)
{
        char *buffer = runner_get_interval_result("cgroup-2");
        TEST_ASSERT_NOT_NULL(buffer);
        TEST_ASSERT_EQUAL_STRING(buffer, "interval ==> cgroup-2");
        runner_put_result_string(buffer);
}

void test_total_result(void)
{
        char *buffer = runner_get_total_result("cgroup-2");
        TEST_ASSERT_NOT_NULL(buffer);
        TEST_ASSERT_EQUAL_STRING(buffer, "total ==> cgroup-2");
        runner_put_result_string(buffer);
}

int main(void)
{
        UNITY_BEGIN();

        RUN_TEST(test_interval_result);
        RUN_TEST(test_total_result);

        return UNITY_END();
}
