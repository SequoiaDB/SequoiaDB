/******************************************************************************
 * @Description   : seqDB-26605:连接1开启事务删除记录，连接2关闭事务插入与该记录冲突的记录
 * @Author        : Lin Yingting
 * @CreateTime    : 2022.06.10
 * @LastEditTime  : 2022.06.11
 * @LastEditors   : Lin Yingting
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_26605";

main( test );

function test ( testPara )
{
   var db1 = new Sdb( COORDHOSTNAME, COORDSVCNAME );
   var db2 = new Sdb( COORDHOSTNAME, COORDSVCNAME );
   var clName = testConf.clName;

   //普通表创建唯一索引，插入记录
   var cl = testPara.testCL;
   cl.createIndex( "idx_a", { a: 1 }, true, true );
   cl.insert( { a: 1, b: 1 } );

   //连接1开启事务，删除记录，不提交事务
   db1.transBegin();
   var cl1 = db1.getCS( COMMCSNAME ).getCL( clName );
   cl1.remove( { a: 1, b: 1 } );

   //连接2不开启事务，指定更新规则，插入与该记录冲突的记录
   var cl2 = db2.getCS( COMMCSNAME ).getCL( clName );
   assert.tryThrow( SDB_IXM_DUP_KEY, function ()
   {
      cl2.insert( { a: 1, b: 2 }, { UpdateOnDup: true, Update: { "$inc": { "a": 1 } } } );
   } );

   //连接1提交事务
   db1.transCommit();
   var actRes = cl.find().sort( { a: 1 } );
   var expRes = "";
   commCompareResults( actRes, expRes );
}
