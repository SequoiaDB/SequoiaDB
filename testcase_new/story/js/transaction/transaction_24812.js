/******************************************************************************
 * @Description   : seqDB-24812:CL 层面 IS 锁升级为 X 锁
 * @Author        : liuli
 * @CreateTime    : 2021.12.14
 * @LastEditTime  : 2021.12.30
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_24812";

main( test );
function test ( testPara )
{
   var sessionAttr = { TransIsolation: 2, TransMaxLockNum: 10 };
   db.setSessionAttr( sessionAttr );

   var dbcl = testPara.testCL;
   var docs = [];
   var expResult = [];
   for( var i = 0; i < 10; i++ )
   {
      docs.push( { a: i, b: i } );
      expResult.push( { a: i, b: i + 1 } );
   }
   expResult.push( { a: 10, b: 11 } );
   expResult.push( { a: 11, b: 12 } );
   dbcl.insert( docs );

   db.transBegin();

   dbcl.find().toArray();
   checkCLLockType( db, LOCK_IS );

   // 读取10条数据之后再写入一条数据，IS锁升级为X锁
   dbcl.insert( { a: 10, b: 10 } );
   dbcl.insert( { a: 11, b: 11 } );
   checkCLLockType( db, LOCK_X );
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
