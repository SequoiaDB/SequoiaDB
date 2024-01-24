/******************************************************************************
*@Description : seqDB-22176:getDetail()获取普通表的集合快照信息֤
*@author:      wuyan
*@createdate:  2020.05.09
******************************************************************************/
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;
testConf.clOpt = { ShardingKey: { a: 1 } };
testConf.clName = COMMCLNAME + "_snapshot_22176";
main( test );

function test ( testPara )
{
   insertRecs( testPara.testCL );
   var getDetailInfo = testPara.testCL.getDetail().next().toObj();
   var clSnapshot = getCLSnapshotFromMasterNode( testPara.srcGroupName, testConf.clName );

   //由于时间不一致，剔除"ResetTimestamp"字段后比较结果
   delete getDetailInfo.Details[0]["ResetTimestamp"];
   delete clSnapshot.Details[0]["ResetTimestamp"];
   checkResult( getDetailInfo, clSnapshot );
}
