/******************************************************************************
 * @Description   : seqDB-24824:CL 层面 SIX 锁升级为 Z 锁
 * @Author        : liuli
 * @CreateTime    : 2021.12.14
 * @LastEditTime  : 2021.12.30
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_24824";

main( test );
function test ( testPara )
{
   var sessionAttr = { TransIsolation: 2, TransMaxLockNum: 10 };
   db.setSessionAttr( sessionAttr );

   var dbcl = testPara.testCL;
   var docs = [];
   var docs1 = [];
   var docs2 = [];
   for( var i = 0; i < 20; i++ )
   {
      docs.push( { a: i } );
      if( i < 10 )
      {
         docs1.push( { a: i } );
      } else
      {
         docs2.push( { a: i } );
      }
   }
   dbcl.insert( docs1 );

   db.transBegin();
   // 开启事务后写入10条新数据
   dbcl.insert( docs2 );

   // 读取原来的9条数据
   dbcl.find( { a: { "$lt": 9 } } ).toArray();
   checkCLLockType( db, LOCK_SIX );

   // 读操作之后执行truncate
   assert.tryThrow( SDB_DPS_INVALID_LOCK_UPGRADE_REQUEST, function()
   {
      dbcl.truncate();
   } );
   db.transCommit();

   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, docs );
}
