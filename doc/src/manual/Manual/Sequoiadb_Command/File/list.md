##名称##

list - 列出当前目录的文件

##语法##

**File.list(\[options\],\[filter\])**

##类别##

File

##描述##

列出当前目录的文件

##参数##

| 参数名  | 参数类型 | 描述                                     | 是否必填 |
| ------- | -------- | ---------------------------------------- | -------- |
| options | JSON     | 可选参数                                 | 否       |
| filter  | JSON     | 筛选条件，不指定筛选条件默认显示全部内容 | 否       |

options 参数详细说明如下：

| 属性     | 值类型  | 描述             | 是否必填 |
| -------- | ------- | ---------------- | -------- |
| detail   | boolean | 是否显示详细内容 | 否       |
| pathname | string  | 文件路径         | 否       |


参数 filter 支持对结果中的某些字段进行 and 、 or 、not 和精确匹配计算，对结果集进行筛选。

##返回值##

返回指定目录下的文件信息。

##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##版本##

v3.2 及以上版本

##示例##

* 列出当前目录的文件

    ```lang-javascript
    > File.list( { detail: true, pathname: "/opt/sequoiadb" } )
    {
        "name": "file1.txt",
        "size": "20480",
        "mode": "drwxr-xr-x",
        "user": "root",
        "group": "root",
        "lasttime": "6月 11 11:58"
    }
    {
        "name": "file2.txt",
        "size": "20480",
        "mode": "drwxr-xr-x",
        "user": "root",
        "group": "root",
        "lasttime": "6月 12 12:58"
    }
    {
        "name": "file3.txt",
        "size": "20480",
        "mode": "drwxr-xr-x",
        "user": "root",
        "group": "root",
        "lasttime": "6月 13 13:58"
    }
    ```

* 列出当前目录的文件后，对结果进行筛选

    ```lang-javascript
    > File.list( { detail: true, pathname: "/opt/sequoiadb" }, { $and: [ { name: "file1" }, { size: "20480" } ] } )
    {
        "name": "file1.txt",
        "size": "20480",
        "mode": "drwxr-xr-x",
        "user": "root",
        "group": "root",
        "lasttime": "6月 13 13:58"
    }
    ```
