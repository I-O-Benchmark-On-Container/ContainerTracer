#include <stdlib.h>
#include <jemalloc/jemalloc.h>
#include <unity.h>
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
					\"task_option\": [ {\"name\": \"cgroup-1\" , \"weight\":100}, {\"name\": \"cgroup-2\" ,\"weight\":250}, {\"name\": \"cgroup-3\" ,\"weight\":500}, {\"name\": \"cgroup-4\" ,\"weight\":1000}],\
					} \
				}";
        const struct runner_config *config = NULL;
        TEST_ASSERT_EQUAL(0, runner_init(json));
        TEST_ASSERT_NOT_NULL(config = runner_get_global_config());
        TEST_ASSERT_EQUAL_STRING(config->driver, "trace-replay");
        TEST_ASSERT_EQUAL(TRACE_REPLAY_DRIVER,
                          get_generic_driver_index(config->driver));
}

void tearDown(void)
{
        runner_free();
}

void test(void)
{
        TEST_ASSERT_EQUAL(0, runner_run());
}

int main(void)
{
        UNITY_BEGIN();

        RUN_TEST(test);

        return UNITY_END();
}
