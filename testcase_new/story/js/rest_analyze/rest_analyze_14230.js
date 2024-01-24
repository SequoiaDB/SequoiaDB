/****************************************************
@description:     test analyze cl
@testlink cases:  seqDB-14230
@modify list:
2018-07-30        linsuqiang init
****************************************************/

main( test );

function test ()
{
   var analyzeClName = COMMCLNAME + "_14230";
   var nonAnalyzeClName = COMMCLNAME + "_14230_2";
   var csName = COMMCSNAME + "_14230";
   commDropCS( db, csName, true );
   var options = { PageSize: 4096 };
   var cs = commCreateCS( db, csName, false, "fail to create cl", options )
   var analyzeCl = cs.createCL( analyzeClName );
   insertDataWithIndex( analyzeCl );
   var nonAnalyzeCl = cs.createCL( nonAnalyzeClName );
   insertDataWithIndex( nonAnalyzeCl );

   checkScanTypeByExplain( analyzeCl, "ixscan" );
   checkScanTypeByExplain( nonAnalyzeCl, "ixscan" );
   var clFullName = csName + "." + analyzeClName;
   tryCatch( ["cmd=analyze", "options={Collection: \"" + clFullName + "\"}"], [0], "fail to analyze." );
   checkScanTypeByExplain( analyzeCl, "tbscan" );
   checkScanTypeByExplain( nonAnalyzeCl, "ixscan" );
   db.dropCS( csName )
}
