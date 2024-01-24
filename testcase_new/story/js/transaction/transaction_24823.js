/******************************************************************************
 * @Description   : seqDB-24823:CL 层面 SIX 锁升级为 S(DDL) 锁
 * @Author        : liuli
 * @CreateTime    : 2021.12.20
 * @LastEditTime  : 2022.01.03
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_24823";

main( test );
function test ( testPara )
{
   var sessionAttr = { TransIsolation: 2, TransMaxLockNum: 10 };
   db.setSessionAttr( sessionAttr );

   var dbcl = testPara.testCL;
   var dbcs = testPara.testCS;
   var indexName = "index_24823";
   var clNewName = "cl_new_24823";
   var docs1 = [];
   var docs2 = [];
   var docs = [];
   for( var i = 0; i < 20; i++ )
   {
      if( i < 10 )
      {
         docs1.push( { a: i } );
      } else
      {
         docs2.push( { a: i } );
      }
      docs.push( { a: i } );
   }
   dbcl.insert( docs1 );
   dbcl.createIndex( indexName, { a: 1 } );

   db.transBegin();
   // 开启事务后写入10条新数据
   dbcl.insert( docs2 );

   // 读取原来的10条数据
   dbcl.find( { a: { "$lt": 10 } } ).hint( { "": indexName } ).toArray();
   checkCLLockType( db, LOCK_SIX );

   // 读操作之后执行renameCL
   assert.tryThrow( SDB_DPS_INVALID_LOCK_UPGRADE_REQUEST, function()
   {
      dbcs.renameCL( testConf.clName, clNewName );
   } );
   db.transCommit();

   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, docs );
}
