##名称##

getRG - 获取指定复制组

##语法##

**db.getRG(\<name\>|\<id\>)**

##类别##

Sdb

##描述##

该函数用于获取指定复制组。

##参数##

| 参数名 | 类型   | 描述 | 是否必填 |
| ------ | ------ | ------ | ------ |
| name | string | 复制组名，同一个数据库对象中，复制组名唯一 | name 和 id 任选一个 |
| id | number | 复制组 ID，创建复制组时系统自动分配 | id 和 name 任选一个 |

> **Note:**
>
> name 字段的值不能是空串，不能含点（.）或者美元符号（$），且长度不超过 127B。

##返回值##

函数执行成功时，将返回一个 SdbReplicaGroup 类型的对象。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v2.0 及以上版本

##示例##

* 指定 name 值，返回复制组 group1 的引用

    ```lang-javascript
    > db.getRG("group1")
    ```

* 指定 id 值，返回复制组 group1 的引用（假定 group1 的复制组 ID 为1000）

    ```lang-javascript
    > db.getRG(1000)
    ```


[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md