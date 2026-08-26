
本章节介绍当出现一些特殊情况时，如何处理。

## 情况一：往集合写数据时，报错 -315

-315 错误码是集合正在处于不可写入的状态，通常可能由以下原因引起：

1. 数据切换任务在进行数据切换时，集合会被设置为不可写状态。

解决方案：

1. 将涉及到该集合的迁移任务和数据切换任务停止

2. 连接源站点的 sdb shell，执行如下命令，查看集合是否完成了数据切换，以集合 cs.cl 为例：

    ```shell
    $ db.snapshot(8, {Name: "cs.cl"})
    ```
    > - 若结果显示该集合关联了数据源，说明数据切换已经完成，该集合可以正常写入数据。

3. 若结果显示该集合没有关联数据源，说明数据切换未完成，则需要手动将集合的不可写状态撤销，执行如下命令：

   ```shell
   $ db.cs.cl.alter({RepairCheck: false})
   ```

4. 连接元数据服务的 sdb shell，执行如下命令，清理迁移与数据切换相关状态记录，以集合 cs.cl 为例：

```shell
# source_site: 源站点名称, target_site: 目标站点名称, collection: 集合全名（包含集合空间名称）
# 若工具使用的集合空间不是 SDB_SCHEDULE_SYSTEM，则需要切换到对应的集合空间下执行该命令
$ db.SDB_SCHEDULE_SYSTEM.COLLECTION_TRANSFER_RECORD_STATUS.remove({"source_site": "rootsite", "target_site": "datasource-site", "collection": "cs.cl"})
$ db.SDB_SCHEDULE_SYSTEM.COLLECTION_DATA_SWITCH_EVENT.remove({"source_site": "rootsite", "target_site": "datasource-site", "collection": "cs.cl"})
```


## 情况二：集合在数据切换后，访问到的数据不一致

数据切换任务在数据切换时，会将集合的数据访问路径切换到数据源站点上，而原始集合会被重命名。如果发现数据不一致，可以通过以下操作，将数据访问路径切换回原始集合。

解决方案：

- 普通集合（非主子表）：

1. 将涉及到该集合的迁移任务和数据切换任务停止，修改任务配置，将该集合移除后，重新启动任务。

2. 删除集合

3. 将重命名后的集合名修改为原始集合名，并撤销集合的不可写状态

4. 将目标站点上的集合数据删除

5. 连接元数据服务的 sdb shell，执行如下命令，清理迁移与数据切换相关状态记录，以集合 cs.cl 为例：

   ```shell
   # source_site: 源站点名称, target_site: 目标站点名称, collection: 集合全名（包含集合空间名称）
   # 若工具使用的集合空间不是 SDB_SCHEDULE_SYSTEM，则需要切换到对应的集合空间下执行该命令
   $ db.SDB_SCHEDULE_SYSTEM.COLLECTION_TRANSFER_RECORD_STATUS.remove({"source_site": "rootsite", "target_site": "datasource-site", "collection": "cs.cl"})
   $ db.SDB_SCHEDULE_SYSTEM.COLLECTION_DATA_SWITCH_EVENT.remove({"source_site": "rootsite", "target_site": "datasource-site", "collection": "cs.cl"})
   ```

- 子表

1. 将涉及到该集合的迁移任务和数据切换任务停止，修改任务配置，将该集合移除后，重新启动任务。

2. 查看子表挂载的信息

3. 删除子表

4. 将重命名后的子表名修改为原始子表名，并撤销集合的不可写状态

5. 将原始子表重新挂载到主表上

6. 将目标站点上的子表数据删除

7. 连接元数据服务的 sdb shell，执行如下命令，清理迁移与数据切换相关状态记录，以集合 cs.sub_cl 为例

   ```shell
   # source_site: 源站点名称, target_site: 目标站点名称, collection: 集合全名（包含集合空间名称）
   # 若工具使用的集合空间不是 SDB_SCHEDULE_SYSTEM，则需要切换到对应的集合空间下执行该命令
   $ db.SDB_SCHEDULE_SYSTEM.COLLECTION_TRANSFER_RECORD_STATUS.remove({"source_site": "rootsite", "target_site": "datasource-site", "collection": "cs.sub_cl"})
   $ db.SDB_SCHEDULE_SYSTEM.COLLECTION_DATA_SWITCH_EVENT.remove({"source_site": "rootsite", "target_site": "datasource-site", "collection": "cs.sub_cl"})
   ```