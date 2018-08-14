/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-07-30     Ernest Chen  first implementation.
 */
 
#ifndef __SHT20_H__
#define __SHT20_H__

#include <rthw.h>
#include <rtthread.h>

#include <rthw.h>
#include <rtdevice.h>

#include "drv_iic.h"

#define SHT20_AVERAGE_TIMES 10

struct sht20_device
{
    struct rt_i2c_bus_device *i2c;

#ifdef SHT20_USING_SOFT_FILTER
    struct
    {
        float buf[SHT20_AVERAGE_TIMES];
        float average;

        rt_off_t index;
        rt_bool_t is_full;

    } temp_filter, humi_filter;

    rt_thread_t thread;
    rt_uint32_t period;
#endif /* SHT20_USING_SOFT_FILTER */

    rt_mutex_t lock;
};
typedef struct sht20_device *sht20_device_t;

enum sht20_param_type
{
    SHT20_PARAM_PRECISION,
    SHT20_PARAM_BATTERY_STATUS,
    SHT20_PARAM_HEATING,
};
typedef enum sht20_param_type sht20_param_type_t;

/**
 * This function resets all parameter with default
 *
 * @param dev the pointer of device driver structure
 *
 * @return the softreset status,RT_EOK reprensents setting successfully.
 */
rt_err_t sht20_softreset(sht20_device_t dev);

/**
 * This function initializes sht20 registered device driver
 *
 * @param dev the name of sht20 device
 *
 * @return the sht20 device.
 */
sht20_device_t sht20_init(const char *i2c_bus_name);

/**
 * This function releases memory and deletes mutex lock
 *
 * @param dev the pointer of device driver structure
 */
void sht20_deinit(sht20_device_t dev);

/**
 * This function reads temperature by sht20 sensor measurement
 *
 * @param dev the pointer of device driver structure
 *
 * @return the relative temperature converted to float data.
 */
float sht20_read_temperature(sht20_device_t dev);

/**
 * This function reads relative humidity by sht20 sensor measurement
 *
 * @param dev the pointer of device driver structure
 *
 * @return the relative humidity converted to float data.
 */
float sht20_read_humidity(sht20_device_t dev);

/**
 * This function sets parameter of sht20 sensor
 *
 * @param dev the pointer of device driver structure
 * @param type the parameter type of device
 * @param value the pointer value of type
 *
 * @return the setting parameter status,RT_EOK reprensents setting successfully.
 */
rt_err_t sht20_set_param(sht20_device_t dev, sht20_param_type_t type, rt_uint8_t value);

/**
 * This function gest parameter of sht20 sensor
 *
 * @param dev the pointer of device driver structure
 * @param type the parameter type of device
 * @param value the pointer value of type
 *
 * @return the getting parameter status,RT_EOK reprensents getting successfully.
 */
rt_err_t sht20_get_param(sht20_device_t dev, sht20_param_type_t type, rt_uint8_t *value);

#endif /* _SHT20_H__ */
