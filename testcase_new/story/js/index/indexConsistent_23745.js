/******************************************************************************
 * @Description   : seqDB-23745:分区表创建/删除$id索引
 * @Author        : Yi Pan
 * @CreateTime    : 2021.03.29
 * @LastEditTime  : 2022.06.24
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_23745";
testConf.clOpt = { ShardingKey: { a: 1 }, AutoSplit: true, AutoIndexId: false };

main( test );
function test ( args )
{
   var idxName = "$id";
   var clName = COMMCLNAME + "_23745";
   var dbcl = args.testCL;

   // 创建id索引
   var docs = [];
   for( var i = 0; i < 100; i++ )
   {
      docs.push( { "_id": i, a: i, b: i } );
   }
   dbcl.insert( docs );
   dbcl.createIdIndex();

   // 检查索引
   checkExistIndex( db, COMMCSNAME, clName, idxName );

   // 校验数据并查看访问计划
   var cursor = dbcl.find().sort( { "_id": 1 } );
   commCompareResults( cursor, docs, false );
   checkExplain( dbcl, { "_id": 5 }, "ixscan", idxName );

   // 删除id索引，检查操作结果
   dbcl.dropIdIndex();
   commCheckIndexConsistent( db, COMMCSNAME, clName, idxName, false );

   // 查看访问计划
   checkExplain( dbcl, { "_id": 5 }, "tbscan" );
}

function checkExistIndex ( db, csName, clName, idxName )
{
   var doTime = 0;
   var timeOut = 300000;
   var nodes = commGetCLNodes( db, csName + "." + clName );
   do
   {
      var sucNodes = 0;
      for( var i = 0; i < nodes.length; i++ )
      {
         var seqdb = new Sdb( nodes[i].HostName + ":" + nodes[i].svcname );
         try
         {
            var dbcl = seqdb.getCS( csName ).getCL( clName );
         }
         catch( e )
         {
            if( e != SDB_DMS_NOTEXIST && e != SDB_DMS_CS_NOTEXIST )
            {
               throw new Error( e );
            }
            break;
         }
         try
         {
            dbcl.getIndex( idxName );
            sucNodes++;
         }
         catch( e )
         {
            if( e != SDB_IXM_NOTEXIST )
            {
               throw new Error( e );
            }
            break;
         }
         seqdb.close();
      }
      sleep( 200 );
      doTime += 200;
   } while( doTime < timeOut && sucNodes < nodes.length )
   if( doTime >= timeOut )
   {
      throw new Error( "check timeout index not synchronized !" );
   }
}