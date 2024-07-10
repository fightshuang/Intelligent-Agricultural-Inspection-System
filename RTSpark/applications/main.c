/////*
//// * Copyright (c) 2006-2021, RT-Thread Development Team
//// *
//// * SPDX-License-Identifier: Apache-2.0
//// *
//// * Change Logs:
//// * Date           Author       Notes
//// * 2023-5-10      ShiHao       first version
//// */
////
#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <aht10.h>
#include <driver/buzzer.h>
#include <driver/led.h>
#include <drv_lcd.h>
#include <rttlogo.h>
#include <drv_matrix_led.h>
#define SAMPLE_UART_NAME    "uart2"
static struct rt_semaphore rx_sem;
static rt_device_t serial;
#define DBG_TAG "main"
#define DBG_LVL         DBG_LOG
#include <rtdbg.h>
static struct rt_mailbox led_matrix_mb;
static struct rt_mailbox buzzer_mb;
static char led_matrix_mb_pool[128];
static char buzzer_mb_pool[128];
float humidity = 0;
float temperature = 0;
uint16_t flag;
rt_int16_t x;
rt_int16_t y;
rt_int16_t z;
enum{
    EXTERN_LED_0,
    EXTERN_LED_1,
    EXTERN_LED_2,
    EXTERN_LED_3,
    EXTERN_LED_4,
    EXTERN_LED_5,
    EXTERN_LED_6,
    EXTERN_LED_7,
    EXTERN_LED_8,
    EXTERN_LED_9,
    EXTERN_LED_10,
    EXTERN_LED_11,
    EXTERN_LED_12,
    EXTERN_LED_13,
    EXTERN_LED_14,
    EXTERN_LED_15,
    EXTERN_LED_16,
    EXTERN_LED_17,
    EXTERN_LED_18,
};

static void uart_parse_frame(uint8_t *buffer, uint8_t length)
{
    uint8_t sum = 0;
    for (uint8_t i = 0; i < (length - 1); i++)
        sum += *(buffer + i);
    if (!(sum == *(buffer + length - 1)))
        return;
    if (!(*(buffer) == 0xAA && *(buffer + 1) == 0x29))
        return;

    /* 参数读取ID 0x42 */
    if (buffer[2] == 0x42)
    {
        flag = buffer[4];
        int16_t object_width = ((uint16_t)buffer[5] << 8) + buffer[6];
        int16_t object_x = ((uint16_t)buffer[7] << 8) + buffer[8];
        int16_t object_y = ((uint16_t)buffer[9] << 8) + buffer[10];
        uint16_t fps = buffer[11];

        rt_kprintf("flag:%d width:%d blob_x:%d blob_y:%d ms:%d\n", flag, object_width, object_x, object_y, fps);
    }
}

static int uart_receive_byte(uint8_t data)
{
    static uint8_t RxBuffer[50];
    static uint8_t _data_len = 0, _data_cnt = 0;
    static uint8_t state = 0;

    /* 帧头 */
    if (state == 0 && data == 0xAA)
    {
        _data_cnt = 0;
        _data_len = 0;

        state = 1;
        RxBuffer[0] = data;
    }
    /* 帧头 */
    else if (state == 1 && data == 0x29)
    {
        state = 2;
        RxBuffer[1] = data;
    }
    /* ID */
    else if (state == 2 && data < 0xF1)
    {
        state = 3;
        RxBuffer[2] = data;
    }
    /* 数据长度 */
    else if (state == 3 && data < 50)
    {
        state = 4;
        RxBuffer[3] = data;
        _data_len = data;
    }
    /* 和校验 */
    else if (state == 4)
    {
        RxBuffer[4 + _data_cnt++] = data;
        if (_data_cnt >= _data_len)
            state = 5;
    }
    /* 数据解析 */
    else if (state == 5)
    {
        state = 0;
        RxBuffer[4 + _data_cnt++] = data;

        uart_parse_frame(RxBuffer, _data_cnt + 4);
        return 1;
    }
    else
        state = 0;

    return 0;
}

/* 接收数据回调函数 */
static rt_err_t uart_rx_ind(rt_device_t dev, rt_size_t size)
{
    /* 串口接收到数据后产生中断，调用此回调函数，然后发送接收信号量 */
    if (size > 0)
    {
        rt_sem_release(&rx_sem);
    }
    return RT_EOK;
}

static char uart_sample_get_char(void)
{
    char ch;

    while (rt_device_read(serial, 0, &ch, 1) == 0)
    {
        rt_sem_control(&rx_sem, RT_IPC_CMD_RESET, RT_NULL);
        rt_sem_take(&rx_sem, RT_WAITING_FOREVER);
    }
    return ch;
}

/* 数据解析线程 */
static void data_parsing(void)
{
    while (1)
    {
        uart_receive_byte(uart_sample_get_char());
    }
}

