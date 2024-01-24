/******************************************************************************
 * @Description   : seqDB-24037:创建删除id索引
 * @Author        : Yi Pan
 * @CreateTime    : 2021.05.11
 * @LastEditTime  : 2022.02.25
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_24037";
testConf.clOpt = { AutoIndexId: false };

main( test );
function test ( testPara )
{
   var indexName = "$id";
   var cl = testPara.testCL;

   cl.createIdIndex();

   checkExistIndex( db, COMMCSNAME, COMMCLNAME + "_24037", indexName );

   cl.dropIdIndex();

   cl.createIdIndex();

   checkExistIndex( db, COMMCSNAME, COMMCLNAME + "_24037", indexName );
}

// 校验主备节点存在$id索引
function checkExistIndex ( db, csName, clName, indexName )
{
   var doTime = 0;
   var timeOut = 600000;
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
            dbcl.getIndex( indexName );
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