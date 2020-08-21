#include <unity.h>
#include <log.h>

void setUp(void)
{
        pr_info(INFO, "%s contents here...\n", "setUp");
}

void tearDown(void)
{
        pr_info(INFO, "%s contents here...\n", "tearDown");
}

void test(void)
{
        pr_info(INFO, "%s contents here...\n", "test");
}

int main(void)
{
        UNITY_BEGIN();

        RUN_TEST(test);

        return UNITY_END();
}
