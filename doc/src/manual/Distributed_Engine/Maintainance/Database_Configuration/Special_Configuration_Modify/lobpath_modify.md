大对象数据文件存储在 lobpath 指定的路径下。lobpath 的值默认与 dbpath 相同。下面以节点"sdbserver1:11820"（其大对象数据文件存储原路径为 `/opt/sequoiadb/database/data/11820`）为例，介绍修改 lobpath 的详细步骤：

1. 停止节点 11820

    ```lang-bash
    $ sdbstop -p 11820
    ```

2. 创建新的大对象数据文件存储目录，并修改目录权限为数据库管理用户（安装 SequoiaDB 时指定，默认为 sdbadmin）

    ```lang-bash
    $ mkdir /data/disk1/sequoiadb/lob/11820
    $ chown -R sdbadmin:sdbadmin_group /data/disk1/sequoiadb/lob/11820
    $ chmod 755 /data/disk1/sequoiadb/lob/11820
    ```

3. 切换至原路径

    ```lang-bash
    $ cd /opt/sequoiadb/database/data/11820
    ```

4. 将大对象数据文件和大对象元数据文件转移至目标路径

    ```lang-bash
    $ mv *.lobd *.lobm /data/disk1/sequoiadb/lob/11820
    ```

    >**Note:**
    >
    > 大对象元数据文件（.lobm）默认存储在 lobpath 指定的路径。如果设置了 [lobmetapath][lobm]，则需要将文件转移至对应路径。

5. 修改节点 11820 的配置文件

    ```lang-bash
    $ vim /opt/sequoiadb/conf/local/11820/sdb.conf
    ```

    将参数 lobpath 修改为 `/data/disk1/sequoiadb/lob/11820`

    ```lang-ini
    ...
    lobpath=/data/disk1/sequoiadb/lob/11820
    ...
    ```

6. 重启节点 11820

    ```lang-bash
    $ sdbstart -p 11820
    ```

7. 通过快照查看节点 11820 的配置信息

    ```lang-javascript
    > db.snapshot(SDB_SNAP_CONFIGS, {"svcname": "11820"}, {"lobpath": ""})
    ```


[^_^]:
     本文使用的所有引用及链接
[lobm]:manual/Distributed_Engine/Maintainance/Database_Configuration/Special_Configuration_Modify/lobmetapath_modify.md