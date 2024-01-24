/******************************************************************************
 * @Description   : seqDB-24819:CL 层面 S 锁升级为 S(DDL) 锁 
 * @Author        : liuli
 * @CreateTime    : 2021.12.23
 * @LastEditTime  : 2021.12.30
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_24819";

main( test );
function test ( testPara )
{
   var sessionAttr = { TransIsolation: 2, TransMaxLockNum: 10 };
   db.setSessionAttr( sessionAttr );

   var dbcl = testPara.testCL;
   var dbcs = testPara.testCS;
   var clNewName = "cl_new_24819";
   var indexName = "index_24819";
   var docs = insertData( dbcl, 30 );
   dbcl.createIndex( indexName, { a: 1 } );

   db.transBegin();
   // 读取前20条数据后校验集合锁
   dbcl.find( { a: { "$lt": 20 } } ).hint( { "": indexName } ).toArray();
   checkCLLockType( db, LOCK_S );

   // 执行renameCL，读取后10条数据
   dbcs.renameCL( testConf.clName, clNewName );
   checkCLLockType( db, LOCK_S );
   dbcl = dbcs.getCL( clNewName );
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, docs );
   checkCLLockType( db, LOCK_S );
   db.transCommit();
}
