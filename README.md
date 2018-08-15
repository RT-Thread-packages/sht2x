# SHT2x 软件包

## 1、介绍

SHT2x 软件包提供了使用温度与湿度传感器 `sht20` 基本功能，并且提供了读取数据的采集时间以及数据处理基本方式。本文介绍该软件包的基本功能、实现，以及 `Finsh/MSH` 测试命令等。

### 1.1 目录结构

| 名称 | 说明 |
| ---- | ---- |
| sht20.h | 传感器使用头文件 |
| sht20.c | 传感器使用源代码 |
| SConscript | RT-Thread 默认的构建脚本 |
| README.md | 软件包使用说明 |

### 1.2 许可证

SHT2x 软件包遵循 LGPLv2.1 许可，详见 `LICENSE` 文件。

### 1.3 依赖

依赖 RT-Thread 操作系统及其组件。

## 2、如何打开 sht2x 软件包

使用 `sht2x` 软件包需要在 RT-Thread 的包管理器中选择它，具体路径如下：

```
RT-Thread online packages
    peripheral libraries and drivers  --->		
           [*] digital humidity and temperature sensor sht2x driver library  --->	      
				[*]   Enable support filter function
                      Version (latest)  --->
```


每个功能的配置说明如下：

- digital humidity and temperature sensor sht2x driver library）：使用 `sht20` 软件包；
- Enable support filter function：开启采集温湿度过滤功能；
- Version：配置软件包版本。

然后让 RT-Thread 的包管理器自动更新，或者使用 `pkgs --update` 命令更新包到 BSP 中。

## 3、使用 sht2x 软件包

按照前文介绍，获取 `sht2x` 软件包后，就可以按照下面提供的 API 使用传感器 `sht20` 。

## 3.1 API

### 3.1.1  初始化 

`sht20_device_t sht20_init(const char *i2c_bus_name)`

获取 `i2c` 设备对象，具体参数与返回说明如下表

| 参数    | 描述                      |
| :----- | :----------------------- |
| name   | i2c 设备名称 |
| **返回** | **描述** |
| != NULL | 将返回 i2c 设备对象 |
| = NULL | 查找失败 |

### 3.1.2  解初始化

void sht20_deinit(sht20_device_t dev)

释放 `i2c` 设备对象、删除锁，以及在开启温湿度过滤功能中删除线程，具体参数说明如下表

| 参数 | 描述       |
| :--- | :--------- |
| dev  | sht20 设备 |

### 3.1.3 读取温度

float sht20_read_temperature(sht20_device_t dev)

通过 `sht20` 传感器读取温度测量值，返回浮点型温度值，具体参数与返回说明如下表

| 参数     | 描述        |
| :------- | :---------- |
| dev      | `sht20`设备 |
| **返回** | **描述**    |
| != 0.0   | 测量温度值  |
| =0.0     | 测量失败    |

### 3.1.4 读取湿度

float sht20_read_temperature(sht20_device_t dev)

通过 `sht20` 传感器读取湿度测量值，返回浮点型湿度值，具体参数与返回说明如下表

| 参数     | 描述        |
| :------- | :---------- |
| dev      | `sht20`设备 |
| **返回** | **描述**    |
| != 0.0   | 测量湿度值  |
| =0.0     | 测量失败    |

### 3.1.5 设置参数

rt_err_t sht20_set_param(sht20_device_t dev, sht20_param_type_t type, rt_uint8_t value)

通过 `sht20` 传感器读取湿度测量值，返回浮点型湿度值，具体参数与返回说明如下表

| 参数      | 描述            |
| :-------- | :-------------- |
| dev       | `sht20`设备     |
| type      | `sht20`命令类型 |
| value     | 设置命令值      |
| **返回**  | **描述**        |
| RT_EOK    | 设置成功        |
| -RT_ERROR | 设置失败        |

`sht20`命令类型有三种，分别如下：

````
SHT20_PARAM_PRECISION        //设置 ADC 存储容量
SHT20_PARAM_BATTERY_STATUS   //设置电池提示状态
SHT20_PARAM_HEATING			//设置是否加热
````

### 3.1.6 读取参数

rt_err_t sht20_get_param(sht20_device_t dev, sht20_param_type_t type, rt_uint8_t *value)

通过 `sht20` 传感器读取湿度测量值，返回浮点型湿度值，具体参数与返回说明如下表

| 参数      | 描述            |
| :-------- | :-------------- |
| dev       | `sht20`设备     |
| type      | `sht20`命令类型 |
| value     | 获取命令值      |
| **返回**  | **描述**        |
| RT_EOK    | 获取成功        |
| -RT_ERROR | 获取失败        |

## 3.2 Finsh/MSH 测试命令

sht2x 软件包提供了丰富的测试命令，项目只要在 RT-Thread 上开启 Finsh/MSH 功能即可。在做一些基于 `sht2x` 的应用开发、调试时，这些命令会非常实用。它可以准确的读取指传感器测量的温度与湿度。

具体功能如下：输入 `sht20` 可以看到完整的命令列表

```
msh />sht20
Usage:
sht20 probe <dev_name>   - probe sensor by given name
sht20 read               - read sensor sht20 data
msh />
```

### 3.2.1 探测指定待操作 `i2c` 设备 

当第一次使用 `sht20` 命令时，直接输入 `sht20 probe`  将会显示提示输入 `i2c` 总线的名称，包括提示信息，日志如下：

```
msh />sht20 probe    
sht20 probe <dev_name>   - probe sensor by given name
msh />
```

### 3.2.2 读取数据

探测之后，输入 `sht20 read` 即可获取温度与湿度，包括提示信息，日志如下： 

```
msh />sht20 read
read sht20 sensor humidity   : 54.7 %
read sht20 sensor temperature: 27.3 
msh />
```

## 4、注意事项

暂无。

## 5、联系方式

* 维护：[Ernest](https://github.com/ErnestChen1)
* 主页：https://github.com/RT-Thread-packages/sht2x

