/******************************************************************************
 * @Description   : seqDB-24814:验证 IX 是否能升级为 SIX
 * @Author        : Yao Kang
 * @CreateTime    : 2021.12.14
 * @LastEditTime  : 2022.01.04
 * @LastEditors   : Yao Kang
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_24814";

main( test );
function test ()
{
   var sessionAttr = { TransIsolation: 2, TransMaxLockNum: 10 };
   db.setSessionAttr( sessionAttr );
   var dbcl = testPara.testCL;

   insertData( dbcl, 10 );

   db.transBegin();
   insertData( dbcl, 10 );
   checkCLLockType( db, LOCK_IX );
   dbcl.find().limit( 10 ).toArray();
   checkCLLockType( db, LOCK_SIX );
   checkIsLockEscalated( db, true );
   dbcl.update( { $set: { phone: 13600000000 } } );
   var results = dbcl.find();
   var resultArray = results.toArray();
   for( var i = 0; i < resultArray.length; i++ )
   {
      var result = JSON.parse( resultArray[i] );
      assert.equal( result.phone, 13600000000 );
   }
   dbcl.remove();
   assert.equal( dbcl.find().toArray().length, 0 );
   db.transCommit();
}
