##名称##

list - 获取列表

##语法##

**db.list(\<listType\>,[cond],[sel],[sort])**

##类别##

Sdb

##描述##

该函数用于获取指定列表，查看当前系统状态。列表是一种轻量级的命令。

##参数##

| 参数名   | 类型    | 描述   													| 是否必填 |
|----------|-------------|----------------------------------------------------------|----------|
| listType | macro       | 需要获取的列表，取值可参考[列表类型][list_info]| 是 	   |
| cond     | object      | 设置匹配条件以及[命令位置参数][location] | 否 	   |
| sel      | object      | 选择返回的字段名，为 null 时返回所有的字段名         | 否 	   |
| sort     | object      | 对返回的记录按选定的字段排序，取值如下：<br>1：升序<br>-1：降序        | 否 	   |

>**Note:**
>
>* sel 参数是一个 json 结构，如：{字段名:字段值}，字段值一般指定为空串。sel 中指定的字段名在记录中存在，设置字段值不生效；不存在则返回 sel 中指定的字段名和字段值。
>* 记录中字段值类型为数组，用户可以在 sel 中指定该字段名，用"."操作符加上双引号("")来引用数组元素。

##返回值##

函数执行成功时，将返回一个 SdbCursor 类型的对象。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v2.0 及以上版本

##示例##

* 指定 listType 的值为 SDB_LIST_CONTEXTS

    ```lang-javascript
    > db.list( SDB_LIST_CONTEXTS )
    {
      "NodeName": "ubuntu-200-043:11850",
      "SessionID": 29,
      "TotalCount": 1,
      "Contexts": [
        254
      ]
    }
    ```

* 指定 listType 的值为 SDB_LIST_STORAGEUNITS

    ```lang-javascript
    > db.list( SDB_LIST_STORAGEUNITS )
    {
      "NodeName": "ubuntu-200-043:11830",
      "Name": "sample",
      "UniqueID": 61,
      "ID": 4094,
      "LogicalID": 186,
      "PageSize": 65536,
      "LobPageSize": 262144,
      "Sequence": 1,
      "NumCollections": 1,
      "CollectionHWM": 1,
      "Size": 306315264
    }
    ```

* 返回符合条件 LogicalID 大于 1 的记录，并且每条记录只返回 Name 和 ID 这两个字段，记录按 Name 字段的值升序排序

    ```lang-javascript
    > db.list( SDB_LIST_STORAGEUNITS, { "LogicalID": { $gt: 1 } }, { Name: "", ID: "" }, { Name: 1 } )
    {
      "Name": "sample",
      "ID": 4094
    }
    ```

* 指定命令位置参数，只返回数据组 db1 的 context

    ```lang-javascript
    > db.list( SDB_LIST_CONTEXTS, { GroupName: "db1" } )
    {
      "NodeName": "ubuntu-200-043:20000",
      "SessionID": 29,
      "TotalCount": 1,
      "Contexts": [
        254
      ]
    }
    ```

[^_^]:
     本文使用的所有引用及链接
[location]:manual/Manual/Sequoiadb_Command/location.md
[list_info]:manual/Manual/List/Readme.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md