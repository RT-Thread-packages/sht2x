/*
 * File      : drv_sht20.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006 - 2012, RT-Thread Development Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-01-30     DQL          first implementation.
 * 2018-07-30     Ernest Chen  code refactoring
 */

#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>

#include <string.h>

#define DBG_ENABLE
#define DBG_SECTION_NAME "sht20"
#define DBG_LEVEL DBG_LOG
#define DBG_COLOR
#include <rtdbg.h>

#include "sht20.h"

#define BSP_USING_SHT20
#ifdef BSP_USING_SHT20

/*sht20 registers define */
#define TRIG_TEMP_MEASUREMENT_HM 0xE3   // trigger temperature measurement on hold master
#define TRIG_HUMI_MEASUREMENT_HM 0xE5   // trigger humidity measurement on hold master
#define TRIG_TEMP_MEASUREMENT_POLL 0xF3 // trigger temperature measurement without hold master
#define TRIG_HUMI_MEASUREMENT_POLL 0xF5 // trigger humidity measurement without hold master
#define USER_REG_W 0xE6                 // write user register
#define USER_REG_R 0xE7                 // read user register
#define SOFT_RESET 0xFE

#define SHT20_RES_12_14BIT 0x00 // RH=12bit, T=14bit default
#define SHT20_RES_8_12BIT 0x01  // RH= 8bit, T=12bit
#define SHT20_RES_10_13BIT 0x80 // RH=10bit, T=13bit
#define SHT20_RES_11_11BIT 0x81 // RH=11bit, T=11bit
#define SHT20_RES_MASK 0x81     // Mask for res. bits (7,0) in user reg.

#define SHT20_HEATER_ON 0x04   // heater on
#define SHT20_HEATER_OFF 0x00  // heater off
#define SHT20_HEATER_MASK 0x04 // Mask for Heater bit(2) in user reg.

#define SHT20_BATTERY_HIGH 0x00 // higher than 2.25
#define SHT20_BATTERY_LOW 0x40  // lower than 2.25
#define SHT20_BATTERY_MASK 0x40 // Mask for batter bit(6) in user reg.

#define SHT20_ADDR 0x40

static rt_err_t sht20_write_reg(struct rt_i2c_bus_device *bus, rt_uint8_t reg, rt_uint8_t data)
{
    rt_uint8_t buf[2];

    buf[0] = reg;
    buf[1] = data;

    if (rt_i2c_master_send(bus, SHT20_ADDR, 0, buf, 2) == 2)
        return RT_EOK;
    else
        return -RT_ERROR;
}

static rt_err_t sht20_read_regs(struct rt_i2c_bus_device *bus, rt_uint8_t reg, rt_uint8_t len, rt_uint8_t *buf)
{
    struct rt_i2c_msg msgs[2];

    msgs[0].addr = SHT20_ADDR;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf = &reg;
    msgs[0].len = 1;

    msgs[1].addr = SHT20_ADDR;
    msgs[1].flags = RT_I2C_RD;
    msgs[1].buf = buf;
    msgs[1].len = len;

    if (rt_i2c_transfer(bus, msgs, 2) == 2)
    {
        return RT_EOK;
    }
    else
    {
        return -RT_ERROR;
    }
}

static rt_err_t sht20_write_cmd(struct rt_i2c_bus_device *bus, rt_uint8_t cmd)
{
    struct rt_i2c_msg msgs;

    msgs.addr = SHT20_ADDR;
    msgs.flags = RT_I2C_WR;
    msgs.buf = &cmd;
    msgs.len = 1;

    if (rt_i2c_transfer(bus, &msgs, 1) == 1)
        return RT_EOK;
    else
        return -RT_ERROR;
}

#ifdef SHT20_USING_SOFT_FILTER
static void sht20_average_temp(sht20_device_t dev)
{
    rt_uint32_t i;
    float sum = 0;
    rt_uint32_t temp;
    rt_err_t result;

    RT_ASSERT(dev);

    result = rt_mutex_take(dev->lock, RT_WAITING_FOREVER);
    if (result == RT_EOK)
    {
        if (dev->temp_filter.is_full)
        {
            temp = SHT20_AVERAGE_TIMES;
        }
        else
        {
            temp = dev->temp_filter.index;
        }

        for (i = 0; i < temp; i++)
        {
            sum += dev->temp_filter.buf[i];
        }
        dev->temp_filter.average = sum / temp;
    }
    rt_mutex_release(dev->lock);
}

