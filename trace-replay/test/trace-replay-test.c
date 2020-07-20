#include <unity.h>
#include <trace_replay.h>

void setUp(void)
{
}

void tearDown(void)
{
}

void test(void)
{
        TEST_ASSERT_EQUAL(512, MAX_THREADS);
}

int main(void)
{
        UNITY_BEGIN();

        RUN_TEST(test);

        return UNITY_END();
}