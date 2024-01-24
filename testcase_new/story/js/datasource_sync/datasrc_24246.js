/******************************************************************************
 * @Description   : seqDB-24246:源集群设置不继承会话访问属性，设置preferedPeriod属性
 * @Author        : Wu Yan
 * @CreateTime    : 2021.06.07
 * @LastEditTime  : 2021.06.07
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var dataSrcName = "datasrc24246";
   var clName = CHANGEDPREFIX + "_datasource24246";
   var srcCSName = "datasrcCS_24246";
   var csNameA = "DS_24246A";
   var csNameB = "DS_24246B";
   commDropCS( datasrcDB, srcCSName );
   clearDataSource( [csNameA, csNameB], dataSrcName );
   commCreateCS( datasrcDB, srcCSName );
   var groups = commGetGroups( datasrcDB )[0];;
   var groupName = groups[0].GroupName;
   var primaryPos = groups[0].PrimaryPos;
   commCreateCL( datasrcDB, srcCSName, clName, { ShardingKey: { a: 1 }, ReplSize: -1, Group: groupName } );

   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd, "SequoiaDB", { InheritSessionAttr: false } );
   //集合空间级映射
   var csA = db.createCS( csNameA, { DataSource: dataSrcName, Mapping: srcCSName } );
   var dbclA = csA.getCL( clName );
   //集合级映射
   var csB = db.createCS( csNameB );
   var dbclB = csB.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   var docs = [{ a: 1, b: 1, c: "testc" }, { a: 2, b: [1, "test"], c: "testsgasdgasdg" }, { a: 3, b: 234.3, c: { a: 1 } }, { a: 4, b: 4, c: "testtess4" }];

   var masterNode = datasrcDB.getRG( groupName ).getMaster();
   var expAccessNode = [masterNode.toString()];

   //preferedPeriod为0     
   db.setSessionAttr( { PreferedInstance: "s", PreferedPeriod: 0 } );
   dbclA.insert( docs );
   dbclB.insert( docs );
   findAndCheckAccessNodes( dbclA, expAccessNode );
   findAndCheckAccessNodes( dbclB, expAccessNode );

   //preferedPeriod为5  
   db.setSessionAttr( { PreferedPeriod: 5 } );
   dbclA.insert( docs ); 
   dbclB.insert( docs );
   findAndCheckAccessNodes( dbclA, expAccessNode );
   findAndCheckAccessNodes( dbclB, expAccessNode );
   //5s后检查会话访问节点为备节点,时间是一个cpu的tick，不精准，改成10s后查看
   sleep( 10000 );
   findAndCheckAccessNodes( dbclA, expAccessNode );
   findAndCheckAccessNodes( dbclB, expAccessNode );

   //preferedPeriod为-1
   db.setSessionAttr( { PreferedPeriod: -1 } )
   dbclA.insert( docs );
   dbclB.insert( docs );
   findAndCheckAccessNodes( dbclA, expAccessNode );
   findAndCheckAccessNodes( dbclB, expAccessNode );
   //5s后检查会话访问节点还是主节点
   sleep( 5000 );
   findAndCheckAccessNodes( dbclA, expAccessNode );
   findAndCheckAccessNodes( dbclB, expAccessNode );

   //preferedPeriod为60
   db.setSessionAttr( { PreferedPeriod: 60 } )
   dbclA.insert( docs );
   dbclB.insert( docs ); 
   findAndCheckAccessNodes( dbclA, expAccessNode );
   findAndCheckAccessNodes( dbclB, expAccessNode );
   //60s后检查会话访问节点为备节点
   sleep( 65000 );
   findAndCheckAccessNodes( dbclA, expAccessNode );
   findAndCheckAccessNodes( dbclB, expAccessNode );

   datasrcDB.dropCS( srcCSName );
   datasrcDB.close();
   clearDataSource( [csNameA, csNameB], dataSrcName );
}

