/******************************************************************************
 * @Description   : seqDB-24020:getTask接口验证
 * @Author        : liuli
 * @CreateTime    : 2021.08.05
 * @LastEditTime  : 2021.12.10
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_24308";

main( test );
function test ( args )
{
   var indexName = "Index_24308";

   var dbcl = args.testCL;

   var docs = [];
   for( var i = 0; i < 10000; i++ )
   {
      var cValue = parseInt( Math.random() * 5000 );
      docs.push( { a: i, b: i, c: cValue } );
   }
   dbcl.insert( docs );

   var taskId = dbcl.createIndexAsync( indexName, { a: 1 } );

   assert.tryThrow( SDB_OUT_OF_BOUND, function()
   {
      db.getTask();
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.getTask( "" );
   } );

   assert.tryThrow( SDB_CAT_TASK_NOTFOUND, function()
   {
      db.getTask( -1 );
   } );

   db.waitTasks( taskId );
   var actCur = db.getTask( taskId );
   var expCur = db.listTasks( { TaskID: taskId } );
   commCompareObject( expCur, actCur );
}