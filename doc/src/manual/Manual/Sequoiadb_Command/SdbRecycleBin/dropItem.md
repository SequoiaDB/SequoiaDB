##名称##

dropItem - 删除指定的回收站项目

##语法##

**db.getRecycleBin().dropItem(\<recycleName\>, [recursive], [options])**

##类别##

SdbRecycleBin

##描述##

该函数用于删除指定的回收站项目。

##参数##

- recycleName（ *string，必填* ）

    需删除的回收站项目名称

- recursive（ *boolean，选填* ）

    删除集合空间类型的回收站项目时，是否删除该集合空间关联的集合回收站项目，默认为 false，表示不删除关联项目并返回错误信息

- options（ *object，选填* )

    通过 options 可以设置其他选填参数：

    - Async（ *boolean* ）：是否使用异步模式删除回收站项目，默认为 false，表示不使用异步模式

        格式：`Async: true`

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`dropItem()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决办法 |
| ------ | ------ | --- | ------ |
| -384 | SDB_RECYCLE_ITEM_NOTEXIST | 回收站项目不存在 | 通过 [SdbRecycleBin.list()][list] 检查回收站项目是否存在。 |
| -385 | SDB_RECYCLE_CONFLICT | 回收站项目冲突 | 如果操作为“删除集合空间类型的回收站项目”，需指定参数 recursive 为 true |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.6 及以上版本

##示例##

1. 查看当前已存在的回收站项目

    ```lang-javascript
    > db.getRecycleBin().list()
    {
      "RecycleName": "SYSRECYCLE_8_17179869185",
      "OriginName": "sample1.employee1",
      "Type": "Collection",
      ···
    }
    {
      "RecycleName": "SYSRECYCLE_9_12884901889",
      "OriginName": "sample.employee",
      "Type": "Collection"
      ···
    }
    {
      "RecycleName": "SYSRECYCLE_10_3",
      "OriginName": "sample",
      "Type": "CollectionSpace",
      ···
    }
    ```

2. 删除名为“SYSRECYCLE_8_17179869185”的集合回收站项目

    ```lang-javascript
    > db.getRecycleBin().dropItem("SYSRECYCLE_8_17179869185")
    ```

3. 删除名为“SYSRECYCLE_10_3”的集合空间回收站项目

    ```lang-javascript
    > db.getRecycleBin().dropItem("SYSRECYCLE_10_3")
    ```

    因回收站中存在与集合空间 sample 关联的集合回收站项目，删除操作报错

    ```lang-text
    (shell):1 uncaught exception: -385
    Recycle bin item conflicts:
    Failed to drop collection space recycle item [origin sample, recycle SYSRECYCLE_10_3], there are recursive collection recycle items inside
    ```

    重新执行删除操作并指定参数 recursive 为 true

    ```lang-javascript
    > db.getRecycleBin().dropItem("SYSRECYCLE_10_3", true)
    ```

4. 确认项目删除成功

    ```lang-javascript
    > db.getRecycleBin().list()
    ```

[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[list]:manual/Manual/Sequoiadb_Command/SdbRecycleBin/list.md
