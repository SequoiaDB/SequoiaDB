
本文档主要介绍如何使用 [ C 客户端驱动接口][api]编写使用 SequoiaDB 数据库的程序。完整的示例代码请参考 SequoiaDB 安装目录下的 `samples/C`


##连接数据库##

* 连接 SequoiaDB 数据库，之后即可访问、操作数据库。如下为连接数据库的示例代码 `connect.c`。代码中需要包含 “client.h” 头文件

   ```lang-c
   #include <stdio.h>  
   #include "client.h"
   
   // Display Syntax Error
   void displaySyntax ( CHAR *pCommand )
   {
        printf( "Syntax: %s<hostname> <servicename> <username> <password>"
        OSS_NEWLINE, pCommand ) ;
   }
   
   INT32 main ( INT32 argc, CHAR **argv )
   {
        // define a connecion handle; use to connect to database
        INT32 rc = SDB_OK ;
        sdbConnectionHandle connection    = 0 ;

        // verify syntax
        if ( 5 != argc )
        {
           displaySyntax( (CHAR*)argv[0] ) ;
           exit ( 0 ) ;
        }

        // read argument
        CHAR *pHostName    = (CHAR*)argv[1] ;
        CHAR *pServiceName = (CHAR*)argv[2] ;
        CHAR *pUsr         = (CHAR*)argv[3] ;
        CHAR *pPasswd      = (CHAR*)argv[4] ;

        // connect to database
        rc = sdbConnect( pHostName, pServiceName, pUsr, pPasswd, &connection ) ;
        if( rc!=SDB_OK )
        {
             printf( "Fail to connet to database, rc = %d" OSS_NEWLINE, rc ) ;
             goto error ;
        }

   done:
        // disconnect from database
        if ( connection )
        {
             sdbDisconnect( connection ) ;
        }
        // release connection
        if ( connection )
        {
             sdbReleaseConnection( connection ) ;
        }
        return 0 ;
   error:
        goto done ;
   }
   ```

   在 Linux 下，可以进行如下编译并链接动态链接库文件 `libsdbc.so`：

   ```lang-bash
   $ gcc -o connect connect.c -I /< PATH >/sdbdriver/include -lsdbc -L /< PATH >/sdbdriver/lib
   $ ./connect localhost 11810 "" ""
   ```

   >**Note:**
   >本示例连接到本地数据库的 11810 端口，使用的是空的用户名和密码。用户可根据实际情况配置参数

##数据库操作##

* 创建集合空间和集合

   ```lang-c
   // 定义集合空间和集合句柄
   sdbCSHandle collectionspace       = 0 ;
   sdbCollectionHandle collection    = 0 ;

   // 创建集合空间 sample，配置集合空间内的集合的数据页大小为 4k
   sdbCreateCollectionSpace( connection, "sample", SDB_PAGESIZE_4K, &collectionspace ) ;
   // 在新建立的集合空间中创建集合 employee
   sdbCreateCollection( collectionspace, "employee", &collection ) ;
   ```

* 插入数据

   ```lang-c
   INT32 rc = SDB_OK ;
   bson obj ;
   bson result ;
   bson_init( &obj ) ;
   bson_init( &result ) ;

   // 准备需要插入的数据
   bson_append_string( &obj, "name", "tom" ) ;
   bson_append_int( &obj, "age", 24 ) ;
   rc = bson_finish( &obj ) ;
   if ( SDB_OK != rc )
   {
        printf( "Failed to create the inserting bson, rc = %d", rc ) ;
   }

   // 插入
   rc = sdbInsert2( collection, &obj, FLG_INSERT_RETURN_OID, &result ) ;
   if ( rc != SDB_OK )
   {
        printf( "Failed to insert, rc = %d", rc ) ;
   }
   else
   {
        printf( "Insert result: " ) ;
        bson_print( &result ) ;
   }

   bson_destroy( &obj );
   bson_destroy( &result );
   ```

