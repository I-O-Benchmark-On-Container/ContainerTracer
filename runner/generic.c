/**
 * @copyright "Container Tracer" which executes the container performance mesurements
 * Copyright (C) 2020 BlaCkinkGJ
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * @file generic.c
 * @brief runtime에 driver를 특정할 수 있도록 만들어주는 generic 헤더의 구현부에 해당합니다.
 * @author BlaCkinkGJ (ss5kijun@gmail.com)
 * @version 0.1
 * @date 2020-08-05
 * @note 참고 자료: https://github.com/mit-pdos/xv6-public/blob/master/syscall.c
 */

#include <generic.h>
#include <driver/tr-driver.h>
#include <driver/docker-driver.h>

const char *driver_name_tbl[] = { [TRACE_REPLAY_DRIVER] = "trace-replay",
                                  [DOCKER_DRIVER] = "docker",
                                  NULL };

static int (*driver_init_tbl[])(void *object) = {
        [TRACE_REPLAY_DRIVER] = tr_init,
        [DOCKER_DRIVER] = docker_init,
};

/**
 * @brief 임의의 이름에 적합한 driver의 index를 반환해주도록 합니다.
 *
 * @param name 찾고자 하는 driver의 이름에 해당합니다.
 *
 * @return name에 해당하는 driver의 index가 반환됩니다.
 * @exception -EINVAL 테이블에서 찾을 수 없는 유효하지 않은 name이 부여되었음을 의미합니다.
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
 * @param object 초기화하는 driver의 init에 들어가는 object의 주소를 가집니다.
 *
 * @return 성공적으로 초기화를 한 경우에 0을 반환하고, 그렇지 않은 경우에 0 이하의 값을 반홥니다.
 * @exception -EINVAL 존재하지 않는 name을 조회한 경우에 반환을 하는 역할을 합니다.
 */
int generic_driver_init(const char *name, void *object)
{
        int num = get_generic_driver_index(name);
        if (num < 0) {
                return -EINVAL;
        }

        return (driver_init_tbl[num])(object);
}
