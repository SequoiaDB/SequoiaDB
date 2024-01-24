[^_^]:
    回收站

SequoiaDB 巨杉数据库提供回收站机制，可以有效防止因误删集合空间或集合而导致的数据丢失，加强对数据的保护。

回收站机制默认为启用状态，并且仅对以下操作有效：

- [dropCS()][dropCS]：删除集合空间
- [dropCL()][dropCL]：删除集合
- [truncate()][truncate]：删除集合的所有数据

启用回收站机制后，SequoiaDB 将对已删除的集合空间或集合数据进行回收，并生成对应的回收站项目。用户可通过回收站项目，快速恢复集合空间或集合的数据。

##使用##

###生成回收站项目###

下述以集合 sample.employee 为例，介绍操作步骤。

1. 连接协调节点

    ```lang-javascript
    > db = new Sdb("localhost", 11810)
    ```

2. 确认回收站机制已启用

    ```lang-javascript
    > db.getRecycleBin().getDetail()
    ```

    输出结果如下，字段 Enable 显示为 true 表示回收站机制已启用：

    ```lang-json
    {
      "Enable": true,
      "ExpireTime": 4320,
      "MaxItemNum": 100,
      "MaxVersionNum": 2,
      "AutoDrop": false
    }
    ```

    > **Note:**
    >
    > - [getRecycleBin()][getRecycleBin] 用于获取回收站的引用。
    > - 如果字段 Enable 显示为 false，用户可通过 [enable()][enable] 启用回收站机制。
    > - 其他结果字段说明可参考 [getDetail()][getDetail]。

3. 删除集合 sample.employee

    ```lang-javascript
    > db.sample.dropCL("employee")
    ```

4. 查看是否已生成对应的回收站项目

    ```lang-javascript
    > db.getRecycleBin().list({OriginName: "sample.employee"})
    {
      "RecycleName": "SYSRECYCLE_5_12884901889",
      "RecycleID": 5,
      "OriginName": "sample.employee",
      "OriginID": 12884901889,
      "Type": "Collection",
      "OpType": "Drop",
      "RecycleTime": "2022-02-11-16.52.59.000000"
    }
    ```

    > **Note:**
    >
    > 用户可通过 [list()][list] 或[回收站项目列表][SDB_LIST_RECYCLEBIN]查看所有已生成的回收站项目，结果字段说明可参考[回收站项目列表][SDB_LIST_RECYCLEBIN]。

###从回收站中恢复数据###

下述以从回收站中恢复集合 sample.employee 为例，介绍操作步骤。

1. 连接协调节点

    ```lang-javascript
    > db = new Sdb("localhost", 11810)
    ```

2. 查看已生成的回收站项目

    ```lang-javascript
    > db.getRecycleBin().list()
    ```

    输出结果中显示集合空间 sample 和集合 sample.employee 分别在不同时间被删除

    ```lang-json
    {
      "OpType": "Drop",
      "OriginID": 12884901889,
      "OriginName": "sample.employee",
      "RecycleID": 5,
      "RecycleName": "SYSRECYCLE_5_12884901889",
      "RecycleTime": "2022-02-11-16.52.59.000000",
      "Type": "Collection"
    }
    {
      "RecycleName": "SYSRECYCLE_6_3",
      "RecycleID": 6,
      "OriginName": "sample",
      "OriginID": 3,
      "Type": "CollectionSpace",
      "OpType": "Drop",
      "RecycleTime": "2022-02-11-16.54.06.000000"
    }
    ```

3. 恢复集合空间 sample

    ```lang-javascript
    > db.getRecycleBin().returnItem("SYSRECYCLE_6_3")
    ```

    > **Note:**
    >
    > [returnItem()][returnItem] 用于恢复指定的回收站项目。在恢复集合时，如果集合对应的集合空间已被删除，需要先恢复该集合空间。

4. 查看集合空间是否已恢复

    ```lang-javascript
    > db.listCollectionSpaces()
    {
      "Name": "sample"
    }
    ```

5. 恢复集合 sample.employee

    ```lang-javascript
    > db.getRecycleBin().returnItem("SYSRECYCLE_5_12884901889")
    ```

6. 查看集合是否已恢复

    ```lang-javascript
    > db.listCollections()
    {
      "Name": "sample.employee"
    }
    ```

##配置##

用户可通过 [getDetail()][getDetail] 查看当前回收站的配置。如果配置不符合预期，可以使用 [alter()][alter] 或 [setAttributes()][setAttributes] 进行修改。

##参考##

更多操作可参考

| 操作 | 说明 |
| ---- | ---- |
| [SdbRecycleBin.disable()][disable] | 禁用回收站机制 |
| [SdbRecycleBin.returnItemToName()][returnItemToName] | 以特定的名称恢复指定的回收站项目 |
| [SdbRecycleBin.snapshot()][snapshot] | 查看回收站项目的快照 |
| [SdbRecycleBin.count()][count] | 查看回收站项目的个数 |
| [SdbRecycleBin.dropItem()][dropItem] | 删除指定的回收站项目 |
| [SdbRecycleBin.dropAll()][dropAll] | 删除所有的回收站项目 |
| [Sdb.dropCS()][dropCS]/[SdbCS.dropCL()][dropCL]/[SdbCollection.truncate()][truncate] | 删除集合空间/删除集合/删除集合的所有数据<br>函数中增加参数 SkipRecycleBin，当一些临时表的操作不希望被回收时，可以设置 SkipRecycleBin 为 true








[^_^]:
     本文使用的所有链接及引用
[dropCS]:manual/Manual/Sequoiadb_Command/Sdb/dropCS.md
[dropCL]:manual/Manual/Sequoiadb_Command/SdbCS/dropCL.md
[truncate]:manual/Manual/Sequoiadb_Command/SdbCollection/truncate.md
[getDetail]:manual/Manual/Sequoiadb_Command/SdbRecycleBin/getDetail.md
[getRecycleBin]:manual/Manual/Sequoiadb_Command/Sdb/getRecycleBin.md
[enable]:manual/Manual/Sequoiadb_Command/SdbRecycleBin/enable.md
[disable]:manual/Manual/Sequoiadb_Command/SdbRecycleBin/disable.md
[count]:manual/Manual/Sequoiadb_Command/SdbRecycleBin/count.md
[list]:manual/Manual/Sequoiadb_Command/SdbRecycleBin/list.md
[snapshot]:manual/Manual/Sequoiadb_Command/SdbRecycleBin/snapshot.md
[SDB_LIST_RECYCLEBIN]:manual/Manual/List/SDB_LIST_RECYCLEBIN.md
[SDB_SNAP_RECYCLEBIN]:manual/Manual/Snapshot/SDB_SNAP_RECYCLEBIN.md
[returnItem]:manual/Manual/Sequoiadb_Command/SdbRecycleBin/returnItem.md
[returnItemToName]:manual/Manual/Sequoiadb_Command/SdbRecycleBin/returnItemToName.md
[alter]:manual/Manual/Sequoiadb_Command/SdbRecycleBin/alter.md
[setAttributes]:manual/Manual/Sequoiadb_Command/SdbRecycleBin/setAttributes.md
[dropItem]:manual/Manual/Sequoiadb_Command/SdbRecycleBin/dropItem.md
[dropAll]:manual/Manual/Sequoiadb_Command/SdbRecycleBin/dropAll.md
