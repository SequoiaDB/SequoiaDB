/****************************************************
@description:     test analyze group
@testlink cases:  seqDB-14232
@modify list:
2018-07-30        linsuqiang init
****************************************************/

main( test );

function test ()
{
   if( commIsStandalone( db ) || commGetGroupsNum( db ) < 2 )
   {
      return;
   }

   var csName = "analyze14232";
   commDropCS( db, csName, true, "fail to drop cs" )
   var options = { PageSize: 4096 };
   var cs = commCreateCS( db, csName, true, "fail to create cl", options )

   var groups = commGetGroups( db );
   var analyzeGroup = groups[0][0]['GroupName'];
   var nonAnalyzeGroup = groups[1][0]['GroupName'];
   var clName = "analyze14232";

   var options = {
      ShardingKey: { a: 1 }, ShardingType: 'range',
      Group: analyzeGroup
   };
   var cl = cs.createCL( clName, options );
   cl.split( analyzeGroup, nonAnalyzeGroup, { a: 1000 }, { a: 3000 } );
   var str = getString( 4096 );
   var analyzeRec = { a: 0, b: str }; // record on analyzeGroup
   insertData( cl, analyzeRec );
   var nonAnalyzeRec = { a: 2000 }; // record on nonAnalyzeGroup
   insertData( cl, nonAnalyzeRec );

   checkScanTypeByExplain( cl, "ixscan", nonAnalyzeRec );
   checkScanTypeByExplain( cl, "ixscan", nonAnalyzeRec );
   tryCatch( ["cmd=analyze", "options={GroupName:\"" + analyzeGroup + "\"}"], [0], "fail to analyze" );
   checkScanTypeByExplain( cl, "tbscan", analyzeRec );
   checkScanTypeByExplain( cl, "ixscan", nonAnalyzeRec );

   db.dropCS( csName );
}

function insertData ( cl, rec )
{
   var recs = [];
   var recNum = 2000;
   for( var i = 0; i < recNum; i++ )
   {
      recs.push( rec );
   }
   cl.insert( recs );
}
