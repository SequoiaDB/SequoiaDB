/******************************************************************************
 * @Description   : seqDB-24870:CL 层面 SIX 锁升级为 X 锁
 * @Author        : liuli
 * @CreateTime    : 2021.12.20
 * @LastEditTime  : 2022.01.03
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_24870";

main( test );
function test ( testPara )
{
   var sessionAttr = { TransIsolation: 2, TransMaxLockNum: 10 };
   db.setSessionAttr( sessionAttr );

   var dbcl = testPara.testCL;
   var indexName = "index_24870";
   dbcl.createIndex( indexName, { a: 1 } );

   var docs = [];
   var docs1 = [];
   var docs2 = [];
   var docs3 = [];
   for( var i = 0; i < 30; i++ )
   {
      if( i < 10 )
      {
         docs1.push( { a: i } );
      }
      else if( i < 20 )
      {
         docs2.push( { a: i } );
      }
      else
      {
         docs3.push( { a: i } );
      }
      docs.push( { a: i } );
   }
   dbcl.insert( docs1 );

   // 开启事务后写入10条数据
   db.transBegin();
   dbcl.insert( docs2 );

   // 读取集合原有的前10条数据
   dbcl.find( { a: { "$lt": 10 } } ).hint( { "": indexName } ).toArray();
   checkCLLockType( db, LOCK_SIX );

   // 再次写入10条数据
   dbcl.insert( docs3 );
   checkCLLockType( db, LOCK_X );

   db.transCommit();

   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, docs );
}