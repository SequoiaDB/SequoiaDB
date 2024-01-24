/************************************
*@Description: seqDB-22497:备节点回放统计信息，检查重放结果
*@Author:      2020/07/29  liuli
**************************************/

testConf.skipStandAlone = true;
testConf.clName = CHANGEDPREFIX + "_22497";
testConf.clOpt = {};
testConf.useSrcGroup = true;

main( test );

function test ()
{
   // 获取 group 信息
   var dataGroupName = testPara.srcGroupName;

   // 连接主节点和备节点
   db1 = db.getRG( dataGroupName ).getSlave().connect();
   db2 = db.getRG( dataGroupName ).getMaster().connect();

   // 备节点回放统计信息
   db1.analyze();
   db.analyze();

   // 检查组的LSN是否一致
   commCheckLSN( db, dataGroupName );
   // 关闭连接
   db1.close();
   db2.close();
}

