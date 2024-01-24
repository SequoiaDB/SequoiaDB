/******************************************************************************
 * @Description   : seqDB-24855:事务执行期间查询事务快照
 * @Author        : liuli
 * @CreateTime    : 2021.12.15
 * @LastEditTime  : 2021.12.15
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_24855";

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;

   // 开启事务后插入一条数据
   db.transBegin();
   dbcl.insert( { a: 1 } );

   // 校验事务快照和当前事务快照
   var cursor = db.snapshot( SDB_SNAP_TRANSACTIONS );
   checkSnapshot( cursor );

   var cursor = db.snapshot( SDB_SNAP_TRANSACTIONS_CURRENT );
   checkSnapshot( cursor );

   var cursor = db.exec( "select * from $SNAPSHOT_TRANS" );
   checkSnapshot( cursor );

   var cursor = db.exec( "select * from $SNAPSHOT_TRANS_CUR" )
   checkSnapshot( cursor );

   db.transCommit();
}

function checkSnapshot ( cursor )
{
   while( cursor.next() )
   {
      var obj = cursor.current().toObj();
      var isLockEscalated = obj.IsLockEscalated;
      assert.equal( isLockEscalated, false );
   }
   cursor.close();
}