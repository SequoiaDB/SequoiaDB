
回收站项目列表可以列出当前[回收站][recycle_bin]中已回收项目的元数据信息。

##标识##

$LIST_RECYCLEBIN

##字段信息##

| 字段名          | 类型       | 描述 |
| --------------- | ---------- | ---- |
| RecycleName     | string     | 回收站项目的名称 |
| RecycleID       | int64      | 回收站项目的唯一标识 |
| OriginID        | int64      | 已回收的集合空间或集合的唯一标识   |
| OriginName      | string     | 已回收的集合空间或集合的名称 |
| Type            | string     | 回收站项目的类型，取值如下：<br>"CollectionSpace"：集合空间<br>"Collection" ：集合 |
| OpType          | string     | 回收站项目的操作类型，取值如下：<br>"Drop"：删除集合空间或集合操作<br>"Truncate"：删除数据操作 |
| RecycleTime     | string     | 生成回收站项目的时间 |

##示例##

查看回收站项目列表

```lang-javascript
> db.exec("select * from $LIST_RECYCLEBIN")
```

输出结果如下：

```lang-json
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


[^_^]:
    本文使用的所有引用及链接
[recycle_bin]:manual/Distributed_Engine/Maintainance/recycle_bin.md