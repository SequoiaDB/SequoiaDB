##名称##

returnItem - 恢复指定的回收站项目

##语法##

**db.getRecycleBin().returnItem(\<recycleName\>, [options])**

##类别##

SdbRecycleBin

##描述##

该函数用于恢复指定的回收站项目。

##参数##

* recycleName（ *string，必填* ）

    需恢复的回收站项目名称

* options（ *object，选填* ）

    通过 options 可以设置其他选填参数：

    - Enforced（ *boolean* ）：是否删除冲突的集合或集合空间，默认为 false，表示对冲突进行报错处理

        该参数取值为 true 表示在恢复回收站项目时，如果存在相同名称或相同 uniqueID 的集合或集合空间，将强制删除原有的集合或集合空间，以解决冲突。

        格式：`Enforced: true`

##返回值##

函数执行成功时，将返回一个 BSONObj 类型的对象。通过该对象获取恢复后的集合空间或集合的名称。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`returnItem()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决办法 |
| ------ | ------ | --- | ------ |
| -384 | SDB_RECYCLE_ITEM_NOTEXIST | 回收站项目不存在 | 通过 [SdbRecycleBin.list()][list] 检查回收站项目是否存在 |
| -385 | SDB_RECYCLE_CONFLICT | 回收站项目冲突 | 通过 [returnItemToName()][returnItemToName] 将需要恢复的项目重命名，或指定参数 Enforced 为 true |

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

2. 恢复名为“SYSRECYCLE_9_21474836481”的回收站项目

    ```lang-javascript
    > db.getRecycleBin().returnItem("SYSRECYCLE_9_21474836481")
    {
      "ReturnName": "sample.employee"
    }
    ```

[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[list]:manual/Manual/Sequoiadb_Command/SdbRecycleBin/list.md
[returnItemToName]:manual/Manual/Sequoiadb_Command/SdbRecycleBin/returnItemToName.md
