
SORT 的推演公式将展示以下信息：

| 字段名          | 类型   | 描述 |
| --------------- | ------ | ---- |
| Records         | int64  | SORT 输入记录个数的估算值 |
| SortFields      | int32  | SORT 进行排序的字段个数的估算值 |
| RecordTotalSize | int64  | SORT 输入记录总大小的估算值<br>公式为 ```Records * RecordSize``` |
| Pages           | int32  | SORT 输入记录页数的估算值（输入记录个数的总大小存放入 4K 页面中的页数）<br>公式为 ```max( 1, ceil( RecordTotalSize / PageUnit) )``` |
| SortType        | string | 估算的 SORT 排序类型<br>RecordTotalSize 小于 sortbuff 时，为内存排序"InMemory"<br>RecordTotalSize 大于 sortbuff 时，为外存排序"External" |
| IOCost          | array  | 估算的 SORT 的 IO 代价公式及计算过程<br>SortType 为"InMemory"时不需要计算<br>各个数据页需要写出磁盘，并进行归并排序，假设归并排序中 75% 为顺序读，25% 为随机读<br>公式为 ```ceil( Pages * ( SeqWrtIOCostUnit + SeqReadIOCostUnit * 0.75 + RandomReadIOCostUnit * 0.25 ) )``` |
| CPUCost         | array  | 估算的 SORT 的 CPU 代价公式及计算过程<br>即各个记录进行排序的代价<br>公式为 ```ceil( 2 * OptrCPUCost * SortFields * max( 2, Records ) * log2( max( 2, Records ) ) )``` |
| StartCost       | array  | 估算的 SORT 启动代价<br>需要计算子操作的总代价和排序的代价<br>公式为 ```ChildTotalCost + IOCPURate * IOCost + CPUCost``` |
| RunCost         | array  | 估算的 SORT 运行代价（内部表示）<br>即从排序缓存中提取各个记录的代价<br>公式为：```OptrCPUCost * Records``` |
| TotalCost       | array  | 估算的 SORT 总代价（内部表示）<br>公式为 ```StartCost + RunCost``` |
| OutputRecords   | array  | 估算的 SORT 输出记录个数<br>公式为 ```Records``` |

示例
----

```lang-json
"SortNode": {
  "Records": 1000000,
  "SortFields": 1,
  "RecordTotalSize": [
    "Records * RecordSize",
    "1000000 * 269",
    269000000
  ],
  "Pages": [
    "max( 1, ceil( RecordTotalSize / PageUnit) )",
    "max( 1, ceil( 269000000 / 4096) )",
    65674
  ],
  "SortType": "External",
  "IOCost": [
    "ceil( Pages * ( SeqWrtIOCostUnit + SeqReadIOCostUnit * 0.75 + RandomReadIOCostUnit * 0.25 ) )",
    "ceil( 65674 * ( 2 + 1 * 0.75 + 10 * 0.25 ) )",
    344789
  ],
  "CPUCost": [
    "ceil( 2 * OptrCPUCost * SortFields * max( 2, Records ) * log2( max( 2, Records ) ) )",
    "ceil( 2 * 1 * 1 * max( 2, 1000000 ) * log2( max( 2, 1000000 ) ) )",
    39863138
  ],
  "StartCost": [
    "ChildTotalCost + IOCPURate * IOCost + CPUCost",
    "160864000 + 2000 * 344789 + 39863138",
    890305138
  ],
  "RunCost": [
    "OptrCPUCost * Records",
    "1 * 1000000",
    1000000
  ],
  "TotalCost": [
    "StartCost + RunCost",
    "890305138 + 1000000",
    891305138
  ],
  "OutputRecords": [
    "Records",
    "1000000",
    1000000
  ]
}
```
