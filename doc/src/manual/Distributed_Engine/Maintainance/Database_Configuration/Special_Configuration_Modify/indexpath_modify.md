索引文件存储在 indexpath 指定的路径下。indexpath 的值默认与 dbpath 相同。如果需要修改 indexpath 的值，建议修改为 ssd 盘下的目录路径。下面以节点"sdbserver1:11820"（其索引文件存储原路径为 `/opt/sequoiadb/database/data/11820`）为例，介绍修改 indexpath 的详细步骤：

1. 停止节点 11820

    ```lang-bash
    $ sdbstop -p 11820
    ```

2. 创建新的索引文件存储目录，并修改目录权限为数据库管理用户（安装 SequoiaDB 时指定，默认为 sdbadmin）

    ```lang-bash
    $ mkdir /data/ssd1/sequoiadb/index/11820
    $ chown -R sdbadmin:sdbadmin_group /data/ssd1/sequoiadb/index/11820
    $ chmod 755 /data/ssd1/sequoiadb/index/11820
    ```

3. 切换至原路径

    ```lang-bash
    $ cd /opt/sequoiadb/database/data/11820
    ```

4. 将索引文件转移至目标路径

    ```lang-bash
    $ mv *.idx /data/ssd1/sequoiadb/index/11820
    ```

5. 修改节点 11820 的配置文件

    ```lang-bash
    vim /opt/sequoiadb/conf/local/11820/sdb.conf
    ```

    将参数 indexpath 修改为 `/data/ssd1/sequoiadb/index/11820`

    ```lang-ini
    ...
    indexpath=/data/ssd1/sequoiadb/index/11820
    ...
    ```

6. 重启节点 11820

    ```lang-bash
    $ sdbstart -p 11820
    ```

7. 通过快照查看节点 11820 的配置信息

    ```lang-javascript
    > db.snapshot(SDB_SNAP_CONFIGS, {"svcname": "11820"}, {"indexpath": ""})
    ```