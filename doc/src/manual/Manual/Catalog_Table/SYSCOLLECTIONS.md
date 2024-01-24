
SYSCAT.SYSCOLLECTIONS 集合中包含了该集群中所有的用户集合信息，每个用户集合保存为一个文档。

每个文档包含以下字段：

|字段名|类型|描述|
|----|----|----|
|Name|string|集合的完整名，为<集合空间>.<集合名>形式 |
|Version|number|集合的版本号，由 1 起始，每次对该集合的元数据变更会造成版本号 +1 |
|IsMainCL|boolean|集合是否为表分区中的主表 |
|MainCLName|string|集合在表分区中的主表 |
|ReplSize|number|最小复制组，确保任何写操作必须被复制到至少指定数量的节点后返回成功 |
|ConsistencyStrategy|number| [同步一致性][ConsistencyStrategy]策略 |
|ShardingKey|object|分区键，为<字段名>:<数值>形式，仅在分区集合中存在<br>字段名为分区字段名，数值取值如下：<br> 1：正向排序 <br>-1：逆向排序 |
|ShardingType|string|分区类型，仅在分区集合中存在 <br> 分区类型有范围分区（Range）和散列分区（Hash）两种 |
|Partition|number|散列分区的分区大小值，必须为 2 的幂 |
|CataInfo|array|集合所在的逻辑节点信息 <br>1）在单分区集合中，该数组仅包含一个元素，代表该集合所在的复制组 <br>2）在多分区集合中，该数组中包含一个或多个元素，代表该集合中的每一个取值范围所在的复制组；每个取值范围包括 LowBound 与UpBound，代表其下限与上限，闭合关系为左闭右开 <br>3）在主表集合中，该数组中包含一个或多个元素，代表该集合中的每一个取值范围所在的子表；每个取值范围包括 LowBound 与 UpBound，代表其下限与上限，闭合关系为左闭右开 |
|AttributeDesc|string|集合的属性描述，取值如下：<br>"Compressed"：集合已开启压缩  <br> "NoIDIndex"：集合未创建 $id 索引  <br>"Capped"：（内部使用） <br> "StrictDataMode"：集合已开启严格数据模式，具体说明可参考 [createCL()][createCL] 的参数 StrictDataMode <br> 同一集合可具备多个属性，属性之间通过“\|”连接 |
|Attribute|number|集合的属性掩码，与参数 AttributeDesc 对应，相应的掩码如下：<br>1：对应"Compressed" <br>2：对应"NoIDIndex" <br>4：对应"Capped" <br>8：对应"StrictDataMode" <br> 当集合具备多个属性时，该参数值为各属性掩码值相加 |
|CompressionTypeDesc|string|压缩算法类型描述，取值可参考 [createCL()][createCL] 的参数 CompressionType|
|CompressionType|number|压缩算法类型掩码，与参数 CompressionTypeDesc 对应，相应的掩码如下：<br>0：对应"snappy" <br>1：对应"lzw"|
|EnsureShardingIndex|boolean|集合是否自动使用 ShardingKey 包含的字段创建名字为“$shard”的索引|
|Internalv|number|hash 算法版本号（仅数据库分区集合显示，内部使用）|
|LobShardingKeyFormat|string|主分区集合中大对象的切分键格式（仅使用大对象的集合显示）|
|DataSourceID|number|数据源 ID（仅使用数据源的集合显示）|
|Mapping|string|在数据源中所映射的集合名称（仅使用数据源的集合显示）|
| CreateTime | string | 创建集合的时间（仅在 v3.6.1 及以上版本生效） |
| UpdateTime | string | 更新集合元数据的时间（仅在 v3.6.1 及以上版本生效） |

**示例**

- 一个典型的单分区集合信息如下：

    ```lang-json
    {"Name": "sample.employee", "Version": 1, "CataInfo": [{"GroupID": 1000}]}
    ```

- 一个典型的多分区集合信息如下：

    ```lang-json
    {
      "Name": "sample.employee",
      "Version": 1,
      "ShardingKey": {"Field1": 1, "Field2": -1},
      "ShardingType": "range",
      "ReplSize": 3,
      "Attribute": 0,
      "AttributeDesc": "",
      "CataInfo":
        [
          {
          "GroupID": 1000,
          "LowBound": {"": MinKey, "": MaxKey},
          "UpBound": {"": MaxKey, "": MinKey}
          }
        ],
      "CreateTime": "2022-10-06-18.04.30.874000",
      "UpdateTime": "2022-10-06-18.04.30.874000"
    }
    ```


[^_^]:
     本文使用的所有引用及链接
[createCL]:manual/Manual/Sequoiadb_Command/SdbCS/createCL.md
[ConsistencyStrategy]:manual/Distributed_Engine/Architecture/Location/consistency_strategy.md