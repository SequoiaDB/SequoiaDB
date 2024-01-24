/******************************************************************************
*@Description : test insert special decimal value to range/hash table
*               seqDB-13998:水平分区表插入特殊decimal值           
*@author      : Liang XueWang 
******************************************************************************/

main( test )
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }
   var groups = getGroupName( db );
   if( groups.length < 2 )
   {
      return;
   }

   // test range split cl
   var clName = COMMCLNAME + "_13998";
   commDropCL( db, COMMCSNAME, clName );
   var option = { ShardingKey: { a: 1 }, ShardingType: "range" };
   var cl = commCreateCL( db, COMMCSNAME, clName, option );

   var docs = [{ a: { $decimal: "MAX" } },
   { a: { $decimal: "MIN" } },
   { a: { $decimal: "NaN" } }];
   cl.insert( docs );

   var startCond = { a: 10 };
   var splitGrpInfo = ClSplitOneTimes( COMMCSNAME, clName, startCond );
   var expRecs = [[{ a: { $decimal: "MIN" } }, { a: { $decimal: "NaN" } }],
   [{ a: { $decimal: "MAX" } }]];
   checkRangeClSplitResult( db, clName, splitGrpInfo, {}, {}, expRecs, { _id: 1 } );

   // test hash split cl
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL after test range split" );
   option = { ShardingKey: { a: 1 }, ShardingType: "hash" };
   cl = commCreateCL( db, COMMCSNAME, clName, option, true, true );

   cl.insert( docs );
   splitGrpInfo = ClSplitOneTimes( COMMCSNAME, clName, 0.5 );
   var expRecsNum = 3;
   checkHashClSplitResult( db, clName, splitGrpInfo, {}, {}, expRecsNum, { _id: 1 } );

   commDropCL( db, COMMCSNAME, clName );
}
