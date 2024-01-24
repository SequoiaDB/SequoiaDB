[^_^]:
    索引

索引是一种提高数据访问效率的特殊对象。在没有索引辅助的时候，如果要对少量记录进行精确查询，需要逐行地匹配扫描集合中所有的记录，这种方式的效率显然比较低。而有索引时，可以通过特定字段的值快速定位到匹配的记录，精确查询的效率将会大大提升。

使用
----

### 创建索引
    
如需创建索引，可以参考 [createIndex()][create_index] 接口。例如，在 sample.employee 集合上为 id 字段建立 idIdx 索引：
    
```lang-bash
db.sample.employee.createIndex( 'idIdx', { 'id': 1 } );
```
   
> **Note:**
>
> 一个集合可以拥有多个索引，一个索引也可以拥有多个字段。详细规格可以参考数据库[限制][limit]。

+ **正序与倒序索引**

    指定索引排序的顺序，在合适的场景下，会提升索引查找的效率。使用正序索引时，按索引字段正序排序的查找会更快，因为无需将匹配的记录再次进行排序；在匹配值小的记录时，也可能更快地命中。如果有需要索引字段倒序排序的查询，或者经常需要匹配值较大的记录时，则适合使用倒序的索引。
    
    在创建索引的接口中，索引定义指定字段 1 为正序，-1 为倒序。例如，在 sample.employee 集合上为 birthdate 字段建立倒序的索引：
    ```lang-bash
    db.sample.employee.createIndex( 'dateIdx', { 'birthdate': -1 } );
    ```
    
    字段的值在类型相同时，按类型比较规则对比大小。而字段的值在类型不同时，则按类型[优先级权值][data_type]比较大小。如 {'a': 1} < {'a': 2}，{'a': 1} < {'a': '1'}。

+ **唯一索引**

    如果需要确保索引字段的值是唯一的，可以使用唯一索引。使用唯一索引时，如果插入或更新会产生重复的值，则会报错。创建索引时指定 Unique 选项为 true，即可创建唯一索引。例如，为 id 字段创建唯一索引：
    ```lang-bash
    db.sample.employee.createIndex('idUniqueIdx', { 'id': 1 }, { 'Unique': true })
    ```
    默认地，唯一索引允许多个空值（null）同时存在。如果只允许空值唯一存在，可以附加指定 Enforced 选项为 true。例如：
    ```lang-bash
    db.sample.employee.createIndex( 'idUniqueIdx', { 'id': 1 }, { 'Unique': true, 'Enforced': true } )
    ```
>**Note：**
>
> 唯一索引包含多个字段时，只有每个字段均相同，才认为值是相同的。

+ **复合索引**

    复合索引（多字段索引）是包含了一个以上字段的索引。如果匹配条件经常使用某几个字段，可以为这些字段创建复合索引，使准确查询更加高效。例如，sample.employee 集合中有 lastName 和 firstName 字段，为两个字段建立唯一索引：
    ```lang-bash
    db.sample.employee.createIndex( 'nameIdx', { 'lastName': 1, 'firstName': 1 } )
    ```
    假设业务有以下的查询：
    ```lang-bash
    db.sample.employee.find( { 'lastName': 'Jafferson', 'firstName': 'John' } )
    ```
    在使用复合索引时，该查询会比任意一个单字段的索引速度更快。
    
    复合索引会根据索引定义中字段的顺序排序。根据例子，nameIdx 会先根据 lastName 排序，在 lastName 相同时，再按 firstName 对 lastName 相同的记录排序。因此当查询条件只覆盖复合索引定义的前几个字段时，也能使用该索引地查询。例如，有定义为 `{ x: 1, y: 1, z: 1 }` 的复合索引，那么以下的查询均可以使用复合索引：
    ```lang-bash
    db.sample.employee.find( { 'x': 10, 'y': 10, 'z': 100 } )
    db.sample.employee.find( { 'x': 10, 'y': 10 } )
    db.sample.employee.find( { 'x': 10 } )
    ```
    而类似 `{ 'y': 10 }`，`{ 'y': 10, 'z': 100 }` 的条件则无法使用索引。

