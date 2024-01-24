/******************************************************************************
 * @Description   : seqDB-24811:CL 层面 IS 锁升级为 S(DDL) 锁 
 * @Author        : liuli
 * @CreateTime    : 2021.12.14
 * @LastEditTime  : 2021.12.30
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_24811";

main( test );
function test ( testPara )
{
   var clNameNew = "cl_new_24811";
   var sessionAttr = { TransIsolation: 2, TransMaxLockNum: 10 };
   db.setSessionAttr( sessionAttr );

   // 使用配置session的会话获取db并插入数据
   var dbcs = testPara.testCS;
   var dbcl = testPara.testCL;
   var docs = insertData( dbcl, 20 );

   db.transBegin();

   // 开启事务后查询10条数据并校验集合锁
   dbcl.find().limit( 10 ).toArray();
   checkIsLockEscalated( db, false );
   checkCLLockType( db, LOCK_IS );

   // renameCL后校验集合锁
   dbcs.renameCL( testConf.clName, clNameNew );
   checkIsLockEscalated( db, false );
   checkCLLockType( db, LOCK_S );

   // 再次查询10条数据并校验集合锁
   var dbcl = dbcs.getCL( clNameNew );
   dbcl.find().skip( 10 ).limit( 10 ).toArray();
   checkIsLockEscalated( db, false );
   checkCLLockType( db, LOCK_S );
   db.transCommit();

   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, docs );
}
