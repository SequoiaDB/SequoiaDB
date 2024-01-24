
SYSGTS.SEQUENCES 集合中包含了该集群中所有的自增字段信息，每个自增字段保存为一个文档。

每个文档包含以下字段：

|  字段名      |   类型  |    描述                                     |
|--------------|---------|------------------------------------------   |
| Name         | string  | 自增字段名                                  |
| AcquireSize  | number  | 协调节点每次获取的序列值的数量，可参考 [AcquireSize][sequence]     |
| CacheSize    | number  | 编目节点每次缓存的序列值的数量，取值须大于0 |
| CurrentValue | number  | 自增字段的当前值，可参考 [CurrentValue][sequence]    |
| Cycled       | boolean | 序列值达到最大值或最小值时是否允许循环 <br> true：允许循环 <br> false：不允许循环 | 
| ID           | number  | 自增字段 ID |
| Increment    | number  | 自增字段每次增加的间隔，可参考 [Increment][sequence]   |
| Initial      | boolean | 序列是否已经分配过序列值 <br> true：未分配过序列值 <br> false：已分配过序列值    |
| Internal     | boolean | 自增字段由系统内部定义还是由用户定义 <br> true：系统内部定义 <br> false：用户定义（由用户定义的自增字段暂未开放）                             |
| MaxValue     | number  | 自增字段的最大值，可参考 [MaxValue][sequence]  |
| MinValue     | number  | 自增字段的最小值                            |
| StartValue   | number  | 自增字段的起始值                            |
| Version      | number  | 自增字段的版本号                            |

##示例##

一个典型的自增字段信息如下：

```lang-json
{
  "AcquireSize": 1000,
  "CacheSize": 1000,
  "CurrentValue": 1001,
  "Cycled": false,
  "ID": 3,
  "Increment": 1,
  "Initial": false,
  "Internal": true,
  "MaxValue": {
    "$numberLong": "9223372036854775807"
  },
  "MinValue": 1,
  "Name": "SYS_4294967303_studentID_SEQ",
  "StartValue": 1,
  "Version": 0,
  "_id": {
    "$oid": "5ea7e6bbd200b5897ef049ce"
  }
}
```


[^_^]:
    本文使用的所有引用及链接
[sequence]:manual/Distributed_Engine/Architecture/Data_Model/sequence.md#自增字段