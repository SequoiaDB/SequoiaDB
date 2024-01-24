本文档主要介绍如何使用[ C++ 客户端驱动接口](api/cpp/html/index.html)编写使用 SequoiaDB 数据库的程序。完整的示例代码请参考 SequoiaDB 安装目录下的 `samples/CPP`

##连接数据库##

* 连接 SequoiaDB 数据库，之后即可访问、操作数据库。如下为连接数据库的示例代码 `connect.cpp`。代码中需要包含 “client.hpp” 头文件及使用命名空间 sdbclient

   ```lang-cpp
   #include <iostream>
   #include "client.hpp"
 
   using namespace std ;
   using namespace sdbclient ;
   using namespace bson ;
 
   // Display Syntax Error
   void displaySyntax ( CHAR *pCommand ) ;
 
   INT32 main ( INT32 argc, CHAR **argv )
   {
        // verify syntax
        if ( 5 != argc )
        {
           displaySyntax( (CHAR*)argv[0] ) ;
           exit ( 0 ) ;
        }
        // read argument
        CHAR *pHostName    = (CHAR*)argv[1] ;
        CHAR *pPort        = (CHAR*)argv[2] ;
        CHAR *pUsr         = (CHAR*)argv[3] ;
        CHAR *pPasswd      = (CHAR*)argv[4] ;
 
        // define local variable
        sdb connection ;
        INT32 rc = SDB_OK ;
 
        // connect to database
        rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
        if( rc!=SDB_OK )
        {
           cout << "Fail to connet to database, rc = " << rc << endl ;
           goto error ;
        }
        else
        cout << "Connect success!" << endl ;
 
       done:
          // disconnect from database
          connection.disconnect() ;
          return 0 ;
       error:
          goto done ;
     }
 
     // Display Syntax Error
     void displaySyntax ( CHAR *pCommand )
     {
       cout << "Syntax:" << pCommand << " <hostname>  <servicename>  <username>  <password> " << endl ;
     }
   ```

   在 Linux 下，可以使用如下命令编译及链接动态链接库文件 `libsdbcpp.so`：

  ```lang-bash
  $ g++ -o connect connect.cpp -I <PATH>/sdbdriver/include -lsdbcpp -L <PATH>/sdbdriver/lib -D_GLIBCXX_USE_CXX11_ABI=0
  $ ./connect localhost 11810 "" ""
  ```

  >**Note:**
  >
  > * 当用户使用的 GCC 编译器版本小于 GCC 5.1 时，使用 CPP 驱动动态库或者静态库不需要添加 -D_GLIBCXX_USE_CXX11_ABI=0 编译选项。
  >
  > * 本示例连接到本地数据库的 11810 端口，使用的是空的用户名和密码。用户可以实际情况配置参数。

##数据库操作##

* 创建集合空间和集合

   ```lang-cpp
   // 定义集合空间和集合对象
   sdbCollectionSpace collectionspace ;
   sdbCollection collection ;
   // 创建集合空间 sample，配置集合空间内的集合的数据页大小为 4k
   rc = connection.createCollectionSpace( "sample", SDB_PAGESIZE_4K, collectionspace ) ;
   // 在新建立的集合空间中创建集合 employee
   rc = collectionspace.createCollection( "employee", collection ) ;
   ```

* 插入数据

   ```lang-cpp
   BSONObj obj ;
   BSONObj result ;
   // 准备需要插入的数据
   obj = BSON( "name" << "tom" << "age" << 24 ) ;
   // 插入
   collection.insert( obj, FLG_INSERT_RETURN_OID, &result ) ;
   // 查看插入的结果
   cout<< "Insert result: " << result.toString() <<endl ;
   ```

* 查询

   ```lang-cpp
   // 定义一个游标对象
   sdbCursor cursor ;
   // 查询所有记录，并把查询结果放在游标对象中
   collection.query( cursor ) ;
   // 从游标中显示所有记录
   while( !( rc=cursor.next( obj ) ) )
   {
       cout << obj.toString() << endl ;
   }
   ```

* 索引

 在集合对象 collection 中创建一个以"name"为升序、"age"为降序的索引

   ```lang-cpp
   # define INDEX_NAME "index"
   // 创建包含指定索引信息的 BSONObj 对象
   BSONObj obj ;
   obj = BSON ( "name" << 1 << "age" << -1 ) ;
   // 创建一个以"name"为升序，"age"为降序的索引
   collection.createIndex ( obj, INDEX_NAME, FALSE, FALSE ) ;
   ```

* 更新

   ```lang-cpp
   BSONObj modifier ;
   BSONObj matcher ;
   BSONObj hint ;
   BSONObj result ;
   // 设置更新的内容
   modifier = BSON ( "$set" << BSON ( "age" << 19 ) ) ;
   cout << modifier.toString() << endl ;
   // 更新集合中的所有记录
   collection.update( modifier, matcher, hint, 0, &result ) ;
   // 查看更新的结果
   cout<< "Update result: " << result.toString() <<endl ;
   ```

* 删除

   ```lang-cpp
   BSONObj matcher ;
   BSONObj hint ;
   BSONObj result ;
   // 设置删除条件
   matcher = BSON( "age" << 19 ) ;
   // 更新集合中的所有记录
   collection.del( matcher, hint, 0, &result ) ;
   // 查看删除的结果
   cout<< "Delete result: " << result.toString() <<endl ;
   ```

##集群操作##

集群操作主要涉及复制组与节点。如下以创建复制组、获取节点为例

* 创建复制组

   ```lang-cpp
   // 定义一个复制组实例
   sdbReplicaGroup rg  ;
   // 定义创建节点需要使用的配置项，此处定义一个空的配置项，表示使用默认配置
   BSONObj conf ;
   // 创建数据复制组
   connection.createRG ( "dataGroup", rg ) ;
   // 创建第一个数据节点
   rg.createNode ( "sdbserver", "11820", "/opt/sequoiadb/database/data/11820", conf ) ;
   // 启动复制组
   rg.start () ;
  ```

* 获取节点

   ```lang-cpp
   // 定义数据节点实例
   sdbNode node ;
   // 获取主数据节点
   rg.getMaster( node ) ;
   ```
