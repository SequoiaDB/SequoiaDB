/****************************************************
@description:     test analyze index
@testlink cases:  seqDB-14231
@modify list:
2018-07-30        linsuqiang init
****************************************************/
main( test );

function test ()
{
   var csName = COMMCSNAME + "_14231";
   commDropCS( db, csName, true );
   var options = { PageSize: 4096 };
   var cs = commCreateCS( db, csName, false, "fail to create cl", options )
   var clName = COMMCLNAME + "_14231";
   var cl = cs.createCL( clName );

   var indexNum = 5;
   insertData( cl, indexNum );
   var idxArray = createIndexes( cl, indexNum );
   var analyzeIdx = idxArray.pop();
   var nonAnalyzeIdxArray = idxArray;

   checkScanTypeByExplain( cl, analyzeIdx, "ixscan" );
   for( var i = 0; i < nonAnalyzeIdxArray.length; i++ )
      checkScanTypeByExplain( cl, nonAnalyzeIdxArray[i], "ixscan" );
   var clFullName = csName + "." + clName;
   var optStr = "options={Collection:\"" + clFullName + "\", Index:\"" + analyzeIdx + "\"}";
   tryCatch( ["cmd=analyze", optStr], [0], "fail to analyze." );
   checkScanTypeByExplain( cl, analyzeIdx, "tbscan" );
   for( var i = 0; i < nonAnalyzeIdxArray.length; i++ )
      checkScanTypeByExplain( cl, nonAnalyzeIdxArray[i], "ixscan" );

   commDropCS( db, csName, false );
}

function insertData ( cl, fieldNum )
{
   var rec = {};
   for( var i = 0; i < fieldNum; i++ )
   {
      fieldName = "field_" + i;
      var fieldName1 = "field_str_" + i;
      rec[fieldName] = 0;
      rec[fieldName1] = getString( 4096 );
   }
   var recs = [];
   var recNum = 25
   for( var i = 0; i < recNum; i++ )
      recs.push( rec );
   cl.insert( recs );
}

function createIndexes ( cl, indexNum )
{
   var idxArray = [];
   for( var i = 0; i < indexNum; i++ )
   {
      var fieldName = "field_" + i;
      var indexName = "idx_" + fieldName;
      var indexDef = {};
      indexDef[fieldName] = 1;
      cl.createIndex( indexName, indexDef );
      idxArray.push( indexName );
   }
   return idxArray;
}

function checkScanTypeByExplain ( cl, indexName, expScanType )
{
   var fieldName = indexName.substr( 4, indexName.length );
   var cond = {};
   cond[fieldName] = 0;
   var cursor = cl.find( cond ).explain( { Run: true } );
   var actScanType = cursor.next().toObj().ScanType;
   cursor.close();
   if( expScanType !== actScanType )
   {
      throw new Error( "expect: " + expScanType + ", actual: " + actScanType );
   }
}
