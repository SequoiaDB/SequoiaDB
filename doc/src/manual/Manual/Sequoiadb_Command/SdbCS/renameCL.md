##名称##

renameCL - 修改集合名

##语法##

**db.collectionspace.renameCL( \<oldname\>, \<newname\> )**

##类别##

Collection Space

##描述##

该函数用于对指定集合空间下已存在的集合进行重命名，在重命名的过程中会阻塞相应数据节点的写操作。

用户在使用该函数时，不允许直接连接数据节点对集合进行改名。

##参数##

- **oldname**（ *String，必填* ）

  需要修改的集合名

- **newname**（ *String，必填* ）

  修改后的集合名

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛出异常并输出错误信息。

##错误##

常见异常如下：

| [错误码](manual/Manual/Sequoiadb_error_code.md)| 错误类型 | 描述 | 解决方法 |
| ------| ----------------------- | --- | ------ |
| -23   | SDB_DMS_NOTEXIST | oldname对应的集合不存在。 | 对已存在的集合执行rename。 |
| -22   | SDB_DMS_EXIST    | newname对应的集合已存在。 | newname设为不存在的名字。 |
| -67   | SDB_BACKUP_HAS_ALREADY_START | 数据节点正在做备份。 | 等待备份完成，再执行改名。 |
| -148  | SDB_DMS_STATE_NOT_COMPATIBLE | 数据节点上有其他rename操作正在执行。 | 等待其余rename完成，再执行改名。 |
| -149  | SDB_REBUILD_HAS_ALREADY_START| 数据节点正在做rebuild。 | 等待rebuild完成，再执行改名。 |

当异常抛出时，可以通过 [getLastErrObj()](manual/Manual/Sequoiadb_Command/Global/getLastErrObj.md)  或 [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) 获取错误信息 或 通过 [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) 获取错误码。
更多错误可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md) 。

##版本##

v3.0.1 及以上版本

##示例##

将集合 sample.employee，改名为 sample.employee_new

```lang-javascript
> db = new Sdb( "localhost", 11810 )    // 连接协调节点
> db.sample.renameCL( "employee", "employee_new" )
```

