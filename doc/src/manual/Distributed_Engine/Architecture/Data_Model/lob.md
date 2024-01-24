[^_^]:
    大对象

大对象 LOB (Large Object) 的功能是为了突破 SequoiaDB 巨杉数据库的单条记录最大长度为 16MB 的限制，为用户写入或读取更大型记录提供便利。对于文档、图片、音频和视频等非结构化的数据，用户可以使用 LOB 存储。LOB 存放在集合中，每一个 LOB 都需要一个 OID 来唯一标识。LOB 的内容只存放在一个集合中，当集合被删除时，其拥有的 LOB 将自动删除。

存放 LOB 的集合应该满足如下要求：

- 当集合是普通集合，集合只存在某一个数据组中。此时，LOB 的最大容量为集合能使用的最大文件空间。
- 当集合是[散列分区（hash）集合][data_mode_hash_split]，LOB 对该散列分区集合的 ShardingKey 没有要求。一般情况下，用户在创建散列分区集合来存放 LOB 时，可以使用 _id 键作为 ShardingKey。当集合为散列分区集合时，集合存在一个或者多个数据组中。在这种情况下，LOB 最大的容量由散列分区集合使用的数据组的数据决定。

##存储结构##

LOB 以集合为单位进行存储，保存[集合空间][data_mode_collection_space]和集合的逻辑结构。在磁盘的数据存储中，对应的集合空间会增加两个文件：

```lang-text
[CollectionSpace].1.lobm
[CollectionSpace].1.lobd
```

LOBM 文件和 LOBD 文件一一对应，LOBM 为元数据文件，用于 LOB 数据页的分配、查找和管理；LOBD 为 LOB 的数据存储文件，存储真实的数据。

LOBD 在存储数据时，以数据页为最小单位。数据页大小最小为 4KB，最大为 512KB，默认为 256KB。在 LOBD 上存放数据时，若 LOB 总大小小于 1 个数据页的大小，该 LOB 也会独占整个数据页，哪怕该 LOB 大小只有 1Byte。所以，当存储的 LOB 总大小较小时，用户应该选择适当的数据页大小来存储 LOB 的内容，以减少空间浪费。关于 LOB 的数据页大小的选择，可参考 [createCS()][data_mode_createCS] 中参数 LobPageSize 的介绍。

LOBM 和 LOBD 的存储结构如图 1 所示。

![LOB 存储文件结构图][data_mode_lob_storage_struct]

##功能介绍##

目前，LOB 支持以下功能：

- 顺序读写和随机读写
- 打开读操作和打开写操作
- 并发读和并发写

在对 LOB 进行操作时，注意以下情况：

||可读|可写|可删除|可并发读|可并发写|备注|
|----|----|----|----|----|----|----|
|创建 LOB|×|√|×|×|×||
|打开读 LOB|√|×|×|√|×||
|打开写 LOB|×|√|×|√|√|并发写时需要按写入的数据段加锁并 seek 到加锁的数据段后写入数据。 <br> 并发锁定的数据段不能重叠。 <br> 当某数据段被锁定后，其上面的数据将可被覆盖写入。<br> 关于 LOB 的 seek, lock 和 lockAndSeek 操作，可查看各驱动 LOB 相关 API 的说明。|
|删除 LOB|×|×|√|×|×||

##操作说明##

下表以在 SDB Shell 上操作 LOB 来介绍 LOB 相关 API 的使用。

| LOB 操作 | 参见 | 说明 | 相关 API |
| ---- | ---- | ---- | ---- |
| 创建 | [putLob()][data_mode_putLob] | 向集合创建一个 LOB | SdbCollection::openLob() // 以创建的方式打开 <br> SdbLob::write() <br> SdbLob::close() |
| 读取 | [getLob()][data_mode_getLob] | 从集合读取某个 LOB 记录| SdbCollection::openLob() // 以只读的方式打开 <br> SdbLob::read() <br> SdbLob::close() |
| 删除 | [deleteLob()][data_mode_deleteLob] | 删除集合某个 LOB 对象 | SdbCollection::removeLob() |
| 列表 | [listLobs()][data_mode_listLobs] | 列出集合所有 LOB 对象 | SdbCollection::listLobs() |

> **Note:**
>
> - SDB Shell 使用 C++ 驱动连接数据库，上表相关 API 是 C++ LOB API 的情况。其他驱动的 LOB API 拥有类似的接口，详情可参考相关的驱动。
> - 关于 LOB 的更多 API 说明，可参考各驱动 LOB API 的说明。

##示例##

1. 将本地视频文件 `video_2019_02_26_1.avi` 上传至集合 sample.employee 中

    ```lang-bash
    > db.sample.employee.putLob('/opt/video_2019_02_26_1.avi')
    5435e7b69487faa663000897
    ```

2. 查看集合 sample.employee 中所有 LOB 及其对应的 OID

    ```lang-bash
    > db.sample.employee.listLobs()
    {
      "Size": 76602,
      "Oid": {
        "$oid": "5435e7b69487faa663000897"
      },
      "CreateTime": {
        "$timestamp": "2019-02-26-12.51.43.628000"
      },
      "ModificationTime": {
        "$timestamp": "2019-02-26-12.51.45.523000"
      },
      "Available": true
    }
    ```

3. 将集合 sample.employee 中 OID 为"5435e7b69487faa663000897"的 LOB 下载到本地文件 `video_2019_02_26_1_bak.avi` 中

    ```lang-bash
    > db.sample.employee.getLob('5435e7b69487faa663000897', '/opt/video_2019_02_26_1_bak.avi')
    ```

4. 将集合 sample.employee 中的 OID 为"5435e7b69487faa663000897"的 LOB 记录删除

    ```lang-bash
    > db.sample.employee.deleteLob('5435e7b69487faa663000897')
    ```

[^_^]:
     本文使用的所有引用和链接
[data_mode_lob_storage_struct]:images/Distributed_Engine/Architecture/Data_Model/log_storage_struct.png
[data_mode_hash_split]:manual/Distributed_Engine/Architecture/Sharding/architecture.md
[data_mode_collection_space]:manual/Distributed_Engine/Architecture/Data_Model/collection_space.md
[data_mode_createCS]:manual/Manual/Sequoiadb_Command/Sdb/createCS.md
[data_mode_putLob]:manual/Manual/Sequoiadb_Command/SdbCollection/putLob.md
[data_mode_getLob]:manual/Manual/Sequoiadb_Command/SdbCollection/getLob.md
[data_mode_deleteLob]:manual/Manual/Sequoiadb_Command/SdbCollection/deleteLob.md
[data_mode_listLobs]:manual/Manual/Sequoiadb_Command/SdbCollection/listLobs.md
