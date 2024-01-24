## 描述

数据源列表可以列出当前数据库中所有数据源的元数据信息。

## 标识

SDB_LIST_DATASOURCES

## 字段信息

返回的字段信息可参考 [SYSCAT.SYSDATASOURCES 集合][SYSDATASOURCES]。

## 示例

查看所有数据源的元数据信息

```lang-javascript
> db.list(SDB_LIST_DATASOURCES)
```

输出结果如下：

```lang-json
{
  "_id": {
    "$oid": "5ffc365c72e60c4d9be30c50"
  },
  "ID": 2,
  "Name": "datasource1",
  "Type": "SequoiaDB",
  "Version": 0,
  "DSVersion": "3.4.1",
  "Address": "sdbserver:11810",
  "User": "",
  "Password": "d41d8cd98f00b204e9800998ecf8427e",
  "ErrorControlLevel": "High",
  "AccessMode": 1,
  "AccessModeDesc": "READ",
  "ErrorFilterMask": 0
  "ErrorFilterMaskDesc": "NONE"
}
```



[^_^]:
     本文使用的所有引用及链接
[SYSDATASOURCES]:manual/Manual/Catalog_Table/SYSDATASOURCES.md