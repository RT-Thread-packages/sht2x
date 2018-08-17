/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-08-08     Ernest Chen  the first version
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

#ifdef PKG_USING_SHT2X

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

static rt_err_t write_reg(struct rt_i2c_bus_device *bus, rt_uint8_t reg, rt_uint8_t data)
{
    rt_uint8_t buf[2];

    buf[0] = reg;
    buf[1] = data;

    if (rt_i2c_master_send(bus, SHT20_ADDR, 0, buf, 2) == 2)
        return RT_EOK;
    else
        return -RT_ERROR;
}

static rt_err_t read_regs(struct rt_i2c_bus_device *bus, rt_uint8_t reg, rt_uint8_t len, rt_uint8_t *buf)
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

static rt_err_t write_cmd(struct rt_i2c_bus_device *bus, rt_uint8_t cmd)
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

static float read_hw_humidity(sht20_device_t dev)
{
    rt_uint8_t temp[2];
    float current_humi = 0.0;
    rt_err_t result;

    RT_ASSERT(dev);

    result = rt_mutex_take(dev->lock, RT_WAITING_FOREVER);
    if (result == RT_EOK)
    {
        read_regs(dev->i2c, TRIG_HUMI_MEASUREMENT_HM, 2, temp);

        current_humi = -6 + (temp[1] | temp[0] << 8) * 125.0 / (1 << 16); //sensor humidity convert to reality
    }
    else
    {
        LOG_E("The sht20 could not respond relative humidity measurement at this time. Please try again");
    }
    rt_mutex_release(dev->lock);

    return current_humi;
}

static float read_hw_temperature(sht20_device_t dev)
{
    rt_uint8_t temp[2];
    float current_temp = 0.0;
    rt_err_t result;

    RT_ASSERT(dev);

    result = rt_mutex_take(dev->lock, RT_WAITING_FOREVER);
    if (result == RT_EOK)
    {
        read_regs(dev->i2c, TRIG_TEMP_MEASUREMENT_HM, 2, temp);

        current_temp = -46.85 + (temp[1] | temp[0] << 8) * 175.72 / (1 << 16); //sensor temperature convert to reality
    }
    else
    {
        LOG_E("The sht20 could not respond temperature measurement at this time. Please try again");
    }
    rt_mutex_release(dev->lock);

    return current_temp;
}

#ifdef SHT20_USING_SOFT_FILTER

static void average_measurement(sht20_device_t dev, filter_data_t *filter)
{
    rt_uint32_t i;
    float sum = 0;
    rt_uint32_t temp;
    rt_err_t result;

    RT_ASSERT(dev);

    result = rt_mutex_take(dev->lock, RT_WAITING_FOREVER);
    if (result == RT_EOK)
    {
        if (filter->is_full)
        {
            temp = SHT20_AVERAGE_TIMES;
        }
        else
        {
            temp = filter->index + 1;
        }

        for (i = 0; i < temp; i++)
        {
            sum += filter->buf[i];
        }
        filter->average = sum / temp;
    }
    else
    {
        LOG_E("The software failed to average at this time, please try again");
    }
    rt_mutex_release(dev->lock);
}

