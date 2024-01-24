/******************************************************************************
*@Description : seqDB-22178:getDetail()获取主子表的集合快照信息֤，多个子表在不同组上
*@author:      wuyan
*@createdate:  2020.05.09
******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;

testConf.clName = COMMCLNAME + "_snapshot_maincl_22178b";
testConf.clOpt = { IsMainCL: true, ShardingKey: { a: 1 }, ShardingType: "range", Compressed: true };
main( test );

function test ( testPara )
{
   var groups = commGetGroups( db );
   var groupName1 = groups[0][0].GroupName;
   var groupName2 = groups[1][0].GroupName;
   var subclName1 = COMMCLNAME + "_snapshot_subcl1_22178b";
   var subclName2 = COMMCLNAME + "_snapshot_subcl2_22178b";
   commDropCL( db, COMMCSNAME, subclName1, true, true );
   commDropCL( db, COMMCSNAME, subclName2, true, true );
   commCreateCL( db, COMMCSNAME, subclName1, { ShardingKey: { no: 1 }, ShardingType: "hash", Group: groupName1 } );
   commCreateCL( db, COMMCSNAME, subclName2, { ShardingKey: { no: 1 }, ShardingType: "range", Group: groupName2 } );

   attachCLAndInsertRecs( testPara.testCL, subclName1, subclName2 )

   getDetailAndCheckResult( groupName1, groupName2, testPara.testCL, testConf.clNamee, subclName1, subclName2 );
}

function getDetailAndCheckResult ( groupName1, groupName2, maincl, mainclName, subclName1, subclName2 )
{
   var subclSnapshot1 = getCLSnapshotFromMasterNode( groupName1, subclName1 );
   var subclSnapshot2 = getCLSnapshotFromMasterNode( groupName2, subclName2 );
   var totalRecords1 = subclSnapshot1.Details[0]["TotalRecords"];
   var totalDataWrite1 = subclSnapshot1.Details[0]["TotalDataWrite"];
   var totalIndexWrite1 = subclSnapshot1.Details[0]["TotalIndexWrite"];
   var totalInsert1 = subclSnapshot1.Details[0]["TotalInsert"];
   var totalRecords2 = subclSnapshot1.Details[0]["TotalRecords"];
   var totalDataWrite2 = subclSnapshot1.Details[0]["TotalDataWrite"];
   var totalIndexWrite2 = subclSnapshot1.Details[0]["TotalIndexWrite"];
   var totalInsert2 = subclSnapshot1.Details[0]["TotalInsert"];
   var allSnapshotInfo = [];
   var getDetailCur = maincl.getDetail();
   var count = 0;
   while( getDetailCur.next() )
   {
      var snapshotInfo = getDetailCur.current().toObj();
      allSnapshotInfo.push( snapshotInfo );
      //选择部分有变化的字段值比较结果
      if( snapshotInfo.Details[0]["GroupName"] == groupName1 )
      {
         compareObjBySpecifyField( subclSnapshot1.Details[0], snapshotInfo.Details[0] );
      }
      else 
      {
         compareObjBySpecifyField( subclSnapshot2.Details[0], snapshotInfo.Details[0] );
      }
      count++;
   }

   //检查getDetail()获取的快照信息数量，子表在多个组上返回多条快照信息
   var expSnapshotCount = 2;
   if( Number( count ) !== expSnapshotCount )  
   {
      throw new Error( "actSnapshot is \n" + JSON.stringify( allSnapshotInfo ) );
   }

}

function compareObjBySpecifyField ( left, right )
{
   var fields = ["NodeName", "TotalRecords", "TotalDataWrite", "TotalIndexWrite", "TotalInsert"];
   for( var i = 0; i < fields.length; i++ )
   {
      var field = fields[i];
      if( left[field] !== right[field] )
      {
         throw new Error( "field is " + field + "\nExpResult is" + JSON.stringify( left )
            + "\n but actResult is \n" + JSON.stringify( right ) );
      }
   }
}

function attachCLAndInsertRecs ( dbcl, subCLName1, subCLName2 )
{
   dbcl.attachCL( COMMCSNAME + "." + subCLName1, { "LowBound": { a: 0 }, "UpBound": { a: 100 } } );
   dbcl.attachCL( COMMCSNAME + "." + subCLName2, { "LowBound": { a: 100 }, "UpBound": { a: 300 } } );

   insertRecs( dbcl, 300 );
}
