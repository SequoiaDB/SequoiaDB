[同步日志][replicalog]文件的大小和数量可通过 logfilesz 和 logfilenum 进行设置。在多副本环境下，为避免数据丢失，修改前需停止写操作并保证复制组内节点 LSN 一致。下面以节点"sdbserver1:11820"为例，介绍修改 logfilesz 和 logfilenum 的详细步骤：

1. 通过快照查看节点信息

    ```lang-javascript
    > db.snapshot(SDB_SNAP_HEALTH, {}, {NodeName: null, IsPrimary: null, CompleteLSN: null})
    ```

    对比主备节点间字段 CompleteLSN 的值，如果保持一致则说明节点 LSN 一致
    
    ```lang-javascript
    ...
    {
      "NodeName": "u16-t09:11820",
      "IsPrimary": true,
      "CompleteLSN": 80148
    }
    {
      "NodeName": "u1604-cmm:11820",
      "IsPrimary": false,
      "CompleteLSN": 80148
    }
    ...
    ```

2. 停止节点 11820

    ```lang-bash
    $ sdbstop -p 11820
    ```

3. 删除全部日志文件

    ```lang-bash
    $ rm -rf /opt/sequoiadb/database/data/11820/replicalog
    ```

4. 修改节点 11820 的配置文件

    ```lang-bash
    $ vim /opt/sequoiadb/conf/local/11820/sdb.conf
    ```
  
    将参数 logfilesz 和 logfilenum 修改为 128 和 30

    ```lang-ini
    ...
    logfilesz=128
    logfilenum=30
    ...
    ```

5. 重启节点 11820

    ```lang-bash
    $ sdbstart -p 11820
    ```

6. 通过快照查看节点 11820 的配置信息

    ```lang-javascript
    > db.snapshot(SDB_SNAP_CONFIGS, {"svcname": "11820"}, {"logfilesz": "", "logfilenum": ""})
    ```


[^_^]:
    本文使用的所有引用及链接
[replicalog]:manual/Distributed_Engine/Architecture/Replication/architecture.md#同步日志
