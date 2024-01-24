##名称##

listGroups - 列出用户组的信息

##语法##

**System.listGroups( \[options\], \[filter\] )**

##类别##

System

##描述##

列出用户组的信息

##参数##

| 参数名    | 参数类型 | 默认值 | 描述                                     | 是否必填 |
| --------- | -------- | ------------- | ----------------------- | -------- |
| options   | JSON     | 默认不显示详细信息 | 显示模式      | 否       |
| filter    | JSON     | 默认显示所有用户组 | 筛选条件 | 否       |

options 参数详细说明如下：

| 属性     | 值类型 | 是否<br>必填 | 格式 | 描述 |
| -------- | ------ | -------- | -------------------- | ---------------------------------- |
| detail    | Bool |     否   | { detail: true }     | 是否显示详细信息                        |

> Note：

> filter 参数支持对结果中的某些字段进行 and 、 or 、not 和精确匹配计算，对结果集进行筛选。

##返回值##

返回用户组的信息。

##错误##

如果出错则抛异常，并输出错误信息，可以通过 [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) 获取错误信息或通过 [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) 获取错误码。关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##版本##

v3.2 及以上版本

##示例##

* 列出所有用户组的信息

    ```lang-javascript
    > System.listGroups()
    {
        "name": "sequoiadb"
    }
    {
        "name": "lpadmin"
    }
    {
        "name": "sambashare"
    }
    {
        "name": "sdbadmin_group"
    }
    ...
    ```

* 对结果进行筛选

    ```lang-javascript
    > System.listGroups( { detail: true }, { "name": "sequoiadb" } )
    {
        "name": "sequoiadb",
        "gid": "1000",
        "members": ""
    }
    ```


