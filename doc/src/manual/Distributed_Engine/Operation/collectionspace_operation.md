[^_^]:
     集合空间操作


下述以名为“sample”的集合空间为例，介绍集合空间的相关操作。

##创建集合空间##

创建名为“sample”的集合空间，并指定集合空间所属的[数据域][domain]为“mydomain”

```lang-javascript
> db = new Sdb("sdbserver", 11810)
> db.createCS("sample", {Domain: "mydomain"})
```

> **Note:**
>
> 创建集合空间的详细参数说明可参考 [createCS()][createCS]。

##查看集合空间##

用户可通过 [listCollectionSpaces()][listCollectionSpaces]、[集合空间列表][LIST_COLLECTIONSPACES]或[集合空间快照][SNAP_COLLECTIONSPACES]查看集合空间的相关信息。

- 通过 listCollectionSpace() 和集合空间列表查看集群中已创建的集合空间

    ```lang-javascript
    > db.listCollectionSpaces()
    ```

    输出结果如下：

    ```lang-json
    {
      "Name":"sample"
    }
    ```

- 通过集合空间快照查看集合空间的详细信息

    ```lang-javascript
    > db.snapshot(SDB_SNAP_COLLECTIONSPACES)
    ```

    输出结果如下：

    ```lang-json
    {
     "Name": "sample",
     "UniqueID": 61,
     "PageSize": 4096,  
     "LobPageSize": 262144,
     "TotalSize": 918945792,
     "FreeSize": 805183062,  
     "TotalDataSize": 155254784,
     "FreeDataSize": 133627904,
     "TotalIndexSize": 151060480,
     "FreeIndexSize": 134152171,
     "TotalLobSize": 352714752,
     "FreeLobSize": 140771328,
     "Collection": [
       {
         "Name": "employee",
         "UniqueID": 261993005057
       }
     ],
     "Group": [
       "group1"
     ]
    }
    ```

##使用集合空间##

用户创建集合空间后，可在集合空间下创建集合，并进行相关的[集合操作][collection]。

##修改集合空间属性##

修改集合空间 sample 的数据页大小为 8192

```lang-javascript
> db.sample.setAttributes({PageSize: 8192})
```

> **Note:**
>
> 修改集合空间属性的详细参数说明可参考 [setAttributes()][setAttributes]。

##删除集合空间##

删除名为“sample”的集合空间，并指定删除时检查集合空间是否为空

```lang-javascript
> db.dropCS("sample", {EnsureEmpty: true})
```

> **Note:**
>
> 删除集合空间的详细参数说明可参考 [dropCS()][dropCS]。

##参考##

更多集合空间操作可参考 [SdbCS][cs]。





[^_^]:
     本文使用的所有引用及链接
[domain]:manual/Distributed_Engine/Architecture/domain.md
[createCS]:manual/Manual/Sequoiadb_Command/Sdb/createCS.md
[listCollectionSpaces]:manual/Manual/Sequoiadb_Command/Sdb/listCollectionSpaces.md
[LIST_COLLECTIONSPACES]:manual/Manual/List/SDB_LIST_COLLECTIONSPACES.md
[SNAP_COLLECTIONSPACES]:manual/Manual/Snapshot/SDB_SNAP_COLLECTIONSPACES.md
[collection]:manual/Distributed_Engine/Operation/collection_operation.md
[setAttributes]:manual/Manual/Sequoiadb_Command/SdbCS/setAttributes.md
[dropCS]:manual/Manual/Sequoiadb_Command/Sdb/dropCS.md
[cs]:manual/Manual/Sequoiadb_Command/SdbCS/Readme.md