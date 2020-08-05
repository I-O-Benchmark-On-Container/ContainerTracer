/**
 * @file generic.c
 * @brief runtime에 driver를 특정할 수 있도록 만들어주는 generic 헤더의 구현부에 해당합니다.
 * @author BlaCkinkGJ (ss5kijun@gmail.com)
 * @version 0.1
 * @date 2020-08-05
 * @ref https://github.com/mit-pdos/xv6-public/blob/master/syscall.c
 */
#include <generic.h>

const char *driver_name_tbl[] = { [TRACE_REPLAY_DRIVER] = "trace-replay",
                                  NULL };

static int (*driver_init_tbl[])(struct runner_op *op) = {
        [TRACE_REPLAY_DRIVER] = tr_init
};

/**
 * @brief 임의의 이름에 적합한 driver의 index를 반환해주도록 합니다.
 *
 * @param name 찾고자 하는 driver의 이름에 해당합니다.
 *
 * @return index name에 해당하는 driver의 index가 반환됩니다.
 * @except -EINVAL 테이블에서 찾을 수 없는 유효하지 않은 name이 부여되었음을 의미합니다.
 */
int get_generic_driver_index(const char *name)
{
        int index = 0;
        while (driver_name_tbl[index] != NULL) {
                if (!strcmp(driver_name_tbl[index], name)) {
                        return index;
                }
                index++;
        }
        return -EINVAL;
}

/**
 * @brief name에 해당하는 driver에 대해서 초기화를 실시해주도록 합니다.
 *
 * @param name driver의 이름에 해당합니다.
 * @param op driver의 명령 집합을 저장하는 전역 ruuner_config의 op 매개 변수가 들어가게 됩니다.
 *
 * @return 성공적으로 초기화를 한 경우에 0을 반환하고, 그렇지 않은 경우에 0 이하의 값을 반홥니다.
 * @except -EINVAL 존재하지 않는 name을 조회한 경우에 반환을 하는 역할을 합니다.
 */
int generic_driver_init(const char *name, struct runner_op *op)
{
        int num = get_generic_driver_index(name);
        if (num < 0) {
                return -EINVAL;
        }

        return (driver_init_tbl[num])(op);
}
