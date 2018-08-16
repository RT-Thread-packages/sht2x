# SHT2x 软件包

## 1 介绍

SHT2x 软件包提供了使用温度与湿度传感器 `sht20` 基本功能，并且提供了读取数据的采集时间以及数据处理基本方式。本文介绍该软件包的基本功能，以及 `Finsh/MSH` 测试命令等。

### 1.1 目录结构

| 名称 | 说明 |
| ---- | ---- |
| sht20.h | 传感器使用头文件 |
| sht20.c | 传感器使用源代码 |
| SConscript | RT-Thread 默认的构建脚本 |
| README.md | 软件包使用说明 |

### 1.2 许可证

SHT2x 软件包遵循  Apache-2.0 许可，详见 LICENSE 文件。

### 1.3 依赖

依赖 `RT-Thread I2C` 设备驱动框架。

## 2 获取软件包

使用 `sht2x` 软件包需要在 RT-Thread 的包管理器中选择它，具体路径如下：

```
RT-Thread online packages
    peripheral libraries and drivers  --->
        [*] sht2x: digital humidity and temperature sensor sht2x driver library  --->
        [*]    Enable support filter function
               Version (latest)  --->
```


每个功能的配置说明如下：

- `sht2x: digital humidity and temperature sensor sht2x driver library`：选择使用 `sht20` 软件包；
- `Enable support filter function`：开启采集温湿度软件平均数滤波器功能；
- `Version`：配置软件包版本，默认最新版本。

然后让 RT-Thread 的包管理器自动更新，或者使用 `pkgs --update` 命令更新包到 BSP 中。

## 3 使用 sht2x 软件包

按照前文介绍，获取 `sht2x` 软件包后，就可以按照 下文提供的 API 使用传感器 `sht20` 与 `Finsh/MSH` 命令进行测试，详细内容如下。

### 3.1 API

#### 3.1.1  初始化 

`sht20_device_t sht20_init(const char *i2c_bus_name)`

根据总线名称，自动初始化对应的 SHT20 设备，具体参数与返回说明如下表

| 参数    | 描述                      |
| :----- | :----------------------- |
| name   | i2c 设备名称 |
| **返回** | **描述** |
| != NULL | 将返回 sht20 设备对象 |
| = NULL | 查找失败 |

#### 3.1.2  反初始化

void sht20_deinit(sht20_device_t dev)

如果设备不再使用，反初始化将回收 sht20 设备的相关资源，具体参数说明如下表

| 参数 | 描述           |
| :--- | :------------- |
| dev  | sht20 设备对象 |

#### 3.1.3 读取温度

float sht20_read_temperature(sht20_device_t dev)

通过 `sht20` 传感器读取温度测量值，返回浮点型温度值，具体参数与返回说明如下表

| 参数     | 描述           |
| :------- | :------------- |
| dev      | sht20 设备对象 |
| **返回** | **描述**       |
| != 0.0   | 测量温度值     |
| =0.0     | 测量失败       |

#### 3.1.4 读取湿度

float sht20_read_humidity(sht20_device_t dev)

通过 `sht20` 传感器读取湿度测量值，返回浮点型湿度值，具体参数与返回说明如下表

| 参数     | 描述            |
| :------- | :-------------- |
| dev      | `sht20`设备对象 |
| **返回** | **描述**        |
| != 0.0   | 测量湿度值      |
| =0.0     | 测量失败        |

#### 3.1.5 设置参数

rt_err_t sht20_set_param(sht20_device_t dev, sht20_param_type_t type, rt_uint8_t value)

通过 `sht20` 参数命令，设置相关参数的值，具体参数与返回说明如下表

| 参数      | 描述            |
| :-------- | :-------------- |
| dev       | `sht20`设备对象 |
| type      | `sht20`命令类型 |
| value     | 设置命令值      |
| **返回**  | **描述**        |
| RT_EOK    | 设置成功        |
| -RT_ERROR | 设置失败        |

`sht20`命令类型有三种，分别如下：

````
SHT20_PARAM_PRECISION        //设置 ADC 转换位数
SHT20_PARAM_BATTERY_STATUS   //设置电池提示状态
SHT20_PARAM_HEATING          //设置是否加热
````

对于 ADC 存储容量，有四种类型，分别表示如下

```
SHT20_RES_12_14BIT 0x00 // 湿度12位，温度14位(默认)
SHT20_RES_8_12BIT 0x01  // 湿度8位，温度12位
SHT20_RES_10_13BIT 0x80 // 湿度10位，温度12位
SHT20_RES_11_11BIT 0x81 // 湿度11位，温度11位
```

#### 3.1.6 读取参数

rt_err_t sht20_get_param(sht20_device_t dev, sht20_param_type_t type, rt_uint8_t *value)

通过 `sht20` 参数命令，读取相关参数的值，具体参数与返回说明如下表

| 参数      | 描述            |
| :-------- | :-------------- |
| dev       | `sht20`设备对象 |
| type      | `sht20`命令类型 |
| value     | 获取命令值      |
| **返回**  | **描述**        |
| RT_EOK    | 获取成功        |
| -RT_ERROR | 获取失败        |

### 3.2 Finsh/MSH 测试命令

sht2x 软件包提供了丰富的测试命令，项目只要在 RT-Thread 上开启 Finsh/MSH 功能即可。在做一些基于 `sht2x` 的应用开发、调试时，这些命令会非常实用，它可以准确的读取指传感器测量的温度与湿度。具体功能可以输入 `sht20` ，可以查看完整的命令列表

```
msh />sht20
Usage:
sht20 probe <dev_name>   - probe sensor by given name
sht20 read               - read sensor sht20 data
msh />
```

#### 3.2.1 在指定的 i2c 总线上探测传感器 

当第一次使用 `sht20` 命令时，直接输入 `sht20 probe <dev_name>` ，其中 `<dev_name>` 为指定的 i2c 总线，例如：i2c0`。如果有这个传感器，就不会提示错误；如果总线上没有这个传感器，将会显示提示找不到相关设备，日志如下：

```
msh />sht20 probe i2c1      #探测成功，没有错误日志
msh />
msh />sht20 probe i2c88     #探测失败，提示对应的 I2C 设备找不到
[E/sht20] can't find sht20 device on 'i2c88'
msh />
```

#### 3.2.2 读取数据

探测成功之后，输入 `sht20 read` 即可获取温度与湿度，包括提示信息，日志如下： 

```
msh />sht20 read
read sht20 sensor humidity   : 54.7 %
read sht20 sensor temperature: 27.3 
msh />
```

## 4 注意事项

暂无。

## 5 联系方式

* 维护：[Ernest](https://github.com/ErnestChen1)
* 主页：https://github.com/RT-Thread-packages/sht2x

