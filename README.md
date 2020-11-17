# SHT2x package

[中文页](README_ZH.md) | English

## 1. Introduction

The SHT2x software package provides the basic function of using the temperature and humidity sensor `sht20`, as well as the optional function of the software average filter. This article introduces the basic functions of the software package and the `Finsh/MSH` test commands.

The basic functions are mainly determined by the sensor `aht10`: within the range of input voltage `1.8v-3.3v`, the measuring range and accuracy of temperature and humidity are shown in the table below

| Function | Range | Accuracy |
| ---- | ------------- | ------- |
| Temperature | `-40℃-125℃` | `±0.3℃` |
| Humidity | `0%-100%` | `±3%` |

### 1.1 Directory structure

| Name | Description |
| ---- | ---- |
| sht20.h | Sensor header file |
| sht20.c | Sensor use source code |
| SConscript | RT-Thread default build script |
| README.md | Package Instructions |
| SHT20_datasheet.pdf| Official Data Sheet |

### 1.2 License

The SHT2x software package complies with the Apache-2.0 license, see the LICENSE file for details.

### 1.3 Dependency

Rely on `RT-Thread I2C` device driver framework.

## 2 Get the package

To use the `sht2x` package, you need to select it in the package manager of RT-Thread. The specific path is as follows:

```
RT-Thread online packages
    peripheral libraries and drivers --->
        [*] sht2x: digital humidity and temperature sensor sht2x driver library --->
        [*] Enable average filter by software
        (10) The number of averaging
        (1000) Peroid of sampling data(unit ms)
               Version (latest) --->
```


The configuration instructions for each function are as follows:

- `sht2x: digital humidity and temperature sensor sht2x driver library`: choose to use the `sht20` software package;
- `Enable average filter by software`: enable the average filter function of the software for collecting temperature and humidity;
- `The number of averaging`: the number of samples to average;
- `Peroid of sampling data(unit ms)`: The period of sampling data, the time unit is `ms`;
- `Version`: Configure the software package version, the latest version is the default.

Then let RT-Thread's package manager automatically update, or use the `pkgs --update` command to update the package to the BSP.

## 3 Use sht2x package

According to the previous introduction, after obtaining the `sht2x` software package, you can use the sensor `sht20` and `Finsh/MSH` commands to test according to the API provided below. The details are as follows.

### 3.1 API

#### 3.1.1 Initialization

```
sht20_device_t sht20_init(const char *i2c_bus_name)
```

According to the bus name, the corresponding SHT20 device is automatically initialized. The specific parameters and return description are as follows

| Parameters | Description |
| :----- | :----------------------- |
| name | i2c device name |
| **Back** | **Description** |
| != NULL | will return sht20 device object |
| = NULL | Find failed |

#### 3.1.2 Deinitialization

```
void sht20_deinit(sht20_device_t dev)
```

If the device is no longer used, deinitialization will reclaim the relevant resources of the sht20 device. The specific parameters are described in the following table

| Parameters | Description |
| :--- | :------------- |
| dev | sht20 device object |

#### 3.1.3 Read temperature

```
float sht20_read_temperature(sht20_device_t dev)
```

Read the temperature measurement value through the `sht20` sensor, and return the floating-point temperature value. The specific parameters and return description are as follows

| Parameters | Description |
| :------- | :------------- |
| dev | sht20 device object |
| **Back** | **Description** |
| != 0.0 | Measuring temperature value |
| =0.0 | Measurement failed |

#### 3.1.4 Read humidity

```
float sht20_read_humidity(sht20_device_t dev)
```

Read the humidity measurement value through the `sht20` sensor and return the floating-point humidity value. The specific parameters and return description are as follows

| Parameters | Description |
| :------- | :-------------- |
| dev | `sht20` device object |
| **Back** | **Description** |
| != 0.0 | Measuring humidity value |
| =0.0 | Measurement failed |

#### 3.1.5 Setting parameters

```
rt_err_t sht20_set_param(sht20_device_t dev, sht20_param_type_t type, rt_uint8_t value)
```

Use the `sht20` parameter command to set the value of related parameters. The specific parameters and return description are as follows

| Parameters | Description |
| :-------- | :-------------- |
| dev | `sht20` device object |
| type | `sht20` command type |
| value | Set command value |
| **Back** | **Description** |
| RT_EOK | Set successfully |
| -RT_ERROR | Setting failed |

There are three types of `sht20` commands, as follows:

````
SHT20_PARAM_PRECISION         //Set ADC conversion digits
SHT20_PARAM_BATTERY_STATUS    //Set the battery reminder status
SHT20_PARAM_HEATING           //Set whether to heat
````

There are four types of ADC conversion bits, which are represented as follows

```
SHT20_RES_12_14BIT 0x00  // Humidity 12 bits, temperature 14 bits (default)
SHT20_RES_8_12BIT 0x01   // 8 bits for humidity, 12 bits for temperature
SHT20_RES_10_13BIT 0x80  // 10 bits for humidity, 12 bits for temperature
SHT20_RES_11_11BIT 0x81  // Humidity 11 bits, temperature 11 bits
```

#### 3.1.6 Read parameters

```
rt_err_t sht20_get_param(sht20_device_t dev, sht20_param_type_t type, rt_uint8_t *value)
```

Through the `sht20` parameter command, read the value of related parameters, the specific parameters and return description are as follows

| Parameters | Description |
| :-------- | :-------------- |
| dev | `sht20` device object |
| type | `sht20` command type |
| value | Get command value |
| **Back** | **Description** |
| RT_EOK | Successfully obtained |
| -RT_ERROR | Get failed |

### 3.2 Finsh/MSH test commands

The sht2x software package provides a wealth of test commands, and the project only needs to enable the Finsh/MSH function on RT-Thread. When doing some application development and debugging based on `sht2x`, these commands will be very useful. It can accurately read the temperature and humidity measured by the sensor. For specific functions, you can enter `sht20` to view the complete command list

```
msh />sht20
Usage:
sht20 probe <dev_name>-probe sensor by given name
sht20 read-read sensor sht20 data
msh />
```

#### 3.2.1 Detect the sensor on the specified i2c bus

When using the `sht20` command for the first time, directly enter `sht20 probe <dev_name>`, where `<dev_name>` is the designated i2c bus, for example: i2c0`. If there is this sensor, no error will be prompted; if there is no such sensor on the bus, it will display a prompt that the relevant device cannot be found, the log is as follows:

```
msh />sht20 probe i2c1 #Detection is successful, no error log
msh />
msh />sht20 probe i2c88 #Detection failed, prompting that the corresponding I2C device could not be found
[E/sht20] can't find sht20 device on'i2c88'
msh />
```

#### 3.2.2 Read data

After the detection is successful, enter `sht20 read` to get the temperature and humidity, including prompt information, the log is as follows:

```
msh />sht20 read
read sht20 sensor humidity: 54.7%
read sht20 sensor temperature: 27.3
msh />
```

## 4 Notes

Nothing.

## 5 Contact

* Maintenance: [Ernest](https://github.com/ErnestChen1)
* Homepage: https://github.com/RT-Thread-packages/sht2x
