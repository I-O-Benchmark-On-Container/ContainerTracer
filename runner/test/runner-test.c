#include <unity.h>
#include <generic.h>

void setUp(void)
{
}

void tearDown(void)
{
}

void test(void)
{
        // 현재 디렉터리를 #/build/release 라고 가정하겠습니다.
        const char *json = "{\"driver\":\"trace-replay\", \
				  \"setting\": { \
					\"nr_tasks\": 4, \
					\"time\": 60, \
					\
					\"q_depth\": 32, \
					\"nr_thread\": 4, \
					\
				    \"prefix_cgroup_name\": \"tester.trace.\", \
					\"scheduler\": \"bfq\", \
					\"task_option\": [ {\"weight\":100}, {\"weight\":250}, {\"weight\":500}, {\"weight\":1000},]\
					} \
				}";
        const struct runner_config *config = NULL;
        TEST_ASSERT_EQUAL(0, runner_init(json));
        TEST_ASSERT_NOT_NULL(config = runner_get_global_config());
        TEST_ASSERT_EQUAL_STRING(config->driver, "trace-replay");
        TEST_ASSERT_EQUAL(TRACE_REPLAY_DRIVER,
                          get_generic_driver_index(config->driver));
}

int main(void)
{
        UNITY_BEGIN();

        RUN_TEST(test);

        return UNITY_END();
}