* 查询

   ```lang-c
   // 定义游标句柄
   INT32 rc = SDB_OK ;
   sdbCursorHandle cursor = 0 ;
   bson obj ;
   bson_init( &obj );

   // 查询所有记录，查询结果放在游标句柄中
   rc = sdbQuery(collection, NULL, NULL, NULL, NULL, 0, -1, &cursor ) ;

   // 从游标中显示所有记录
   while( !( rc=sdbNext( cursor, &obj ) ) )
   {
        bson_print( &obj ) ;
        bson_destroy( &obj ) ;
        bson_init( &obj );
   }

   bson_destroy( &obj ) ;
   ```

* 索引

   在集合句柄 collection 指定的集合中创建一个以“name”为升序、“age”为降序的索引

   ```lang-c
   #define INDEX_NAME "index"
   INT32 rc = SDB_OK ;
   bson obj ;
   bson_init( &obj ) ;

   // 创建包含指定索引信息的 bson 对象
   bson_append_int( &obj, "name", 1 ) ;
   bson_append_int( &obj, "age", -1 ) ;
   rc = bson_finish( &obj ) ;
   if ( SDB_OK != rc )
   {
      printf( "Failed to create the bson for index definition, rc = %d", rc ) ;
   }

   // 创建以"name"为升序，"age"为降序的索引
   rc = sdbCreateIndex ( collection, &obj, INDEX_NAME, FALSE, FALSE ) ;
   if ( rc != SDB_OK )
   {
        printf( "Failed to create index, rc = %d", rc ) ;
   }

   bson_destroy ( &obj ) ;
   ```

* 更新

   ```lang-c
   INT32 rc = SDB_OK ;
   bson modifier ;
   bson result ;
   bson_init( &modifier ) ;
   bson_init( &result ) ;

   // 设置更新规则
   bson_append_start_object ( &modifier, "$set" ) ;
   bson_append_int ( &modifier, "age", 19 ) ;
   bson_append_finish_object ( &modifier ) ;
   rc = bson_finish ( &modifier ) ;
   if ( SDB_OK != rc )
   {
        printf( "Failed to create the modifier bson, rc = %d", rc ) ;
   }

   // 查看更新的内容
   printf ( "The update rule is: " ) ;
   bson_print( &modifier ) ;

   // 更新
   rc = sdbUpdate2( collection, &modifier, NULL, NULL, 0, &result ) ;
   if ( rc != SDB_OK )
   {
        printf( "Failed to update, rc = %d", rc ) ;
   }
   else
   {
        printf( "Update result: " ) ;
        bson_print( &result ) ;
   }

   bson_destroy( &modifier );
   bson_destroy( &result );
   ```

* 删除

   ```lang-c
   INT32 rc = SDB_OK ;
   bson result ;
   bson_init( &result ) ;

   // 删除所有记录
   rc = sdbDelete1( collection, NULL, NULL, 0, &result ) ;
   if ( rc != SDB_OK )
   {
        printf( "Failed to delete , rc = %d", rc ) ;
   }
   else
   {
        printf( "Delete result: " ) ;
        bson_print( &result ) ;
   }

   bson_destroy( &result );
   ```

##集群操作##

集群操作主要涉及复制组与节点。如下以创建复制组、获取节点为例

* 创建复制组

   ```lang-c
   // 定义复制组句柄
   sdbReplicaGroupHandle rg = 0 ;
   ...
   // 建立编目复制组
   sdbCreateCataReplicaGroup( connection, HOST_NAME, SERVICE_NAME, CATALOG_SET_PATH , NULL ) ;
   // 创建数据复制组
   sdbCreateReplicaGroup( connection, GROUP_NAME, &rg ) ;
   // 创建数据节点
   sdbCreateNode( rg, HOST_NAME1, SERVICE_NAME1, DATABASE_PATH1, NULL ) ;
   // 启动复制组
   sdbStartReplicaGroup( rg ) ;
   ```

* 获取数据节点

   ```lang-c
   // 定义一个数据节点句柄
   sdbNodeHandle masternode   = 0 ;
   sdbNodeHandle slavenode    = 0 ;
   ...
   // 获取主数据节点
   sdbGetNodeMaster( rg, &masternode ) ;
   //获取从数据节点
   sdbGetNodeSlave( rg, &slavenode ) ;
   ```

[^_^]:
     本文使用的所有引用和链接
[api]:manual/Database_Instance/Json_Instance/Development/c_driver/c_api.md