static void sht20_average_humi(sht20_device_t dev)
{
    rt_uint32_t i;
    float sum = 0;
    rt_uint32_t temp;
    rt_err_t result;

    RT_ASSERT(dev);

    result = rt_mutex_take(dev->lock, RT_WAITING_FOREVER);
    if (result == RT_EOK)
    {
        if (dev->humi_filter.is_full)
        {
            temp = SHT20_AVERAGE_TIMES;
        }
        else
        {
            temp = dev->humi_filter.index;
        }

        for (i = 0; i < temp; i++)
        {
            sum += dev->humi_filter.buf[i];
        }
        dev->humi_filter.average = sum / temp;
    }
    rt_mutex_release(dev->lock);
}
#endif /* SHT20_USING_SOFT_FILTER */

/**
 * This function resets all parameter with default
 *
 * @param dev the pointer of device driver structure
 *
 * @return the softreset status,RT_EOK reprensents setting successfully.
 */
rt_err_t sht20_softreset(sht20_device_t dev)
{
    RT_ASSERT(dev);

    sht20_write_cmd(dev->i2c, SOFT_RESET);
    return 0;
}

/**
 * This function reads temperature by sht20 sensor measurement
 *
 * @param dev the pointer of device driver structure
 *
 * @return the relative temperature converted to float data.
 */
float sht20_read_temperature(sht20_device_t dev)
{
    rt_uint8_t temp[2];
    float S_T;
    rt_err_t result;

    RT_ASSERT(dev);

    result = rt_mutex_take(dev->lock, RT_WAITING_FOREVER);
    if (result == RT_EOK)
    {
        sht20_read_regs(dev->i2c, TRIG_TEMP_MEASUREMENT_HM, 2, temp);

        S_T = -46.85 + (temp[1] | temp[0] << 8) * 175.72 / (1 << 16); //sensor temperature converse to reality

#ifdef SHT20_USING_SOFT_FILTER
        dev->temp_filter.buf[dev->temp_filter.index] = S_T;
#endif /* SHT20_USING_SOFT_FILTER */
    }
    else
    {
        LOG_E("read humidity error! ");
        return -RT_ERROR;
    }
    rt_mutex_release(dev->lock);

#ifdef SHT20_USING_SOFT_FILTER
    sht20_average_temp(dev);
    S_T = dev->temp_filter.average;

#endif /* SHT20_USING_SOFT_FILTER */

    return S_T;
}

/**
 * This function reads relative humidity by sht20 sensor measurement
 *
 * @param dev the pointer of device driver structure
 *
 * @return the relative humidity converted to float data.
 */
float sht20_read_humidity(sht20_device_t dev)
{
    rt_uint8_t temp[2];
    float S_H;
    rt_err_t result;

    RT_ASSERT(dev);

    result = rt_mutex_take(dev->lock, RT_WAITING_FOREVER);
    if (result == RT_EOK)
    {
        sht20_read_regs(dev->i2c, TRIG_HUMI_MEASUREMENT_HM, 2, temp);

        S_H = -6 + (temp[1] | temp[0] << 8) * 125.0 / (1 << 16); //sensor humidity converse to reality

#ifdef SHT20_USING_SOFT_FILTER
        dev->humi_filter.buf[dev->humi_filter.index] = S_H;
#endif /* SHT20_USING_SOFT_FILTER */
    }
    else
    {
        LOG_E("read humidity error! ");
        return -RT_ERROR;
    }
    rt_mutex_release(dev->lock);

#ifdef SHT20_USING_SOFT_FILTER
    sht20_average_humi(dev);
    S_H = dev->humi_filter.average;

#endif /* SHT20_USING_SOFT_FILTER */

    return S_H;
}

