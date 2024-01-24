/******************************************************************************
 * @Description   : seqDB-24906:CL层面不允许锁升级非事务操作
 * @Author        : Yao Kang
 * @CreateTime    : 2022.01.04
 * @LastEditTime  : 2022.01.04
 * @LastEditors   : Yao Kang
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_24906";

main( test );
function test ( testPara )
{
   var sessionAttr = { TransIsolation: 2, TransMaxLockNum: 0, TransAllowLockEscalation: false };
   db.setSessionAttr( sessionAttr );

   var dbcl = testPara.testCL;
   insertData( dbcl, 1 );
   dbcl.update( { $set: { phone: 13900000000 } } );
   var resultArray = dbcl.find().toArray();
   assert.equal( JSON.parse( resultArray[0] ).phone, 13900000000 );

   dbcl.remove();
   assert.equal( dbcl.find().toArray().length, 0 );
}