/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-06-06     Hdw13       the first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

#define PIN_BUZZER  GET_PIN(B, 0)

void buzzer_on(void)
{
    rt_pin_mode(PIN_BUZZER, PIN_MODE_OUTPUT);
    rt_pin_write(PIN_BUZZER, 1);
}

void buzzer_off(void)
{
    rt_pin_mode(PIN_BUZZER, PIN_MODE_OUTPUT);
    rt_pin_write(PIN_BUZZER, 0);
}
