
* 用户可以使用 [snapshot()][snapshot] 监控每个节点的状态。

   1. 连接到协调节点

     ```lang-bash
     $ /opt/sequoiadb/bin/sdb
     > var db = new Sdb( "localhost", 11810 )
     ```

   2. 获取复制组

     ```lang-bash
     > datarg = db.getRG( "< datagroup1 >" )
     ```

   3. 获取数据节点

     ```lang-bash
     > datanode = datarg.getNode( "< hostname1 >", "< servicename1 >" )
     ```

   4. 获取该节点的快照

     ```lang-bash
     > datanode.connect().snapshot( SDB_SNAP_DATABASE )
     ```

* 用户也可以使用 Shell 脚本监控每个节点的状态。

   1. 编写 Shell 脚本 `monitor_insert.sh`

     ```lang-bash
     #!/bin/bash
     ~/sequoiadb/bin/sdb "db=new Sdb('hostname1',11810); \
                          db.getRG('sample').getNode('hostname2',11820).connect().snapshot(SDB_SNAP_DATABASE)" \
                          | grep TotalInsert
     ```


   2. 运行 Shell 脚本得到该节点的监控信息

     ```lang-bash
     $ ./monitor_insert.sh
     "TotalInsert": 0,
     ```


[^_^]:
       本文所用到的所有链接和引用

[snapshot]: manual/Manual/Sequoiadb_Command/Sdb/snapshot.md