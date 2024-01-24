本文档主要介绍如何使用 Java 驱动接口编写使用 SequoiaDB 数据库的程序。完整的示例代码请参考 SequoiaDB 安装目录下的 `samples/Java`

##连接数据库##

连接 SequoiaDB 数据库，之后即可访问、操作数据库。

   ```lang-java
   // 连接 sdbserver 机器上的 SequoiaDB 数据库
   Sequoiadb sdb = new Sequoiadb( "sdbserver:11810", "admin", "admin" );
   try {
      // 访问、操作数据库
      // ...
   } finally {
      // 关闭连接
      sdb.close();
   }
   ```
 
   >**Note:**
   >
   > * SequoiaDB 类为非线程安全类，每个线程必须单独建立自己的 SequoiaDB 对象，不能传递给多个线程同时操作

##数据操作##

* 创建集合空间、集合

   ```lang-java
   CollectionSpace cs = sdb.createCollectionSpace( "sample" );
   DBCollection cl = cs.createCollection( "employee" );
   ```

* 插入

   ```lang-java
   // 准备待插入的数据
   BSONObject obj = new BasicBSONObject();
   obj.put( "name", "tom" );
   obj.put( "age", 24 );
   // 插入
   InsertResult result = cl.insertRecord( obj );
   // 查看插入的结果
   System.out.println( result );
   ```


* 更新

   ```lang-java
   // 设置匹配条件：age 等于 24
   BSONObject matcher = new BasicBSONObject( "age", 24 );
   // 设置修改的内容：将 age 改为 22
   BSONObject modifier = new BasicBSONObject();
   modifier.put("$set", new BasicBSONObject( "age", 22 ) );
   // 更新
   UpdateResult result = cl.updateRecords( matcher, modifier );
   // 查看更新的结果
   System.out.println(result);
   ```

* 查询

   ```lang-java
   // 设置查询条件：age 等于 22
   BSONObject matcher = new BasicBSONObject( "age", 22 );
   // 查询
   DBCursor cursor = cl.query( matcher, null, null, null );
   // 遍历游标，获取查询的记录
   try {
         while (cursor.hasNext()) {
            BSONObject record = cursor.getNext();
            String name = (String) record.get( "name" );
            System.out.println(" name = " +  name );
         } 
   } finally {
      cursor.close();   
   }
   ```

* 删除

   ```lang-java
   // 设置删除条件：age 等于 22
   BSONObject matcher = new BasicBSONObject( "age", 22 );
   // 删除
   DeleteResult result = cl.deleteRecords( matcher );
   System.out.println( result );
   ```

##集群操作##

* 创建复制组

   ```lang-java
   ReplicaGroup rg = sdb.createRG( "dataGroup" );
   rg.createNode( "sdbserver", 11820, "/opt/sequoiadb/database/data/11820", null );
   // 启动复制组
   rg.start();
   ```

* 在复制组增加节点

   ```lang-java
   ReplicaGroup rg = sdb.getReplicaGroup( "dataGroup" );
   Node node = rg.createNode("sdbserver", 11830, "/opt/sequoiadb/database/data/11830", null);
   // 启动节点
   node.start();
   ```