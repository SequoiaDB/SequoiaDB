/******************************************************************************
*@Description : seqDB-22177:getDetail()获取切分表的集合快照信息֤
*@author:      wuyan
*@createdate:  2020.05.09
******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;

testConf.useSrcGroup = true;
testConf.useDstGroup = true;
testConf.clOpt = { ShardingKey: { a: 1 }, ShardingType: "hash" };
testConf.clName = COMMCLNAME + "_snapshot_22177";
main( test );

function test ( testPara )
{
   var srcGroupName = testPara.srcGroupName;
   var dstGroupName = testPara.dstGroupNames[0];
   insertRecs( testPara.testCL );
   testPara.testCL.split( srcGroupName, dstGroupName, 50 );
   getDetailAndCheckResult( testPara.testCL, srcGroupName, dstGroupName );
}

function getDetailAndCheckResult ( dbcl, srcGroupName, dstGroupName )
{
   var getDetailCur = dbcl.getDetail();
   var count = 0;
   var allSnapshotInfo = [];
   while( getDetailCur.next() )
   {
      var snapshotInfo = getDetailCur.current().toObj();
      allSnapshotInfo.push( snapshotInfo );
      if( snapshotInfo.Details[0]["GroupName"] == srcGroupName )
      {
         //由于时间不一致，剔除"ResetTimestamp"字段后比较结果
         var srcCLSnapshot = getCLSnapshotFromMasterNode( srcGroupName, testConf.clName );
         delete snapshotInfo.Details[0]["ResetTimestamp"];
         delete srcCLSnapshot.Details[0]["ResetTimestamp"];
         checkResult( snapshotInfo, srcCLSnapshot );
      }
      else if( snapshotInfo.Details[0]["GroupName"] == dstGroupName )
      {
         var dstCLSnapshot = getCLSnapshotFromMasterNode( dstGroupName, testConf.clName );
         delete snapshotInfo.Details[0]["ResetTimestamp"];
         delete dstCLSnapshot.Details[0]["ResetTimestamp"];
         checkResult( snapshotInfo, dstCLSnapshot );
      }
      else
      {
         throw new Error( "ExpResult is \n" + JSON.stringify( srcCLSnapshot ) + "\n" + JSON.stringify( dstCLSnapshot )
            + "\n but actResult is \n" + JSON.stringify( snapshotInfo ) );
      }
      count++;
   }
}
