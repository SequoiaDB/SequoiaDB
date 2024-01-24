##名称##

listProcess - 列出进程的信息

##语法##

**System.listProcess( \[options\], \[filter\] )**

##类别##

System

##描述##

列出进程的信息

##参数##

| 参数名    | 参数类型 | 默认值 | 描述                          | 是否必填 |
| --------- | -------- | --- | ------------------------------------- | -------- |
| options   | JSON     | 默认不显示详细信息 | 显示模式         | 否       |
| filter    | JSON     |  默认显示全部内容 | 筛选条件 | 否       |

options 参数详细说明如下：

| 属性     | 值类型 | 是否<br>必填 | 格式 | 描述 |
| -------- | ------ | -------- | -------------------- | ------------------- |
| detail    | Bool |     否   | { detail: true }     | 是否显示详细信息        |

> Note：

> filter 参数支持对结果中的某些字段进行 and 、 or 、not 和精确匹配计算，对结果集进行筛选。

##返回值##

返回进程的信息。

##错误##

如果出错则抛异常，并输出错误信息，可以通过 [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) 获取错误信息或通过 [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) 获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##版本##

v3.2 及以上版本

##示例##

* 列出所有进程的信息

    ```lang-javascript
    > System.listProcess()
    {
        "pid": "30571",
        "cmd": "sequoiadb(50000) S"
    }
    {
        "pid": "30834",
        "cmd": "bin/sdb"
    }
    {
        "pid": "30876",
        "cmd": "/usr/sbin/rsyslogd -n"
    }
    ...
    ```

* 对结果进行筛选

    ```lang-javascript
    > System.listProcess( { detail: true }, { "user": "sdbadmin" } )
    {
        "user": "sdbadmin",
        "pid": "20630",
        "status": "S",
        "cmd": "sleep 1"
    }
    {
        "user": "sdbadmin",
        "pid": "25681",
        "status": "Sl",
        "cmd": "sdbom(11780)"
    }
    ```