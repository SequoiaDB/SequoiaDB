/******************************************************************************
 * @Description   : seqDB-24018:dropIndexAsync接口验证
 * @Author        : Yi Pan
 * @CreateTime    : 2021.05.12
 * @LastEditTime  : 2021.12.10
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_24018";

main( test );
function test ( testPara )
{
   var cl = testPara.testCL;
   var indexName = "index_24018";
   cl.createIndex( indexName, { "a": 1 } );

   // 指定名称存在
   var taskId = cl.dropIndexAsync( indexName );
   db.waitTasks( taskId );

   // 指定名称为空串
   assert.tryThrow( SDB_IXM_NOTEXIST, function()
   {
      cl.dropIndexAsync( "" );
   } );

   // 指定名称不存在
   assert.tryThrow( SDB_IXM_NOTEXIST, function()
   {
      cl.dropIndexAsync( "wrong" );
   } );
}