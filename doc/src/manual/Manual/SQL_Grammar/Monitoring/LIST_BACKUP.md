
备份列表可以列出当前数据库的备份信息。

##标识##

$LIST_BACKUP

##字段信息##

| 字段名 | 类型   | 描述       |
| ------ | ------ | ---------- |
| Version | int32   | 版本号      |
| Name   | string | 备份名称   |
| ID     |  int32 | 备份 ID             |
| NodeName  | string | 节点主机名称       |
| GroupName  | string   | 数据组名称             |
| EnsureInc  | boolean | 是否开启增量备份                     |
| BeginLSNOffset | int64 | 起始 LSN 的偏移              |
| EndLSNOffset   | int64 | 结尾 LSN 的偏移              |
| TransLSNOffset | int64 | 事务当前的日志 LSN 的偏移               |
| StartTime      | string | 备份开始时间                     |
| EndTime        | string | 备份结束时间                     |
| UseTime        | int32   | 备份所花的时间                     |
| CSNum          | int32   | 备份的集合空间数量     |
| DataFileNum    | int32   | 备份的文件数量     |
| BeginDataFileSeq  | int32   | 备份的文件起始序号     |
| LastDataFileSeq   | int32   | 备份的文件结束序号     |
| LastExtentID      | int64   | 上一个数据块的 ID     |
| DataSize       | int64    | 备份的数据大小     |
| ThinDataSize   | int64    | 备份数据中未压缩的数据大小     |
| CompressedDataSize | int64 | 备份数据中已压缩的数据大小    |
| CompressedRatio | double   | 压缩率    |
| CompressionType | string   | 压缩类型    |
| LastLSN        | int64 | 最后的日志 LSN     |
| LastLSNCode    | int32   | LastLSN 的哈希值   |
| HasError       | boolean | 是否有错误      |

##示例##

查看备份列表

```lang-javascript
> db.exec( "select * from $LIST_BACKUP" )
```

输出结果如下：

```lang-json
{
  "Version": 2,
  "Name": "2019-08-14-13:27:02",
  "ID": 0,
  "NodeName": "u1604-ljh:42000",
  "GroupName": "db2",
  "EnsureInc": false,
  "BeginLSNOffset": 6645140616,
  "EndLSNOffset": 6645140668,
  "TransLSNOffset": -1,
  "StartTime": "2019-08-14-13:27:02",
  "EndTime": "2019-08-14-13:27:02",
  "UseTime": 0,
  "CSNum": 2,
  "DataFileNum": 1,
  "BeginDataFileSeq": 1,
  "LastDataFileSeq": 1,
  "LastExtentID": 24,
  "DataSize": 3725093,
  "ThinDataSize": 534118400,
  "CompressedDataSize": 74816015,
  "CompressedRatio": 0,
  "CompressionType": "snappy",
  "LastLSN": 6645140616,
  "LastLSNCode": -1200110631,
  "HasError": false
}
...
```