static int uart_data_sample(void *parameter)
{
    rt_err_t ret = RT_EOK;
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;  /* 初始化配置参数 */

    /* 查找系统中的串口设备 */
    serial = rt_device_find(SAMPLE_UART_NAME);
    if (!serial)
    {
        rt_kprintf("find %s failed!\n", SAMPLE_UART_NAME);
        return RT_ERROR;
    }

    /* 修改串口配置参数 */
    config.baud_rate = 115200;          //修改波特率为 115200
    config.data_bits = DATA_BITS_8;     //数据位 8
    config.stop_bits = STOP_BITS_1;     //停止位 1
    config.bufsz     = 512;             //修改缓冲区
    config.parity    = PARITY_NONE;     //无奇偶校验位

    /* 控制串口设备。通过控制接口传入命令控制字，与控制参数 */
    rt_device_control(serial, RT_DEVICE_CTRL_CONFIG, &config);

    /* 初始化信号量 */
    rt_sem_init(&rx_sem, "rx_sem", 0, RT_IPC_FLAG_FIFO);
    /* 以中断接收及轮询发送模式打开串口设备 */
    rt_device_open(serial, RT_DEVICE_FLAG_INT_RX);
    /* 设置接收回调函数 */
    rt_device_set_rx_indicate(serial, uart_rx_ind);

    /* 创建 serial 线程 */
    rt_thread_t thread = rt_thread_create("UART_RX", (void (*)(void *parameter))data_parsing, RT_NULL, 1024, 25, 10);
    /* 创建成功则启动线程 */
    if (thread != RT_NULL)
    {
        rt_thread_startup(thread);
    }
    else
    {
        ret = RT_ERROR;
    }

    return ret;
}

rt_thread_t buzzer_thread;

static void buzzer_entry()
{
    int *temperature;

    while (1)
       {
            rt_mb_recv(&buzzer_mb, (rt_uint32_t *)&temperature, RT_WAITING_FOREVER);
            if (temperature >= 30)
            {
               buzzer_on();
               led_off_green();
               led_on_red();
            }
            else if (temperature < 21)
            {
               buzzer_on();
               led_off_green();
               led_on_red();
            }
            else
            {
               buzzer_off();
               led_off_red();
               led_on_green();
            }

       }
}

rt_thread_t Humidity_thread;

static void lcd_humidity_entry()
{
    lcd_clear(WHITE1);
    lcd_set_color(WHITE1, BLACK);
    aht10_device_t use_aht10 = aht10_init("i2c3");
    while(1)
    {
        lcd_show_string(75, 200, 24, "Humidity");
        temperature =  aht10_read_temperature(use_aht10);
        humidity =  aht10_read_humidity(use_aht10);
        lcd_show_string(45, 69, 24, "temp: %d.%d",(int)temperature, (int)(temperature * 10) % 10);
        lcd_show_string(45, 69 + 48, 24, "humi: %d.%d %%", (int)humidity, (int)(humidity * 10) % 10);
        lcd_show_string(180-15+9, 69-6-2, 16, "o");
        lcd_show_string(180-15+8+8,69, 24,"C");
        lcd_show_image(15,69 + 48, 24, 24, gImage_water);
        lcd_show_image(18,65 , 15, 36, gImage_temp);
        rt_mb_send(&led_matrix_mb, (rt_uint32_t)temperature);
        rt_mb_send(&buzzer_mb, (rt_uint32_t)temperature);
        rt_thread_mdelay(500);
    }
}

rt_thread_t led_matrix_thread;

static void led_matrix_entry()
{
    int *temperature;

    while (1)
    {
        rt_mb_recv(&led_matrix_mb, (rt_uint32_t *)&temperature, RT_WAITING_FOREVER);
        if (temperature >= 30)
        {
            for (int i = EXTERN_LED_0; i <= EXTERN_LED_18; i++)
            {
                led_matrix_set_color(i, RED);
                rt_thread_mdelay(20);
                led_matrix_reflash();
            }
        }
        else if (temperature < 21)
        {
            for (int i = EXTERN_LED_0; i <= EXTERN_LED_18; i++)
            {
                led_matrix_set_color(i, BLUE);
                rt_thread_mdelay(20);
                led_matrix_reflash();
            }
        }
        else
        {
            for (int i = EXTERN_LED_0; i <= EXTERN_LED_18; i++)
            {
                led_matrix_set_color(i, GREEN);
                rt_thread_mdelay(20);
                led_matrix_reflash();
            }
        }
    }
}

rt_thread_t uart_thread;

int main(void)
{

    rt_mb_init(&led_matrix_mb,"led_matrix_mbt",&led_matrix_mb_pool[0],sizeof(led_matrix_mb_pool) / 4,RT_IPC_FLAG_FIFO);
    rt_mb_init(&buzzer_mb,"buzzer_mbt",&buzzer_mb_pool[0],sizeof(buzzer_mb_pool) / 4,RT_IPC_FLAG_FIFO);

    Humidity_thread = rt_thread_create("humidity",lcd_humidity_entry, RT_NULL,1024,25, 20);
    if (Humidity_thread != RT_NULL)
        rt_thread_startup(Humidity_thread);

    buzzer_thread = rt_thread_create("buzzer",buzzer_entry, RT_NULL,1024,25, 20);
    if (buzzer_thread != RT_NULL)
        rt_thread_startup(buzzer_thread);

    uart_thread = rt_thread_create("uart",uart_data_sample, RT_NULL,1024,25, 20);
    if (uart_thread != RT_NULL)
        rt_thread_startup(uart_thread);

    led_matrix_thread = rt_thread_create("led matrix", led_matrix_entry, RT_NULL, 1024, 20, 20);
    if (led_matrix_thread != RT_NULL)
        rt_thread_startup(led_matrix_thread);
    rt_thread_mdelay(200);

    return 0;
}

