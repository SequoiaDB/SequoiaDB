/******************************************************************************
@Description: seqDB-7499:对多个子表做切分后插入/删除数据，remove带条件删除
@modify list:
   2014-7-30   pusheng Ding  Init
   2019-4-15   xiaoni huang  modify
*******************************************************************************/

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

   var mclName = "mcl_7499";
   var sclName1 = "scl_7499_1";
   var sclName2 = "scl_7499_2";
   var groups = commGetGroups( db, false, "", false, true, true );
   var srcRG = groups[1][0].GroupName;
   var trgRG = groups[2][0].GroupName;

   commDropCL( db, COMMCSNAME, mclName, true, true, "clean main cl" );
   commDropCL( db, COMMCSNAME, sclName1, true, true, "clean sub cl1" );
   commDropCL( db, COMMCSNAME, sclName2, true, true, "clean sub cl2" );

   // create main cl
   var mOpt = { ShardingKey: { "a": 1 }, IsMainCL: true };
   var mainCL = commCreateCL( db, COMMCSNAME, mclName, mOpt, true, true );
   // create sub cl
   var sOpt = { ShardingKey: { a: 1 }, ShardingType: "hash", ReplSize: 0, Compressed: true, Group: srcRG };
   var subCL1 = commCreateCL( db, COMMCSNAME, sclName1, sOpt, true, true );
   var subCL2 = commCreateCL( db, COMMCSNAME, sclName2, sOpt, true, true );
   // attach cl
   mainCL.attachCL( COMMCSNAME + "." + sclName1, { LowBound: { a: 0, b: 1000 }, UpBound: { a: 1000, b: 0 } } );
   mainCL.attachCL( COMMCSNAME + "." + sclName2, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );

   // split
   subCL1.split( srcRG, trgRG, 50 );

   // insert   
   var recordsNum = 2000;
   var docs = [];
   for( var i = 0; i < recordsNum; ++i )
   {
      docs.push( { a: i } );
   }
   mainCL.insert( docs );

   // remove in sub cl1
   var rmNum = 1000;
   subCL1.remove( { a: { $lt: rmNum } } );
   // check results
   var mclCnt = mainCL.count();
   var scl1Cnt = subCL1.count();
   var scl2Cnt = subCL2.count();
   var expMclCnt = recordsNum - rmNum;
   var expScl1Cnt = 0;
   if( Number( mclCnt ) !== expMclCnt || Number( scl1Cnt ) !== expScl1Cnt || Number( scl2Cnt ) !== expMclCnt ) 
   {
      throw new Error( "main check results"
         + "mclCnt: " + expMclCnt + ", scl1Cnt: " + expScl1Cnt + ", scl2Cnt: " + expMclCnt
         + "mclCnt: " + mclCnt + ", scl1Cnt: " + scl1Cnt + ", scl2Cnt: " + scl2Cnt );
   }

   // remove in main cl
   mainCL.remove( { a: { $lt: recordsNum } } );
   // check results
   var mclCnt = mainCL.count();
   var scl1Cnt = subCL1.count();
   var scl2Cnt = subCL2.count();
   var expCnt = 0;
   if( Number( mclCnt ) !== expCnt || Number( scl1Cnt ) !== expCnt || Number( scl2Cnt ) !== expCnt ) 
   {
      throw new Error( "main check results"
         + "mclCnt: " + expCnt + ", scl1Cnt: " + expCnt + ", scl2Cnt: " + expCnt
         + "mclCnt: " + mclCnt + ", scl1Cnt: " + scl1Cnt + ", scl2Cnt: " + scl2Cnt );
   }

   // clear env
   commDropCL( db, COMMCSNAME, mclName, true, false, "clean main cl" );
}
