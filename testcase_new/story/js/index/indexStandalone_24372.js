/******************************************************************************
 * @Description   : seqDB-24372:coord上执行get/list查看本地索引  
 * @Author        : liuli
 * @CreateTime    : 2021.09.27
 * @LastEditTime  : 2021.10.26
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_24372";
testConf.clOpt = { ReplSize: 0 };

main( test );
function test ( args )
{
   var dbcl = args.testCL;
   var indexName = "index_24372";

   // 插入数据后创建本地索引
   insertBulkData( dbcl, 1000 );
   var nodeName = getCLOneNodeName( db, COMMCSNAME + "." + testConf.clName );
   dbcl.createIndex( indexName, { a: 1 }, { Standalone: true }, { NodeName: nodeName } );

   // getIndex获取本地索引
   assert.tryThrow( SDB_IXM_NOTEXIST, function()
   {
      dbcl.getIndex( indexName );
   } );

   // listIndexes列取本地索引
   var cursor = dbcl.listIndexes();
   while( cursor.next() )
   {
      assert.notEqual( cursor.current().toObj().IndexDef.name, indexName );
   }
   cursor.close();
}