#ifdef SHT20_USING_SOFT_FILTER
void sht20_filter_entry(void *device)
{
    RT_ASSERT(device);

    sht20_device_t dev = (sht20_device_t)device;

    while (1)
    {

        dev->temp_filter.buf[dev->temp_filter.index++] = sht20_read_temperature(dev);
        dev->humi_filter.buf[dev->humi_filter.index++] = sht20_read_humidity(dev);

        if (dev->temp_filter.index >= SHT20_AVERAGE_TIMES)
        {
            dev->temp_filter.is_full = RT_TRUE;
            dev->temp_filter.index = 0;
        }
        if (dev->humi_filter.index >= SHT20_AVERAGE_TIMES)
        {
            dev->humi_filter.is_full = RT_TRUE;
            dev->temp_filter.index = 0;
        }

        rt_thread_delay(dev->period);
    }
}
#endif /* SHT20_USING_SOFT_FILTER */

/**
 * This function initializes sht20 registered device driver
 *
 * @param dev the name of sht20 device
 *
 * @return the sht20 device.
 */
sht20_device_t sht20_init(const char *i2c_bus_name)
{
    sht20_device_t dev;

    RT_ASSERT(i2c_bus_name);

    dev = rt_calloc(1, sizeof(struct sht20_device));
    if (dev == RT_NULL)
    {
        LOG_E("can't calloc sht20 %s device memory \r\n", i2c_bus_name);
        rt_free(dev);
        return RT_NULL;
    }

    dev->i2c = rt_i2c_bus_device_find(i2c_bus_name);

    if (dev->i2c == RT_NULL)
    {
        LOG_E("can't find sht20 %s device\r\n", i2c_bus_name);
        rt_free(dev);
        return RT_NULL;
    }

    dev->lock = rt_mutex_create("mutex_sht20", RT_IPC_FLAG_FIFO);
    if (dev->lock == RT_NULL)
    {
        LOG_E("can't create sht20 mutex of %s device\r\n", i2c_bus_name);
        rt_free(dev);
        return RT_NULL;
    }

#ifdef SHT20_USING_SOFT_FILTER
    if (dev->period == 0)
        dev->period = 1000; /* default */

    dev->thread = rt_thread_create("sht20", sht20_filter_entry, (void *)dev, 512, 15, 10);
    if (dev->thread != RT_NULL)
    {
        rt_thread_startup(dev->thread);
    }
    else
    {
        LOG_E("sht20 soft filter start error !\n");
    }
#endif /* SHT20_USING_SOFT_FILTER */

    return dev;
}

/**
 * This function releases memory and deletes mutex lock
 *
 * @param dev the pointer of device driver structure
 */
void sht20_deinit(sht20_device_t dev)
{
    RT_ASSERT(dev);

    rt_mutex_delete(dev->lock);

#ifdef SHT20_USING_SOFT_FILTER
		rt_thread_delete(dev->thread);
#endif

    rt_free(dev);
}

/**
 * This function sets parameter of sht20 sensor
 *
 * @param dev the pointer of device driver structure
 * @param type the parameter type of device
 * @param value the pointer value of type
 *
 * @return the setting parameter status,RT_EOK reprensents setting successfully.
 */
