/****************************************************
@description:     test global analyze 
@testlink cases:  seqDB-14228
@modify list:
2018-07-30        linsuqiang init
****************************************************/

main( test );

function test ()
{
   var csBaseName = COMMCSNAME + "_14228";
   var csNum = 2;
   var clNumPerCs = 2;
   var clArray = [];

   for( var i = 0; i < csNum; i++ )
   {
      var csName = csBaseName + "_" + i;
      var options = { PageSize: 4096 };
      commCreateCS( db, csName, true, "fail to create cl", options )
      for( var j = 0; j < clNumPerCs; j++ )
      {
         var clName = csName + "_" + j;
         var cl = commCreateCL( db, csName, clName );
         clArray.push( cl );
         insertDataWithIndex( cl );
      }
   }

   for( var i = 0; i < clArray.length; i++ )
      checkScanTypeByExplain( clArray[i], "ixscan" );
   tryCatch( ["cmd=analyze"], [0], "fail to analyze" );
   for( var i = 0; i < clArray.length; i++ )
      checkScanTypeByExplain( clArray[i], "tbscan" );

   for( var i = 0; i < csNum; i++ )
   {
      var csName = csBaseName + "_" + i;
      db.dropCS( csName );
   }
}