+ **独立索引**

    独立索引是指在集合的部分数据节点上单独创建的索引。该索引在编目节点上没有元数据信息，且索引的相关操作不写入同步日志中，因此增量同步、全量同步和数据切分时将忽略该索引。例如，在节点 `sdbserver1:11820` 上创建独立索引：  

    ```lang-javascript
    db.sample.employee.createIndex( 'nameIdx', { 'name': 1 }, { Standalone: true }, { NodeName: "sdbserver1:11820" } )
    ```

    > **Note:**
    >
    > - 独立索引不支持配置约束，即参数 Unique/NotNull/NotArray/Global 不能为 true。
    > - 全文索引不能作为独立索引。

+ **其它索引选项**

    - NotNull：如果不允许索引字段不存在或者为 null，可以将这个选项设置为 true。
    - SortBufferSize：创建索引时使用的排序缓存的大小。在集合记录数据量较大时（大于 1000 万条记录）适当增大排序缓存大小可以提高创建索引的速度。

### 使用索引查询

一般地，SequoiaDB 会自动生成[访问计划][access_plan]决定查询是否使用索引扫描，以及使用哪个索引去扫描。如果需要指定索引来进行查询，可以使用 [SdbQuery.hint()][query_hint] 接口完成。例如，在 sample.employee 集合上指定 idIdx 索引来查询 id 为 999 的记录：
```lang-bash
db.sample.employee.find( { 'id': 999 } ).hint( { '': 'idIdx' } )
```

如需查看索引使用情况，可以使用 [explain()][query_explain]。ScanType 字段为 ixscan 说明使用了索引，否则 ScanType 为 tbscan。例如，在 IndexName 字段中可以查看所使用的是哪个索引：
```lang-bash
> db.sample.employee.find( { 'id': 999 } ).explain()
```
输出结果如下：

```lang-json
{
  "NodeName": "sdbserver:11740",
  "GroupName": "group1",
  "Role": "data",
  "Name": "sample.employee",
  "ScanType": "ixscan",
  "IndexName": "idIdx",
  "UseExtSort": false,
  "Query": {
    "$and": []
  },
  "IXBound": {
    "_id": [
      [
        {
          "$minElement": 1
        },
        {
          "$maxElement": 1
        }
      ]
    ]
  },
  "NeedMatch": false,
  "ReturnNum": 0,
  "ElapsedTime": 0.000052,
  "IndexRead": 0,
  "DataRead": 0,
  "UserCPU": 0,
  "SysCPU": 0
}
```

访问计划使用索引的决策决定于集合的[统计信息][statistics]。分析集合和索引的数据，有助于生成更高效的访问计划。用户可使用 [analyze()][analyze] 收集统计信息。

### 删除索引

如需删除无用的索引，可以参考 [dropIndex()][drop_index] 接口。例如，删除集合 sample.employee 中名为 idIdx 的索引：

```lang-bash
db.sample.employee.dropIndex('idIdx')
```

基本原理
----

创建索引时，数据库会将指定字段的值拷贝到一个数据结构索引项中，并对其进行排序。使用索引查询时，数据库会从索引中找到满足条件的索引项，然后根据索引项中记录的位置信息，找到完整的记录。从而实现高效的查询。索引项是以 B 树的形式组织的，因此使用树的遍历可以快速地找到满足条件的索引项。

图 1 中，对集合中的 id 字段建立索引，通过索引查询 id = 5 的记录。流程如红色线所示。

![avatar][picture1]

该查询分为以下几个步骤：

1. 找到 id 字段对应的索引
2. 在索引中，通过遍历 B 树的方式，找到符合条件的索引项
3. 通过索引项中记录的位置信息，找到完整的记录，并返回
4. 查询完成

[^_^]:
    本文使用的所有链接和引用
[create_index]:manual/Manual/Sequoiadb_Command/SdbCollection/createIndex.md
[limit]:manual/Manual/sequoiadb_limitation.md
[data_type]:manual/Distributed_Engine/Architecture/Data_Model/data_type.md
[access_plan]:manual/Distributed_Engine/Maintainance/Access_Plan/Readme.md
[query_hint]:manual/Manual/Sequoiadb_Command/SdbQuery/hint.md
[query_explain]:manual/Manual/Sequoiadb_Command/SdbQuery/explain.md
[statistics]:manual/Distributed_Engine/Maintainance/Access_Plan/statistics.md
[analyze]:manual/Manual/Sequoiadb_Command/Sdb/analyze.md
[drop_index]:manual/Manual/Sequoiadb_Command/SdbCollection/dropIndex.md
[picture1]:images/Distributed_Engine/Architecture/Data_Model/index_picture_1.png
