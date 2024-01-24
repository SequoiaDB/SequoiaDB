/******************************************************************************
 * @Description   : seqDB-24810:CL 层面 IS 锁升级为 S 锁
 * @Author        : liuli
 * @CreateTime    : 2021.12.14
 * @LastEditTime  : 2021.12.30
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_24810";

main( test );
function test ( testPara )
{
   var sessionAttr = { TransIsolation: 2, TransMaxLockNum: 10 };
   db.setSessionAttr( sessionAttr );

   var dbcl = testPara.testCL;
   var indexName = "index_24810";
   var docs = [];
   var expResult = [];
   for( var i = 0; i < 20; i++ )
   {
      docs.push( { a: i, b: i } );
      expResult.push( { a: i, b: i + 1 } );
   }
   dbcl.insert( docs );

   db.transBegin();

   // 开启事务后查询10条数据并校验集合锁
   dbcl.find( { a: { "$lt": 10 } } ).hint( { "": indexName } ).toArray();
   checkCLLockType( db, LOCK_IS );

   // 再次查询10条数据并校验集合锁
   dbcl.find( { a: 11 } ).hint( { "": indexName } ).toArray();
   checkCLLockType( db, LOCK_S );
   checkIsLockEscalated( db, true );

   // 更新所有数据
   dbcl.update( { "$inc": { b: 1 } } );

   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );

   dbcl.remove();

   db.transCommit();

   var actResult = dbcl.find();
   commCompareResults( actResult, [] );
}
