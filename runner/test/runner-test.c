#include <unity.h>
#include <runner.h>

void setUp(void)
{
}

void tearDown(void)
{
}

void test(void)
{
        int a = 1;
        TEST_ASSERT_EQUAL(2, add(a, a));
}

int main(void)
{
        UNITY_BEGIN();

        RUN_TEST(test);

        return UNITY_END();
}