/******************************************************************************
 * @Description   : seqDB-23754:list查看索引
 * @Author        : Yi Pan
 * @CreateTime    : 2021.03.29
 * @LastEditTime  : 2022.01.20
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_23754";

main( test );
function test ( testPara )
{
   var indexName = "index_23754_";
   var cl = testPara.testCL;
   insertBulkData( cl, 300 );

   // 创建多个索引
   cl.createIndex( indexName + 1, { a: 1 } );
   cl.createIndex( indexName + 2, { b: 1 } );
   cl.createIndex( indexName + 3, { c: 1 } );
   cl.createIndex( indexName + 4, { a: 1, b: 1 } );
   cl.createIndex( indexName + 5, { a: 1, c: 1 } );

   // 校验索引一致性
   for( var i = 1; i < 6; i++ )
   {
      commCheckIndexConsistent( db, COMMCSNAME, testConf.clName, indexName + i, true );
   }

   // coord查询和data查询索引信息
   var cursor = cl.listIndexes();
   var expResult = [];
   while( cursor.next() )
   {
      var result = cursor.current().toObj().IndexDef;
      if( result.name != "$id" )
      {
         expResult.push( result );
      }
   }
   cursor.close();

   var groups = commGetCLGroups( db, COMMCSNAME + "." + testConf.clName );
   for( var j = 0; j < groups.length; j++ )
   {
      var nodes = commGetGroupNodes( db, groups[j] );
      for( var i = 0; i < nodes.length; i++ )
      {
         var actResult = [];
         var data = new Sdb( nodes[i].HostName + ":" + nodes[i].svcname );
         var dataCL = data.getCS( COMMCSNAME ).getCL( testConf.clName );
         var cursor = dataCL.listIndexes();
         while( cursor.next() )
         {
            var result = cursor.current().toObj().IndexDef;
            if( result.name != "$id" )
            {
               delete result.CreateTime;
               delete result.RebuildTime;
               actResult.push( result );
            }
         }
         cursor.close();
         assert.equal( actResult.sort(), expResult.sort() );
         data.close();
      }
   }
}