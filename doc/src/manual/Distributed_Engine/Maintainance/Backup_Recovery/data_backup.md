SequoiaDB 巨杉数据库备份功能支持全量备份和增量备份。全量备份过程中会阻塞数据库变更操作，即数据插入、更新、删除等变更操作会被阻塞，直到全量备份完成才会执行；增量备份过程中不阻塞数据库变更操作。
 
- 全量备份：有选择地备份整个数据库的配置、数据和日志
- 增量备份：在上一个全量备份或增量备份的基础上备份新增的配置、数据和日志

备份文件以备份名命名，一次备份会生成 .bak 和 .number 两种文件。

- .bak 文件：用于保存此次备份的元数据信息
- .number 文件：用于保存此次备份的数据

同一节点中，增量备份和全量备份存在如下关系：

- 增量备份名称必须与全量备份名称相同。
- 全量备份生成的 .number 文件为 .1 文件，首次同名增量备份生成的 .number 文件为 .2 文件，后续同名增量备份生成的 .number 文件序号依次递增。

## 全量备份

用户可根据实际情况，对整个数据库集群或指定复制组进行全量备份。

### 对整个数据库集群执行全量备份

1. 启动 SDB Shell，并且连接到协调节点

    ```lang-javascript
    > var db = new Sdb("localhost",11810)
    ```

2. 执行全量备份

    ```lang-javascript
    > db.backup({Name:"backupAll",Description:"backup for all"})
    ```

    - Name：备份名称
    - Description：备份描述信息
 
    >**Note:**
    >
    > 详细参数说明可参考 [backup()][backup]。

### 对指定复制组执行全量备份

1. 启动 SDB Shell，并且连接到协调节点

    ```lang-javascript
    > var db = new Sdb("localhost",11810)
    ```

2. 执行全量备份
 
    ```lang-javascript
    > db.backup({Name:"backupName",Description:"backup group1",GroupName:"group1"})
    ```

    GroupName：指定需要备份的复制组名

## 增量备份

增量备份需要保证日志的连续性和一致性，如果日志不连续，或日志 Hash 校验不一致，则增量备份失败。因此，周期性的增量备份需要计算好日志和周期的关系，以防止日志覆写。

1. 启动 SDB Shell，并且连接到协调节点

    ```lang-javascript
    > var db = new Sdb("localhost",11810)
    ```

2. 执行增量备份

    ```lang-javascript
    > db.backup({Name:"backupAll",Description:"increase backup data",EnsureInc:true})
    ```

    EnsureInc：是否开启增量备份，默认为 false，不开启


## 查看备份信息

用户可使用[备份列表][list]或 [listBackup()][listbackup] 查看当前数据库的备份信息。




[^_^]:
    本文使用的所有引用及链接
[backup]:manual/Manual/Sequoiadb_Command/Sdb/backup.md
[list]:manual/Manual/List/SDB_LIST_BACKUPS.md
[listbackup]:manual/Manual/Sequoiadb_Command/Sdb/listBackup.md