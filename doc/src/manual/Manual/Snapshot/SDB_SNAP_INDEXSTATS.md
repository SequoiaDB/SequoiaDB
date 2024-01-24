
索引统计信息快照可以列出当前数据库节点中所有的索引统计信息。

>**Note:**
>
> 编目节点没有索引统计信息。

##标识##

SDB_SNAP_INDEXSTATS

##数据节点字段信息##

| 字段名          | 数据类型 | 说明 |
| --------------- | -------- | ---- |
| NodeName        | string   | 集合所属节点名，格式为<主机名>:<端口号> |
| GroupName       | string   | 集合所属复制组名 |
| Collection      | string   | 统计的集合名称 |
| StatTimestamp   | string   | 统计收集的时间 |
| Index           | string   | 统计 Index 的名称 |
| TotalIndexLevels| int32    | 统计收集时索引的层数 |
| TotalIndexPages | int32    | 统计收集时索引的页个数 |
| Unique          | boolean  | Index 是否为唯一索引 |
| KeyPattern      | object   | 统计索引的字段定义，例如{a:1, b:-1} |
| DistinctValNum  | array    | 不重复值的个数<br>抽样时，指样本中不重复值的个数<br>数组第 1 个元素表示字段定义中第 1 个字段的不重复值个数；第 2 个元素表示字段定义中第 1 和第 2 个字段的不重复值个数。以此类推 <br>例如，字段定义为{a:1, b:-1}，数组为 [50, 100]，则 a 字段的不重复值有 50 个，a 和 b 字段组合的不重复值有 100 个 |
| MinValue        | object   | 索引最小值<br>抽样时，指样本中的最小值 |
| MaxValue        | object   | 索引最大值<br>抽样时，指样本中的最大值 |
| NullFrac        | int32    | null 值的万分比<br>抽样时，指样本中 null 值的万分比 |
| UndefFrac       | int32    | undefined 值的万分比<br>抽样时，指样本中 undefined 值的万分比 |
| SampleRecords   | int64    | 统计收集时抽样的文档个数 |
| TotalRecords    | int64    | 统计收集时的文档个数 |

##协调节点字段信息##

| 字段名                         | 数据类型 | 说明 |
| ------------------------------ | -------- | ---- |
| Collection                     | string   | 统计的集合名称 |
| Index                          | string   | 统计 Index 的名称 |
| Unique                         | boolean  | Index 是否为唯一索引 |
| KeyPattern                     | object   | 统计索引的字段定义，例如{a:1, b:-1} |
| StatInfo.GroupName             | string   | 集合所属复制组名 |
| StatInfo.Group.NodeName        | string   | 集合所属节点名，格式为<主机名>:<端口号> |
| StatInfo.Group.StatTimestamp   | int64    | 统计收集的时间戳 |
| StatInfo.Group.TotalIndexLevels| int32    | 统计收集时索引的层数 |
| StatInfo.Group.TotalIndexPages | int32    | 统计收集时索引的页个数 |
| StatInfo.Group.DistinctValNum  | array    | 不重复值的个数<br>抽样时，指样本中不重复值的个数 <br>数组第 1 个元素表示字段定义中第 1 个字段的不重复值个数；第 2 个元素表示字段定义中第 1 和第 2 个字段的不重复值个数，以此类推<br>例如，字段定义为{a:1, b:-1}，数组为 [50, 100]，则 a 字段的不重复值有 50 个，a 和 b 字段组合的不重复值有 100 个 |
| StatInfo.Group.MinValue        | object   | 索引最小值<br>抽样时，指样本中的最小值 |
| StatInfo.Group.MaxValue        | object   | 索引最大值<br>抽样时，指样本中的最大值 |
| StatInfo.Group.NullFrac        | int32    | null 值的万分比<br>抽样时，指样本中 null 值的万分比 |
| StatInfo.Group.UndefFrac       | int32    | undefined 值的万分比<br>抽样时，指样本中 undefined 值的万分比 |
| StatInfo.Group.SampleRecords   | int64    | 统计收集时抽样的文档个数 |
| StatInfo.Group.TotalRecords    | int64    | 统计收集时的文档个数 |

##示例##

通过协调节点查看快照

```lang-javascript
> db.snapshot( SDB_SNAP_INDEXSTATS )
```

输出结果如下：

```lang-json
{
  "Collection": "sample.employee",
  "Index": "index01",
  "Unique": true,
  "KeyPattern": {
    "activityType": 1,
    "status": 1
  },
  "StatInfo": [
    {
      "GroupName": "group1",
      "Group": [
        {
          "NodeName": "hostname:11820",
          "TotalIndexLevels": 2,
          "TotalIndexPages": 135,
          "DistinctValNum": [
            2,
            9
          ],
          "MinValue": {
            "activityType": 1,
            "status": 1
          },
          "MaxValue": {
            "activityType": 2,
            "status": 9
          },
          "NullFrac": 0,
          "UndefFrac": 0,
          "SampleRecords": 200,
          "TotalRecords": 135582,
          "StatTimestamp": "2020-06-19-14.10.39.149000"
        }
      ]
    },
    {
      "GroupName": "group2",
      "Group": [
        {
          "NodeName": "hostname:11840",
          "TotalIndexLevels": 2,
          "TotalIndexPages": 135,
          "DistinctValNum": [
            2,
            8
          ],
          "MinValue": {
            "activityType": 1,
            "status": 1
          },
          "MaxValue": {
            "activityType": 2,
            "status": 9
          },
          "NullFrac": 0,
          "UndefFrac": 0,
          "SampleRecords": 200,
          "TotalRecords": 136276,
          "StatTimestamp": "2020-06-19-14.10.38.931000"
        }
      ]
    }
  ]
}
...
```


[^_^]:
    本文使用的所有引用及链接
[statistics]:manual/Distributed_Engine/Maintainance/Access_Plan/statistics.md
