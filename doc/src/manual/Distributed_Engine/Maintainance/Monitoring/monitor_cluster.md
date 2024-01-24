用户可以使用 [listReplicaGroups()][listReplicaGroups] 监控集群状态。

1. 连接到协调节点：

   ```lang-bash
   $ /opt/sequoiadb/bin/sdb
   > var db = new Sdb( "localhost", 11810 )
   ```

2. 集群状态：

   ```lang-bash
   > db.listReplicaGroups()
   ```

   输出结果如下：
    
   ```lang-json
   {
     "Group": [
       {
         "dbpath": "/opt/sequoiadb/database/cata/11800",
         "HostName": "hostname1",
         "Service": [
           ...
         ],
         "NodeID": 1
       },
       {
         "HostName": "hostname2",
         "dbpath": "/opt/sequoiadb/database/cata/11800",
         "Service": [
           ...
         ],
         "NodeID": 2
       },
       {
         "HostName": "hostname3",
         "dbpath": "/opt/sequoiadb/database/cata/11800",
         "Service": [
           ...
         ],
         "NodeID": 3
       }
     ],
     "GroupID": 1,
     "GroupName": "SYSCatalogGroup",
     "PrimaryNode": 1,
     "Role": 2,
     "Status": 1,
     "Version": 3,
     "_id": {
       "$oid": "558b9264de349a1b87451a1d"
     }
   }
   {
     "Group": [
       {
         "HostName": "hostname1",
         "dbpath": "/opt/sequoiadb/database/data/21100",
         "Service": [
           ...
         ],
         "NodeID": 1000
       },
       {
         "HostName": "hostname2",
         "dbpath": "/opt/sequoiadb/database/data/21100",
         "Service": [
           ...
         ],
         "NodeID": 1001
       },
       {
         "HostName": "hostname3",
         "dbpath": "/opt/sequoiadb/database/data/21100",
         "Service": [
           ...
         ],
         "NodeID": 1002
       }
     ],
     "GroupID": 1000,
     "GroupName": "group1",
     "PrimaryNode": 1001,
     "Role": 0,
     "Status": 1,
     "Version": 4,
     "_id": {
       "$oid": "558b9295de349a1b87451a21"
     }
   }
   ...
   ```

[^_^]:
      本文使用到的所有链接和引用

[listReplicaGroups]: manual/Manual/Sequoiadb_Command/Sdb/listReplicaGroups.md