##名称##

returnItemToName - 以特定的名称恢复指定的回收站项目

##语法##

**db.getRecycleBin().returnItemToName(\<recycleName\>, \<returnName\>)**

##类别##

SdbRecycleBin

##描述##

该函数用于以特定的名称恢复指定的回收站项目。

##参数##

- recycleName（ *string，必填* ）

    需恢复的回收站项目名称

- returnName（ *string，必填* )

    项目恢复后的新名称，指定的新名称需满足以下要求：

    - 指定的名称需符合集合空间或集合的命名要求，具体要求可参考[限制][limit]
    - 对于集合类型的回收站项目，指定的名称格式应为 `<集合空间>.<集合>`，且不支持修改原集合空间名称

##返回值##

函数执行成功时，将返回一个 BSONObj 类型的对象。通过该对象获取恢复后的集合空间或集合的名称。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`returnItemToName()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决办法 |
| ------ | ------ | --- | ------ |
| -384 | SDB_RECYCLE_ITEM_NOTEXIST | 回收站项目不存在 | 通过 [SdbRecycleBin.list()][list] 检查回收站项目是否存在 |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.6 及以上版本

##示例##

1. 查看当前已存在的回收站项目

    ```lang-javascript
    > db.getRecycleBin().list()
    {
      "RecycleName": "SYSRECYCLE_9_21474836481",
      "RecycleID": 9,
      "OriginName": "sample.employee",
      "OriginID": 21474836481,
      "Type": "Collection",
      "OpType": "Drop",
      "RecycleTime": "2022-01-24-12.04.12.000000"
    }
    ```

2. 恢复名为“SYSRECYCLE_9_21474836481”的集合回收站项目，并将恢复后的集合重命名为“test”

    ```lang-javascript
    > db.getRecycleBin().returnItemToName( "SYSRECYCLE_9_21474836481", "sample.test" )
    {
      "ReturnName": "sample.test"
    }
    ```

[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[list]:manual/Manual/Sequoiadb_Command/SdbRecycleBin/list.md
[limit]:manual/Manual/sequoiadb_limitation.md
