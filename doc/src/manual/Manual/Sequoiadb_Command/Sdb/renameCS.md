##名称##

renameCS - 集合空间改名

##语法##

**db.renameCS( \<oldname\>, \<newname\> )**

##类别##

Sdb

##描述##

该函数用于为集合空间改名。

##参数##

| 参数名  | 参数类型 | 描述                 | 是否必填 |
| ------  | ------   | ------               | ------ |
| oldname | 字符串   | 要修改的集合空间名。 | 是 |
| newname | 字符串   | 集合空间新名字。     | 是 |

>**Note:**
>
> * 改名过程中会阻塞相应数据节点的写操作。
> * 不允许直连数据节点，对集合空间改名。

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`renameCS`函数常见异常如下：

| [错误码](manual/Manual/Sequoiadb_error_code.md)| 错误类型 | 描述 | 解决方法 |
| ------| ----------------------- | --- | ------ |
| -34   | SDB_DMS_CS_NOTEXIST | oldname对应的集合空间不存在。 | 对已存在的集合空间执行rename。 |
| -33   | SDB_DMS_CS_EXIST    | newname对应的集合空间已存在。 | newname设为不存在的名字。 |
| -67   | SDB_BACKUP_HAS_ALREADY_START | 数据节点正在做备份。 | 等待备份完成，再执行改名。 |
| -148  | SDB_DMS_STATE_NOT_COMPATIBLE | 数据节点上有其他rename操作正在执行。 | 等待其余rename完成，再执行改名。 |
| -149  | SDB_REBUILD_HAS_ALREADY_START| 数据节点正在做rebuild。 | 等待rebuild完成，再执行改名。 |

当异常抛出时，可以通过 [getLastErrObj()](manual/Manual/Sequoiadb_Command/Global/getLastErrObj.md)  或 [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) 获取错误信息 或 通过 [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) 获取错误码。
更多错误可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md) 。

##版本##

3.0.1 及以上版本

##示例##

对集合空间sample，改名为sample_new 。

```lang-javascript
// 连接协调节点
> db = new Sdb( "localhost", 11810 )
> db.renameCS( "sample", "sample_new" )
```

