##名称##

truncate - 删除集合内所有数据

##语法##

**db.collectionspace.collection.truncate([options])**

##类别##

SdbCollection

##描述##

该函数用于删除集合内所有数据（包括普通文档和 LOB 数据），但不会影响其元数据。与 remove 需要按照条件筛选目标不同，truncate 会直接释放数据页，在清空集合（尤其是大数据量下）数据时效率比 remove 更加高效。

> **Note:** 
> 
> 如有自增字段，truncate 后字段序列值将会重置。

##参数##

options（ *object，选填* ）

通过 options 可以设置其他选填参数：

- SkipRecycleBin（ *boolean* ）：是否禁用[回收站][recycle_bin]机制，默认是 false，表示根据字段 [Enable][getDetail] 的值决定是否启用回收站机制

    该参数取值为 true，表示删除集合时将不生成对应的回收站项目。

    格式：`SkipRecycleBin: true`

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`truncate()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决办法 |
| ------ | ------ | --- | ------ |
| -23 | SDB_DMS_NOTEXIST | 集合不存在| 检查集合是否存在|
| -386 | SDB_RECYCLE_FULL | 回收站已满 | 检查回收站是否已满，并通过 [dropItem()][dropItem] 或 [dropAll()][dropAll] 手动清理回收站 |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v2.0 及以上版本

##示例##

- 集合 sample.employee 中插入了普通数据和 LOB 数据，通过快照查看其数据页使用情况

    ```lang-javascript
    > db.snapshot(SDB_SNAP_COLLECTIONS)
    {
      "Name": "sample.employee",
      "Details": [
        {
          "GroupName": "datagroup",
          "Group": [
            {
              "ID": 0,
              "LogicalID": 0,
              "Sequence": 1,
              "Indexes": 1,
              "Status": "Normal",
              "TotalRecords": 10000,
              "TotalDataPages": 33,
              "TotalIndexPages": 7,
              "TotalLobPages": 36,
              "TotalDataFreeSpace": 41500,
              "TotalIndexFreeSpace": 103090
            }
          ]
        }
      ]
    }
    ```

- 上例中可以看到其中数据页为 33，索引页为 7，LOB 页为 36，下面执行 truncate 操作

    ```lang-javascript
    > db.sample.employee.truncate()
    ```

- 再次通过快照查看数据页使用情况，可以查看除索引页为 2（存储了索引的元数据信息）外，其余数据页已经全部被释放了

    ```lang-javascript
    > db.snapshot(SDB_SNAP_COLLECTIONS)
    {
      "Name": "sample.employee",
      "Details": [
        {
          "GroupName": "datagroup",
          "Group": [
            {
              "ID": 0,
              "LogicalID": 0,
              "Sequence": 1,
              "Indexes": 1,
              "Status": "Normal",
              "TotalRecords": 0,
              "TotalDataPages": 0,
              "TotalIndexPages": 2,
              "TotalLobPages": 0,
              "TotalDataFreeSpace": 0,
              "TotalIndexFreeSpace": 65515
            }
          ]
        }
      ]
    }
    ```

[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[recycle_bin]:manual/Distributed_Engine/Maintainance/recycle_bin.md
[dropItem]:manual/Manual/Sequoiadb_Command/SdbRecycleBin/dropItem.md
[dropAll]:manual/Manual/Sequoiadb_Command/SdbRecycleBin/dropAll.md
[getDetail]:manual/Manual/Sequoiadb_Command/SdbRecycleBin/getDetail.md
