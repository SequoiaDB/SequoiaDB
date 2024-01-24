/************************************************************************
*@Description:  	seqDB-819:主表和子表属于相同的CS,在主表做index_SD.subCL.01.006
*@Author:   2015/10/27   huangxiaoni
************************************************************************/
main();

function main ()
{
   try
   {
      if( commIsStandalone( db ) )
      {
         println( " Deploy mode is standalone!" );
         return;
      }

      var csName = COMMCSNAME;
      var mainCLName = COMMCLNAME + "_mcl";
      var subCLName = COMMCLNAME + "_scl";
      var indexName = "testIndex_mcl";

      println( "\n---Begin to drop cl in the pre-condition." );
      commDropCL( db, csName, subCLName, true, true, "Failed to drop CL." );

      var mainCL = createMainCL( csName, mainCLName );
      var subCL = createSubCL( csName, subCLName );
      attachCL( csName, mainCL, subCLName );

      createIndex( mainCL, indexName );
      var insertRecsNum = 10;
      insertRecs( mainCL, insertRecsNum );
      checkResult( csName, mainCL, subCL, indexName, insertRecsNum );

      println( "\n---Begin to drop cl in the end-condition." );
      commDropCL( db, csName, subCLName, true, false, "Failed to drop CL." );
   }
   catch( e )
   {
      throw e;
   }
}

function createMainCL ( csName, mainCLName )
{
   println( "\n---Begin to create mainCL." );

   var options = { ShardingKey: { a: 1 }, IsMainCL: true };
   var mainCL = commCreateCL( db, csName, mainCLName, options, false,
      true, "Failed to create mainCL." );
   return mainCL;
}

function createSubCL ( csName, subCLName )
{
   println( "\n---Begin to create subCL." );

   var options = {
      ShardingKey: { a: 1 }, ShardingType: "range",
      ReplSize: 0, Compressed: true
   };
   var subCL = commCreateCL( db, csName, subCLName, options, false,
      true, "Failed to create subCL." );
   return subCL;
}

function attachCL ( csName, mainCL, subCLName )
{
   println( "\n---Begin to attach CL." );

   var options = { LowBound: { "a": 0 }, UpBound: { "a": 100 } };
   mainCL.attachCL( csName + "." + subCLName, options );
}

function createIndex ( mainCL, indexName )
{
   println( "\n---Begin to create index." );

   mainCL.createIndex( indexName, { b: 1 } );
}

function insertRecs ( mainCL, insertRecsNum )
{
   println( "\n---Begin to insert records." );

   for( i = 0; i < insertRecsNum; i++ )
   {
      mainCL.insert( { a: i, b: i } );
   }
}

function checkResult ( csName, mainCL, subCL, indexName, insertRecsNum )
{
   println( "\n---Begin to check result." );

   //compare scanType of find in the mainCL
   var mainCLFind = mainCL.find( { b: 5 } ).explain().current().toObj();
   var mainCLScanType = mainCLFind.SubCollections[0]["ScanType"];
   var mainCLIndexName = mainCLFind.SubCollections[0]["IndexName"];
   if( mainCLScanType !== "ixscan" || mainCLIndexName !== indexName )
   {
      throw buildException( "checkResult", null, "[compare scanType of find in the mainCL]",
         "[mainCLScanType:ixscan,mainCLIndexName:" + indexName + "]",
         "[mainCLScanType:" + mainCLScanType + ",mainCLIndexName:" + mainCLIndexName + "]" );
   }

   //compare scanType of find in the subCL
   var subCLFind = subCL.find( { b: 5 } ).explain().current().toObj();
   var subCLScanType = subCLFind["ScanType"];
   var subCLIndexName = subCLFind["IndexName"];
   if( subCLScanType !== "ixscan" || subCLIndexName !== indexName )
   {
      throw buildException( "checkResult", null, "[compare scanType of find in the mainCL]",
         "[subCLScanType:ixscan,subCLIndexName:" + indexName + "]",
         "[subCLScanType:" + subCLScanType + ",subCLIndexName:" + subCLIndexName + "]" );
   }

   //compare count of records
   var tolRecsCnt = mainCL.count();
   if( parseInt( tolRecsCnt ) !== insertRecsNum )
   {
      throw buildException( "checkResult", null, "[compare count of records]",
         "[tolRecsCnt:" + insertRecsNum + "]",
         "[tolRecsCnt:" + parseInt( tolRecsCnt ) + "]" );
   }
}