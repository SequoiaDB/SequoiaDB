/******************************************************************************
 * @Description   : seqDB-24825:CL 层面 X 锁升级为 S 锁
 * @Author        : liuli
 * @CreateTime    : 2021.12.14
 * @LastEditTime  : 2021.12.14
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_24825";

main( test );
function test ( testPara )
{
   var sessionAttr = { TransIsolation: 2, TransMaxLockNum: 10 };
   db.setSessionAttr( sessionAttr );

   var dbcl = testPara.testCL;
   var docs = [];
   var docs1 = [];
   var docs2 = [];
   for( var i = 0; i < 30; i++ )
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
   // 开启事务后写入20条新数据
   dbcl.insert( docs2 );
   checkCLLockType( db, LOCK_X );

   // 读取原来的10条数据
   dbcl.find( { a: { $lt: 10 } } ).toArray();
   checkCLLockType( db, LOCK_X );
   db.transCommit();
}