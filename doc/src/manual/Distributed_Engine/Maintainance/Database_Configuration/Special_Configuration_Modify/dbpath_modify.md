数据文件存储在 dbpath 指定的路径下。如果需要修改 dbpath，则要将数据文件存储原路径下的文件转移至目标路径。下面以节点"sdbserver1:11820"（其数据文件存储原路径为 `/opt/sequoiadb/database/data/11820`）为例，介绍修改 dbpath 的详细步骤：

1. 停止节点 11820

    ```lang-bash
    $ sdbstop -p 11820
    ```

2. 创建新的数据文件存储目录，并修改目录权限为数据库管理用户（安装 SequoiaDB 时指定，默认为 sdbadmin）

    ```lang-bash
    $ mkdir /data/disk1/sequoiadb/data/11820
    $ chown -R sdbadmin:sdbadmin_group /data/disk1/sequoiadb/data/11820
    $ chmod 755 /data/disk1/sequoiadb/data/11820
    ```

3. 切换至原路径

    ```lang-bash
    $ cd /opt/sequoiadb/database/data/11820
    ```

4. 将文件转移至目标路径

    ```lang-bash
    $ mv * /data/disk1/sequoiadb/data/11820
    ```

    >**Note:**
    >
    > 索引文件（.idx）、大对象数据文件（.lobd）和大对象元数据文件（.lobm）默认存储在参数 dbpath 指定的路径，如果配置了 [indexpath][indexpath]、[lobpath][lobpath] 和 [lobmetapath][lobmetapath] 等参数，则需要将上述文件转移至对应目录。 

5. 修改节点 11820 的配置文件

    ```lang-bash
    $ vim /opt/sequoiadb/conf/local/11820/sdb.conf
    ```

    将参数 dbpath 配置为 `/data/disk1/sequoiadb/data/11820`

    ```lang-ini
    ...
    dbpath=/data/disk1/sequoiadb/data/11820
    ...
    ```

6. 重启节点 11820

    ```lang-bash
    $ sdbstart -p 11820
    ```

7. 通过快照查看节点 11820 的配置信息

    ```lang-javascript
    > db.snapshot(SDB_SNAP_CONFIGS, {"svcname": "11820"}, {"dbpath": ""})
    ```

[^_^]:
    本文使用的所有引用及连接
[indexpath]:manual/Distributed_Engine/Maintainance/Database_Configuration/Special_Configuration_Modify/indexpath_modify.md
[lobpath]:manual/Distributed_Engine/Maintainance/Database_Configuration/Special_Configuration_Modify/lobpath_modify.md
[lobmetapath]:manual/Distributed_Engine/Maintainance/Database_Configuration/Special_Configuration_Modify/lobmetapath_modify.md