rt_err_t sht20_set_param(sht20_device_t dev, sht20_param_type_t type, rt_uint8_t value)
{
    RT_ASSERT(dev);

    switch (type)
    {
    case SHT20_PARAM_PRECISION:
    {
        rt_uint8_t temp;

        if (!(value == SHT20_RES_12_14BIT || value == SHT20_RES_8_12BIT || value == SHT20_RES_10_13BIT || value == SHT20_RES_11_11BIT))
        {
            LOG_E("parameter value is wrong !\r\n");
            return -RT_ERROR;
        }

        /* read user data */
        sht20_read_regs(dev->i2c, USER_REG_R, 1, &temp);

        if (temp != value)
        {
            value |= temp & (~SHT20_RES_MASK);

            sht20_write_reg(dev->i2c, USER_REG_W, value);
        }
        else
        {
            LOG_E("precision status does not change !\r\n");
        }

        break;
    }

    case SHT20_PARAM_BATTERY_STATUS:
    {
        rt_uint8_t temp;

        if (!(value == SHT20_BATTERY_HIGH || value == SHT20_BATTERY_LOW))
        {
            LOG_E("parameter value is wrong !\r\n");
            return -RT_ERROR;
        }

        /* read user data */
        sht20_read_regs(dev->i2c, USER_REG_R, 1, &temp);

        if (temp != value)
        {

            value |= temp & (~SHT20_BATTERY_MASK);

            sht20_write_reg(dev->i2c, USER_REG_W, value);
        }
        else
        {
            LOG_E("battery status does not change!\r\n");
        }

        break;
    }
    case SHT20_PARAM_HEATING:
    {
        rt_uint8_t temp;

        if (!(value == SHT20_HEATER_ON || value == SHT20_HEATER_OFF))
        {
            LOG_E("parameter value is wrong !\r\n");
            return -RT_ERROR;
        }

        /* read user data */
        sht20_read_regs(dev->i2c, USER_REG_R, 1, &temp);

        if (temp != value)
        {
            value |= temp & (~SHT20_HEATER_MASK);

            sht20_write_reg(dev->i2c, USER_REG_W, value);
        }
        else
        {
            LOG_E("heating status does not change!\r\n");
        }

        break;
    }
    }

    return RT_EOK;
}

/**
 * This function gets parameter of sht20 sensor
 *
 * @param dev the pointer of device driver structure
 * @param type the parameter type of device
 * @param value the pointer value of type
 *
 * @return the getting parameter status,RT_EOK reprensents getting successfully.
 */
rt_err_t sht20_get_param(sht20_device_t dev, sht20_param_type_t type, rt_uint8_t *value)
{
    RT_ASSERT(dev);

    switch (type)
    {
    case SHT20_PARAM_PRECISION:
    {
        rt_uint8_t temp;

        /* read user data */
        sht20_read_regs(dev->i2c, USER_REG_R, 1, &temp);
        *value = temp & SHT20_RES_MASK;

        break;
    }

    case SHT20_PARAM_BATTERY_STATUS:
    {
        rt_uint8_t temp;

        /* read user data */
        sht20_read_regs(dev->i2c, USER_REG_R, 1, &temp);

        *value = temp & SHT20_BATTERY_MASK;

        break;
    }
    case SHT20_PARAM_HEATING:
    {
        rt_uint8_t temp;

        /* read user data */
        sht20_read_regs(dev->i2c, USER_REG_R, 1, &temp);

        *value = temp & SHT20_HEATER_MASK;

        break;
    }
    }

    return RT_EOK;
}

void sht20(int argc, char *argv[])
{
    static sht20_device_t dev = RT_NULL;

    if (argc > 1)
    {
        if (!strcmp(argv[1], "probe"))
        {
            if (argc > 2)
            {
                /* initialize the sensor when first probe */
                if (!dev || strcmp(dev->i2c->parent.parent.name, argv[2]))
                {
                    /* deinit the old device */
                    if (dev)
                    {
                        sht20_deinit(dev);
                    }
                    dev = sht20_init(argv[2]);
                }
            }
            else
            {
                rt_kprintf("sht20 probe <dev_name>   - probe sensor by given name\n");
            }
        }
        else if (!strcmp(argv[1], "read"))
        {
            if (dev)
            {
                float humidity;
                float temperature;

                /* read the sensor */
                humidity = sht20_read_humidity(dev);
                rt_kprintf("read sht20 sensor humidity   :%d \n", (int)humidity);

                temperature = sht20_read_temperature(dev);
                rt_kprintf("read sht20 sensor temperature:%d \n", (int)temperature);
            }
            else
            {
                rt_kprintf("Please using 'sht20 probe <dev_name>' first\n");
            }
        }
        else
        {
            rt_kprintf("Unknown command. Please enter 'sht20' for help\n");
        }
    }
    else
    {
        rt_kprintf("Usage:\n");
        rt_kprintf("sht20 probe <dev_name>   - probe sensor by given name\n");
        rt_kprintf("sht20 read               - read sensor sht20 data\n");
    }
}

MSH_CMD_EXPORT(sht20, sht20 sensor function);

#endif /* BSP_USING_SHT20 */
