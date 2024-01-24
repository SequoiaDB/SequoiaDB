/******************************************************************************
 * @Description   : seqDB-23756:创建索引后，组内新增节点
 * @Author        : Yi Pan
 * @CreateTime    : 2021.03.29
 * @LastEditTime  : 2022.03.09
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_23756";
testConf.useSrcGroup = true;

main( test );
function test ( testPara )
{
   var indexName = "index_23756_";
   var dbcl = testPara.testCL;
   var groupName = testPara.srcGroupName;

   insertBulkData( dbcl, 100 );

   // 创建多个索引
   dbcl.createIndex( indexName + 1, { "a": 1 } );
   dbcl.createIndex( indexName + 2, { "c": 1 }, { "Unique": true } );
   dbcl.createIndex( indexName + 3, { "no": 1 }, { "Unique": true, "Enforced": true } );
   dbcl.createIndex( indexName + 4, { "b": 1 }, { "NotNull": true } );
   dbcl.createIndex( indexName + 5, { "d": 1 }, { "NotArray": true } );

   // cl所在组增加一个节点
   var nodes = commGetGroupNodes( db, groupName );
   println( "node  --  " + nodes[0].HostName );
   var indexNames = [indexName + "1", indexName + "2", indexName + "3", indexName + "4", indexName + "5"];
   try
   {
      createNode( groupName );
      // 校验主备LSN一致
      commCheckLSN( db, groupName, 300000 );
      checkIndexTask( "Create index", COMMCSNAME, COMMCLNAME + "_23756", indexNames, 0 );
      // 校验任务和索引一致性
      for( var i = 1; i < 6; i++ )
      {
         commCheckIndexConsistent( db, COMMCSNAME, COMMCLNAME + "_23756", indexName + i, true );
      }
   }
   finally
   {
      var rg = db.getRG( groupName );
      rg.removeNode( nodes[0].HostName, RSRVPORTBEGIN, { Enforced: true } );
   }
}

function createNode ( groupName )
{
   var nodes = commGetGroupNodes( db, groupName );
   var rg = db.getRG( groupName );
   rg.createNode( nodes[0].HostName, RSRVPORTBEGIN, RSRVNODEDIR + "/data/" + RSRVPORTBEGIN, { diaglevel: 5 } );
   rg.start();
}