static void sht20_filter_entry(void *device)
{
    RT_ASSERT(device);

    sht20_device_t dev = (sht20_device_t)device;

    while (1)
    {
        if (dev->temp_filter.index >= SHT20_AVERAGE_TIMES)
        {
            if (dev->temp_filter.is_full != RT_TRUE)
            {
                dev->temp_filter.is_full = RT_TRUE;
            }

            dev->temp_filter.index = 0;
        }
        if (dev->humi_filter.index >= SHT20_AVERAGE_TIMES)
        {
            if (dev->humi_filter.is_full != RT_TRUE)
            {
                dev->humi_filter.is_full = RT_TRUE;
            }

            dev->humi_filter.index = 0;
        }

        dev->temp_filter.buf[dev->temp_filter.index] = read_hw_temperature(dev);
        dev->humi_filter.buf[dev->humi_filter.index] = read_hw_humidity(dev);

        rt_thread_delay(rt_tick_from_millisecond(dev->period));

        dev->temp_filter.index++;
        dev->humi_filter.index++;
    }
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

    write_cmd(dev->i2c, SOFT_RESET);
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
#ifdef SHT20_USING_SOFT_FILTER
    average_measurement(dev, &dev->temp_filter);

    return dev->temp_filter.average;
#else
    return read_hw_temperature(dev);
#endif /* SHT20_USING_SOFT_FILTER */
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
#ifdef SHT20_USING_SOFT_FILTER
    average_measurement(dev, &dev->humi_filter);

    return dev->humi_filter.average;
#else
    return read_hw_humidity(dev);
#endif /* SHT20_USING_SOFT_FILTER */
}

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
        LOG_E("Can't allocate memory for sht20 device on '%s' ", i2c_bus_name);
        rt_free(dev);
        return RT_NULL;
    }

    dev->i2c = rt_i2c_bus_device_find(i2c_bus_name);

    if (dev->i2c == RT_NULL)
    {
        LOG_E("Can't find sht20 device on '%s' ", i2c_bus_name);
        rt_free(dev);
        return RT_NULL;
    }

    dev->lock = rt_mutex_create("mutex_sht20", RT_IPC_FLAG_FIFO);
    if (dev->lock == RT_NULL)
    {
        LOG_E("Can't create mutex for sht20 device on '%s' ", i2c_bus_name);
        rt_free(dev);
        
        return RT_NULL;
    }

#ifdef SHT20_USING_SOFT_FILTER
    dev->period = SHT20_SAMPLE_PERIOD;

    dev->thread = rt_thread_create("sht20", sht20_filter_entry, (void *)dev, 1024, 15, 10);
    if (dev->thread != RT_NULL)
    {
        rt_thread_startup(dev->thread);
    }
    else
    {
        LOG_E("Can't start filtering function for sht20 device on '%s' ", i2c_bus_name);
        rt_mutex_delete(dev->lock);
        rt_free(dev);
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
            LOG_E("Parameter value is invalid");
            return -RT_ERROR;
        }

        /* read user data */
        read_regs(dev->i2c, USER_REG_R, 1, &temp);

        if (temp != value)
        {
            value |= temp & (~SHT20_RES_MASK);

            write_reg(dev->i2c, USER_REG_W, value);
        }
        break;
    }

    case SHT20_PARAM_BATTERY_STATUS:
    {
        rt_uint8_t temp;

        if (!(value == SHT20_BATTERY_HIGH || value == SHT20_BATTERY_LOW))
        {
            LOG_E("Parameter value is invalid");
            return -RT_ERROR;
        }

        /* read user data */
        read_regs(dev->i2c, USER_REG_R, 1, &temp);

        if (temp != value)
        {

            value |= temp & (~SHT20_BATTERY_MASK);

            write_reg(dev->i2c, USER_REG_W, value);
        }
        break;
    }
    case SHT20_PARAM_HEATING:
    {
        rt_uint8_t temp;

        if (!(value == SHT20_HEATER_ON || value == SHT20_HEATER_OFF))
        {
            LOG_E("Parameter value is invalid");
            return -RT_ERROR;
        }

        /* read user data */
        read_regs(dev->i2c, USER_REG_R, 1, &temp);

        if (temp != value)
        {
            value |= temp & (~SHT20_HEATER_MASK);

            write_reg(dev->i2c, USER_REG_W, value);
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
        read_regs(dev->i2c, USER_REG_R, 1, &temp);
        *value = temp & SHT20_RES_MASK;

        break;
    }

    case SHT20_PARAM_BATTERY_STATUS:
    {
        rt_uint8_t temp;

        /* read user data */
        read_regs(dev->i2c, USER_REG_R, 1, &temp);

        *value = temp & SHT20_BATTERY_MASK;

        break;
    }
    case SHT20_PARAM_HEATING:
    {
        rt_uint8_t temp;

        /* read user data */
        read_regs(dev->i2c, USER_REG_R, 1, &temp);

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

                /* read the sensor data */
                humidity = sht20_read_humidity(dev);
                temperature = sht20_read_temperature(dev);

                rt_kprintf("read sht20 sensor humidity   : %d.%d %%\n", (int)humidity, (int)(humidity * 10) % 10);
                rt_kprintf("read sht20 sensor temperature: %d.%d \n", (int)temperature, (int)(temperature * 10) % 10);
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

#endif /* PKG_USING_SHT2X */
