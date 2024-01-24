/******************************************************************************
 * @Description   : seqDB-26430:锁升级后markDeleting的记录回滚验证
 * @Author        : zhang Yanan
 * @CreateTime    : 2022.04.26
 * @LastEditTime  : 2022.04.26
 * @LastEditors   : Zhang Yanan
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_26430";

main( test );
function test ( testPara )
{
   var sessionAttr = { TransMaxLockNum: 2 };
   db.setSessionAttr( sessionAttr );

   var dbcl = testPara.testCL;

   var docs = [];
   docs.push( { "a": 1 } );
   dbcl.insert( docs );
   // 开启事务后删除所有数据
   db.transBegin();
   dbcl.remove();

   dbcl.insert( { "a": 2 } );
   dbcl.insert( { "a": 3 } );
   dbcl.insert( { "a": 4 } );
   dbcl.update( { $set: { "b": 1 } } );

   // 回滚事务后校验结果
   db.transRollback();
   var actResult = dbcl.find();
   commCompareResults( actResult, docs );
}