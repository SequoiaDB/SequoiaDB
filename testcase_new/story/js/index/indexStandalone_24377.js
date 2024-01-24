/******************************************************************************
 * @Description   : seqDB-24377:直连独立节点创建本地索引 
 * @Author        : liuli
 * @CreateTime    : 2021.09.27
 * @LastEditTime  : 2022.08.01
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_24377";

main( test );
function test ( args )
{
   if( !commIsStandalone( db ) )
   {
      return;
   }

   var dbcl = args.testCL;
   var indexName = "index_24377";
   insertBulkData( dbcl, 1000 );

   // 独立节点创建独立索引
   var nodeName = COORDHOSTNAME + ":" + COORDSVCNAME;
   dbcl.createIndex( indexName, { a: 1 }, { Standalone: true }, { NodeName: nodeName } );

   // 校验独立索引
   var count = 0;
   var cursor = dbcl.listIndexes();
   while( cursor.next() )
   {
      if( cursor.current().toObj().IndexDef.name == indexName )
      {
         assert.equal( cursor.current().toObj().IndexDef.Standalone, true );
         count++;
      }
   }
   cursor.next();
   assert.equal( count, 1 );

   dbcl.dropIndex( indexName );
   var cursor = dbcl.listIndexes();
   while( cursor.next() )
   {
      assert.notEqual( cursor.current().toObj().IndexDef.name, indexName );
   }
   cursor.next();
}

