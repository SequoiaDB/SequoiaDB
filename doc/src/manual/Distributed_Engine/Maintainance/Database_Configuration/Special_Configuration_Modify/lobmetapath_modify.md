大对象元数据文件存储在 lobmetapath 指定的路径下。lobmetapath 的值默认与 lobpath 相同。下面以节点"sdbserver1:11820"（其大对象元数据文件存储原路径为 `/opt/sequoiadb/database/data/11820`）为例，介绍修改 lobmetapath 的详细步骤：

1. 停止节点 11820

    ```lang-bash
    $ sdbstop -p 11820
    ```

2. 创建新的大对象元数据文件存储目录，并修改目录权限为数据库管理用户（安装 SequoiaDB 时指定，默认为 sdbadmin）

    ```lang-bash
    $ mkdir /data/disk1/sequoiadb/lobmet/11820
    $ chown -R sdbadmin:sdbadmin_group /data/disk1/sequoiadb/lobmet/11820
    $ chmod 755 /data/disk1/sequoiadb/lobmet/11820
    ```

3. 切换至原路径

    ```lang-bash
    $ cd /opt/sequoiadb/database/data/11820
    ```

4. 将大对象元数据文件转移至目标路径

    ```lang-bash
    $ mv *.lobm /data/disk1/sequoiadb/lobmet/11820
    ```

5. 修改节点 11820 的配置文件

    ```lang-bash
    $ vim /opt/sequoiadb/conf/local/11820/sdb.conf
    ```

    将参数 lobmetapath 修改为 `/data/disk1/sequoiadb/lobmet/11820`

    ```lang-ini
    ...
    lobmetapath=/data/disk1/sequoiadb/lobmet/11820
    ...
    ```

6. 重启节点 11820

    ```lang-bash
    $ sdbstart -p 11820
    ```

7. 通过快照查看节点 11820 的配置信息

    ```lang-javascript
    > db.snapshot(SDB_SNAP_CONFIGS, {"svcname": "11820"}, {"lobmetapath": ""})
    ```