/******************************************************************************
 * @Description   : seqDB-26412:事务中无事务表数据操作
 * @Author        : liuli
 * @CreateTime    : 2022.04.24
 * @LastEditTime  : 2022.04.24
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_26412";
testConf.clOpt = { NoTrans: true, ShardingKey: { a: 1 }, AutoSplit: true };

main( test );
function test ( args )
{
   var dbcl = args.testCL;

   var recsNum = 1000;
   var docs = [];
   for( var i = 0; i < recsNum; i++ )
   {
      docs.push( { a: i, b: i } );
   }

   // 开启事务 
   db.transBegin();

   // 插入数据
   dbcl.insert( docs );

   // 获取事务锁
   var gotLocks = getTransLock( db );
   assert.equal( gotLocks, [] );

   // 回滚事务
   db.transRollback();

   // 校验数据
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, docs );
}

function getTransLock ( db )
{
   var cursor = db.snapshot( SDB_SNAP_TRANSACTIONS_CURRENT );
   while( cursor.next() )
   {
      var obj = cursor.current().toObj();
      var gotLocks = obj["GotLocks"];
   }
   cursor.close();
   return gotLocks;
}