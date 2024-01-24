下述以名为“datasource”的数据源为例介绍数据源的搭建及使用步骤。该数据源与协调节点地址为 `sdbserver1:11810` 的集群建立交互通道，且 sdbserver1 中源集群的鉴权用户名为“sdbadmin”，用户密码为“sdbadmin”。

##创建数据源##

创建名为“datasource”的数据源

```lang-javascript
> db.createDataSource("datasource", "sdbserver1:11810", "sdbadmin", "sdbadmin")
```

>**Note:**
>
> 创建数据源的详细参数说明可参考 [createDataSource()][create]。

##查看数据源##

用户可通过 [listDataSources()][list] 或[数据源列表][SDB_LIST]查看数据源的元数据信息。

```lang-javascript
> db.listDataSources()
```

>**Note:**
>
> 元数据信息字段说明可参考 [SYSDATASOURCES 集合][SYSDATASOURCES]。

##使用数据源##

用户可以通过创建集合空间或集合时与数据源建立映射，实现跨集群的数据访问。在集合空间上使用数据源时，可以访问被映射集合空间下的所有集合，实现跨集群的多集合数据访问；在集合上使用数据源时，可以实现跨集群的单集合数据访问。

###创建映射###

- **创建集合空间**

   创建集合空间 sample 并关联数据源 datasource 的集合空间 sample1

   ```lang-javascript
   > db.createCS("sample", {DataSource: "datasource", Mapping: "sample1"})
   ```

   >**Note:**
   >
   > 集合空间使用数据源建立映射后，不支持在该集合空间下创建集合。

- **创建集合**

   创建集合 sample2.employee 并关联数据源 datasource 的同名集合

   ```lang-javascript
   > db.sample2.createCL("employee", {DataSource: "datasource"})
   ```

   创建集合 sample2.employee 并关联数据源 datasource 的集合 sample2.employee1

   ```lang-javascript
   > db.sample2.createCL("employee", {DataSource: "datasource", Mapping: "employee1"})
   ```

   创建集合 sample2.employee 并关联数据源 datasource 的集合 sample3.employee1

   ```lang-javascript
   > db.sample2.createCL("employee", {DataSource: "datasource", Mapping: "sample3.employee1"})
   ```

   >**Note:** 
   >
   > - 创建集合或集合空间时，如果映射同名集合或集合空间，则不需要指定 Mapping 参数。
   > - 主集合和分区集合不支持使用数据源。
   > - 使用了数据源的集合可作为子集合挂载到主集合，但使用了同一数据源的集合在相同主集合下只能挂载一个  


###查看映射###

- **查看集合空间信息**

   通过编目节点查看使用了数据源的集合空间信息

   ```lang-javascript
   > var cata = new Sdb("sdbserver", 11800)
   > cata.SYSCAT.SYSCOLLECTIONSPACES.find()
   ```

   输出结果如下：

   ```lang-json
   {
     "_id": {
       "$oid": "5ffc2f0072e60c4d9be30c4d"
     },
     "Name": "sample",
     "UniqueID": 1,
     "CLUniqueHWM": 4294967296,
     "PageSize": 65536,
     "LobPageSize": 262144,
     "Type": 0,
     "DataSourceID": 1,
     "Mapping": "sample1",
     "Collection": []
   }
   ```

- **查看集合信息**

   通过编目快照查看使用了数据源的集合信息

   ```lang-javascript
   > db.snapshot(SDB_SNAP_CATALOG)
   ```

   输出结果如下：

   ```lang-json
   {
     "_id": {
       "$oid": "5ffc313972e60c4d9be30c4f"
     },
     "Name": "sample2.employee",
     "UniqueID": 8589934593,
     "Version": 1,
     "Attribute": 1,
     "AttributeDesc": "Compressed",
     "CompressionType": 1,
     "CompressionTypeDesc": "lzw",
     "CataInfo": [
       {
         "GroupID": -2147483647,
         "GroupName": "DataSource"
       }
     ],
     "DataSourceID": 1,
     "Mapping": "sample2.employee"
   }
   ```

##读写数据源##

假设本地集群中集合 sample.employee 与源集群的集合 sample2.employee 建立映射，且集合 sample2.employee 存在如下记录：

```lang-text
{"name": "Sam", "age": 26}
{"name": "Tom", "age": 30}
{"name": "Mike", "age": 24}
```

1. 通过本地集群检查数据是否正确

    ```lang-json
    > db.sample.employee.find()
    ``` 

2. 通过集合 sample.employee 对源集群插入新数据

    ```lang-json
     > db.sample.employee.insert({"name": "Jack", "age": 32})
    ```

3. 在 sdbserver1 中检查源集群对应集合的数据是否更改

    ```lang-json
    > db.sample2.employee.find()
    ```

    输出结果如下，源集群中对应数据已更改：

    ```lang-json
    {
      "_id": {
        "$oid": "606529276a1169200b8c6c41"
      },
      "name": "Sam",
      "age": 26
    }
    {
      "_id": {
        "$oid": "606529326a1169200b8c6c42"
      },
      "name": "Tom",
      "age": 30
    }
    {
      "_id": {
        "$oid": "606529456a1169200b8c6c43"
      },
      "name": "Mike",
      "age": 24
    }
    {
      "_id": {
        "$oid": "60652ee96a1169200b8c6c44"
      },
      "name": "Jack",
      "age": 32
    }
    Return 4 row(s).
    ````
    
##参考##

更多操作可参考

| 操作                | 说明             |
| -------------       |------------      |
| [db.dropCS()][dropCS] | 删除映射集合空间<br>只会删除本地集群的元数据，不会删除作为数据源集群中的集合空间 | 
| [db.collectionspace.dropCL()][dropCL] | 删除映射集合<br>只会删除本地集群的元数据，不会删除作为数据源集群中的集合 |
| [datasource.alter()][alter] | 修改数据源的元数据信息 |
| [db.dropDataSource()][drop] | 删除指定数据源 |


[^_^]:
    本文使用的所有引用和链接
[create]:manual/Manual/Sequoiadb_Command/Sdb/createDataSource.md
[list]:manual/Manual/Sequoiadb_Command/Sdb/listDataSources.md
[SDB_LIST]:manual/Manual/List/SDB_LIST_DATASOURCES.md
[SYSDATASOURCES]:manual/Manual/Catalog_Table/SYSDATASOURCES.md
[alter]:manual/Manual/Sequoiadb_Command/SdbDataSource/alter.md
[get]:manual/Manual/Sequoiadb_Command/Sdb/getDataSource.md
[drop]:manual/Manual/Sequoiadb_Command/Sdb/dropDataSource.md
[dropCS]:manual/Manual/Sequoiadb_Command/Sdb/dropCS.md
[dropCL]:manual/Manual/Sequoiadb_Command/SdbCS/dropCL.md
