/******************************************************************************
@Description: seqDB-10439:数据落在不同组上的相同子表中，批量插入数据
@modify list:
   2016.11.23  zengxianquan  Init
   2019-4-15   xiaoni huang  modify
******************************************************************************/

main( test );
function test ()
{
   if( true == commIsStandalone( db ) )
   {
      return;
   }
   if( commGetGroupsNum( db ) < 2 )
   {
      return;
   }
   db.setSessionAttr( { PreferedInstance: "M" } );

   var mclName = "mcl_10439";
   var sclName = "scl_10439";
   var groups = commGetGroups( db, false, "", false, true, true );
   var srcRG = groups[1][0].GroupName;
   var trgRG = groups[2][0].GroupName;

   // clear env
   commDropCL( db, COMMCSNAME, mclName, true, true, "drop mcl in the begin" );
   commDropCL( db, COMMCSNAME, sclName, true, true, "drop scl in the begin" );

   // create cs and cl, attach cl
   var mclOpt = { "ShardingKey": { a: 1 }, "IsMainCL": true };
   var mainCL = commCreateCL( db, COMMCSNAME, mclName, mclOpt, true, false );

   var sOpt = { ShardingKey: { a: 1 }, ShardingType: "range", Group: srcRG };
   var subCL = commCreateCL( db, COMMCSNAME, sclName, sOpt, true, true );

   mainCL.attachCL( COMMCSNAME + "." + sclName, { LowBound: { a: 0 }, UpBound: { a: 100 } } );

   // split and create index
   subCL.split( srcRG, trgRG, { a: 50 }, { a: 100 } );

   // insert
   var recordsNum = 100;
   var docs = [];
   for( var i = 0; i < recordsNum; ++i )
   {
      docs.push( { a: i } );
   }
   mainCL.insert( docs );

   // check total count
   var totalCnt = mainCL.count();
   assert.equal( totalCnt, recordsNum );

   // check srcRG count
   var rg1 = db.getRG( srcRG ).getMaster().connect();
   var expRG1Cnt = 50;
   var actRG1Cnt = rg1.getCS( COMMCSNAME ).getCL( sclName ).count();
   assert.equal( actRG1Cnt, expRG1Cnt );

   // check trgRG count
   var rg2 = db.getRG( trgRG ).getMaster().connect();
   var expRG2Cnt = 50;
   var actRG2Cnt = rg2.getCS( COMMCSNAME ).getCL( sclName ).count();
   assert.equal( actRG2Cnt, expRG2Cnt );

   // clear env
   commDropCL( db, COMMCSNAME, mclName, true, false, "drop mcl in the end" );
}