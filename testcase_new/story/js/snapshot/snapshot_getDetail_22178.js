/******************************************************************************
*@Description : seqDB-22178:getDetail()获取主子表的集合快照信息֤
*@author:      wuyan
*@createdate:  2020.05.09
******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ( testPara )
{
   var groupName = testPara.groups[0][0].GroupName;
   var mainclName = COMMCLNAME + "_snapshot_maincl_22178";
   var subclName1 = COMMCLNAME + "_snapshot_subcl1_22178";
   var subclName2 = COMMCLNAME + "_snapshot_subcl2_22178";
   commDropCL( db, COMMCSNAME, mainclName, true, true );
   commDropCL( db, COMMCSNAME, subclName1, true, true );
   commDropCL( db, COMMCSNAME, subclName2, true, true );
   var dbcl = commCreateCL( db, COMMCSNAME, mainclName, { IsMainCL: true, ShardingKey: { a: 1 }, ShardingType: "range", Compressed: true } );
   commCreateCL( db, COMMCSNAME, subclName1, { ShardingKey: { no: 1 }, ShardingType: "hash", Group: groupName } );
   commCreateCL( db, COMMCSNAME, subclName2, { ShardingKey: { no: 1 }, ShardingType: "range", Group: groupName } );

   attachCLAndInsertRecs( dbcl, subclName1, subclName2 );

   getDetailAndCheckResult( groupName, dbcl, mainclName, subclName1, subclName2 );

   commDropCL( db, COMMCSNAME, mainclName, true, true );
}

function getDetailAndCheckResult ( groupName, maincl, mainclName, subclName1, subclName2 )
{
   var subclSnapshot1 = getCLSnapshotFromMasterNode( groupName, subclName1 );
   var subclSnapshot2 = getCLSnapshotFromMasterNode( groupName, subclName2 );
   var allSnapshotInfo = [];
   var count = 0;
   var getDetailCur = maincl.getDetail();
   while( getDetailCur.next() )
   {
      var snapshotInfo = getDetailCur.current().toObj();
      allSnapshotInfo.push( snapshotInfo );
      var checkFields = ["NodeName", "TotalRecords", "TotalDataWrite", "TotalIndexWrite", "TotalInsert"];
      for( i in checkFields )
      {
         var fieldName = checkFields[i];
         if( !checkFieldsValue( snapshotInfo.Details[0], subclSnapshot1.Details[0], subclSnapshot2.Details[0], fieldName ) )
         {
            throw new Error( "check field=" + field + "\nExpResult is \n" + JSON.stringify( subclSnapshot1 ) + "\n" + JSON.stringify( subclSnapshot2 )
               + "\n but actResult is \n" + JSON.stringify( snapshotInfo ) );
         }
      }
      count++;
   }

   //检查getDetail()获取的快照信息数量,子表在一个组上返回一条信息
   var expSnapshotCount = 1;
   if( Number( count ) !== expSnapshotCount )  
   {
      throw new Error( "actSnapshot is \n" + JSON.stringify( allSnapshotInfo ) );
   }
}

function checkFieldsValue ( total, subcl1, subcl2, fieldName )
{
   if( fieldName === "NodeName" )
   {
      if( total[fieldName] === subcl1[fieldName] && total[fieldName] === subcl2[fieldName] )
      {
         return true;
      }
      else
      {
         return false;
      }
   }
   else
   {
      if( total[fieldName] === subcl1[fieldName] + subcl2[fieldName] )
      {
         return true;
      }
      else
      {
         return false;
      }
   }
}

function attachCLAndInsertRecs ( dbcl, subCLName1, subCLName2 )
{
   dbcl.attachCL( COMMCSNAME + "." + subCLName1, { "LowBound": { a: 0 }, "UpBound": { a: 100 } } );
   dbcl.attachCL( COMMCSNAME + "." + subCLName2, { "LowBound": { a: 100 }, "UpBound": { a: 200 } } );

   insertRecs( dbcl, 200 );
}
