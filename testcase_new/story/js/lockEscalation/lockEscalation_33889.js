/******************************************************************************
 * @Description   : seqDB-33889:执行批量删除导致锁升级，锁升级时读取的是内存的记录
 * @Author        : tangtao
 * @CreateTime    : 2023.11.07
 * @LastEditTime  : 2023.11.07
 * @LastEditors   : tangtao
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_lockEscalation_33889";

main( test );
function test ( testPara )
{
   var cl = testPara.testCL;
   var indexName = "test_33889";

   cl.insert( { _id: 1, a: 1 } );
   cl.insert( { _id: 2, a: 1 } );
   cl.insert( { _id: 3, a: 1 } );
   cl.insert( { _id: 4, a: 2 } );

   cl.createIndex( indexName, { a: 1 } );

   db.setSessionAttr( { TransMaxLockNum: 1 } );
   db.transBegin();
   cl.remove( { _id: 1, a: 1 }, { "": indexName } );
   // the rule is current lock number > TransMaxLockNum, next lock acquire will escalate
   // the last remove have only 2 locks at max, so no lock escalation
   checkLockEscalated( db, false );
   cl.remove( { _id: 4, a: 2 }, { "": indexName } );
   // the last remove have only 2 locks at max, so no lock escalation
   checkLockEscalated( db, false );
   cl.remove( { _id: 2, a: 1 }, { "": indexName } );
   // the last remove have 3 locks at max, so trigger lock escalation
   checkLockEscalated( db, true );
   cl.remove( { _id: 3, a: 1 }, { "": indexName } );
   db.transCommit();
}