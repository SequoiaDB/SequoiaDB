
IXSCAN 的推演公式将展示以下信息：

| 字段名            | 类型   | 描述 |
| ----------------- | ------ | ---- |
| IndexPages        | int32  | 估算的 IXSCAN 输入的索引页数 |
| IndexLevels       | int32  | 估算的 IXSCAN 输入的索引层数 |
| MthSelectivity    | int32  | 估算的 IXSCAN 使用匹配符进行过滤的选择率 |
| MthCPUCost        | int32  | 估算的 IXSCAN 使用匹配符过滤一个记录的 CPU 代价 |
| IXScanSelectivity | double | 估算的 IXSCAN 使用索引时需要扫描索引的比例 |
| IXPredSelectivity | double | 估算的 IXSCAN 使用索引进行过滤的选择率 |
| PredCPUCost       | int32  | 估算的 IXSCAN 使用索引进行过滤一个记录的 CPU 代价 |
| IndexReadPages    | array  | 估算的 IXSCAN 需要读取的索引页个数<br>NeedEvalIO 为 false 不需要计算<br>公式为：```max( 1, ceil( IndexPages * IXScanSelectivity ) )``` |
| IndexReadRecords  | array  | 估算的 IXSCAN 需要读取的索引记录个数<br>公式为：```max( 1, ceil( Records * IXScanSelectivity ) )``` |
| ReadPages         | array  | 估算的 IXSCAN 需要读取的数据页个数<br>NeedEvalIO 为 false 不需要计算<br>公式为：```max( 1, ceil( Pages * PredSelevtivity ) )``` |
| ReadRecords       | array  | 估算的 IXSCAN 需要读取的记录个数<br>公式为：```max( 1, ceil( Records * IXPredSelectivity ) )``` |
| IOCost            | array  | 估算的 IXSCAN 的 IO 代价的公式及计算过程<br>NeedEvalIO 为 false 不需要计算<br>即各个数据页进行随机扫描的代价总和<br>公式为：```RandomReadIOCostUnit * ( IndexReadPages + ReadPages ) * ( PageSize / PageUnit )``` |
| CPUCost           | array  | 估算的 IXSCAN 的 CPU 代价的公式及计算过程<br>即各个记录从索引页和数据页中提取并进行匹配符过滤的代价总和<br>如果需要进行匹配符过滤，公式为：```IndexReadRecords * ( IXExtractCPUCost + PredCPUCost ) + ReadRecords * ( RecExtractCPUCost + MthCPUCost )```<br>如果不需要进行匹配符过滤，公式为：```IndexReadRecords * ( IXExtractCPUCost + PredCPUCost ) + ReadRecords * RecExtractCPUCost``` |
| StartCost         | array  | 估算的 IXSCAN 的启动代价（内部表示）<br>公式为：```IXScanStartCost + PredCPUCost * IndexLevels``` |
| RunCost           | array  | 估算的 IXSCAN 的运行代价（内部表示）<br>公式为：```IOCPURate * IOCost + CPUCost``` |
| TotalCost         | array  | 估算的 IXSCAN 的总代价（内部表示）<br>公式为：```StartCost + RunCost``` |
| OutputRecords     | array  | 估算的 IXSCAN 的输出记录个数<br>公式为：```max( 1, ceil( Records * min( IXPredSelectivity, MthSelectivity ) ) )``` |

示例
----

```lang-json
"ScanNode": {
  "IndexPages": 49,
  "IndexLevels": 1,
  "MthSelectivity": 0.00001,
  "MthCPUCost": 2,
  "IXScanSelectivity": 0.00001,
  "IXPredSelectivity": 0.00001,
  "PredCPUCost": 1,
  "IndexReadPages": [
    "max( 1, ceil( IndexPages * IXScanSelectivity ) )",
    "max( 1, ceil( 49 * 1e-05 ) )",
    1
  ],
  "IndexReadRecords": [
    "max( 1, ceil( Records * IXScanSelectivity ) )",
    "max( 1, ceil( 100000 * 1e-05 ) )",
    1
  ],
  "ReadPages": [
    "max( 1, ceil( Pages * IXPredSelectivity ) )",
    "max( 1, ceil( 49 * 1e-05 ) )",
    1
  ],
  "ReadRecords": [
    "max( 1, ceil( Records * IXPredSelectivity ) )",
    "max( 1, ceil( 100000 * 1e-05 ) )",
    1
  ],
  "IOCost": [
    "RandomReadIOCostUnit * ( IndexReadPages + ReadPages ) * ( PageSize / PageUnit )",
    "10 * ( 1 + 1 ) * ( 65536 / 4096 ) ",
    320
  ],
  "CPUCost": [
    "IndexReadRecords * ( IXExtractCPUCost + PredCPUCost ) + ReadRecords * RecExtractCPUCost",
    "1 * ( 2 + 1 ) + 1 * 4",
    7
  ],
  "StartCost": [
    "IXScanStartCost + PredCPUCost * IndexLevels",
    "0 + 1 * 1",
    1
  ],
  "RunCost": [
    "IOCPURate * IOCost + CPUCost",
    "2000 * 320 + 7",
    640007
  ],
  "TotalCost": [
    "StartCost + RunCost",
    "1 + 640007",
    640008
  ],
  "OutputRecords": [
    "max( 1, ceil( Records * min( IXPredSelectivity, MthSelectivity ) ) )",
    "max( 1, ceil( 100000 * min( 0.00001, 0.00001 ) ) )",
    1
  ]
}
```
