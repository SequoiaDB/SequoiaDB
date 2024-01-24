/************************************************************************
*@Description:  	seqDB-6216:子表创建索引后再主表创建相同索引_SD.subCL.01.024
*@Author:   2015/11/23   huangxiaoni
************************************************************************/
main();

function main ()
{
   if( commIsStandalone( db ) )
   {
      println( " Deploy mode is standalone!" );
      return;
   }

   var csName = COMMCSNAME;
   var mainCLName = COMMCLNAME + "_mcl";
   var subCLName1 = COMMCLNAME + "_scl01";
   var subCLName2 = COMMCLNAME + "_scl02";
   var idxName = COMMCLNAME + "_idx";

   println( "\n---Begin to drop cl in the pre-condition." );
   commDropCL( db, csName, subCLName1, true, true, "Failed to drop CL[" + subCLName1 + "]" );
   commDropCL( db, csName, subCLName2, true, true, "Failed to drop CL[" + subCLName2 + "]" );
   commDropCL( db, csName, mainCLName, true, true, "Failed to drop CL[" + mainCLName + "]" );

   var mainCL = createMainCL( csName, mainCLName );
   var subCL1 = createSubCL( csName, subCLName1 );
   var subCL2 = createSubCL( csName, subCLName2 );

   var options1 = { LowBound: { "a": 0 }, UpBound: { "a": 100 } };
   var options2 = { LowBound: { "a": 100 }, UpBound: { "a": 200 } };
   attachCL( mainCL, csName, subCLName1, options1 );
   attachCL( mainCL, csName, subCLName2, options2 );

   insertRecs( mainCL );

   //create index and check scanType
   createIndex( csName, mainCL, subCL1, subCL2, idxName, subCLName1, subCLName2 );
   //drop index and check scanType
   dropIndex( mainCL, subCL1, subCL2, idxName );
   //check records
   checkResult( mainCL );

   println( "\n---Begin to drop subCS in the end-condition." );
   commDropCL( db, csName, subCLName1, true, false, "Failed to drop CL[" + subCLName1 + "]" );
   commDropCL( db, csName, subCLName2, true, false, "Failed to drop CL[" + subCLName2 + "]" );
   commDropCL( db, csName, mainCLName, true, false, "Failed to drop CL[" + mainCLName + "]" );
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
   println( "\n---Begin to create subCL[" + subCLName + "]." );

   var options = {
      ShardingKey: { a: 1 }, ShardingType: "range",
      ReplSize: 0, Compressed: true
   };
   var subCL = commCreateCL( db, csName, subCLName, options, false,
      true, "Failed to create subCL." );
   return subCL;
}

function attachCL ( mainCL, csName, subCLName, options )
{
   println( "\n---Begin to attach subCL[" + subCLName + "]." );

   mainCL.attachCL( csName + "." + subCLName, options );
}

function insertRecs ( mainCL )
{
   println( "\n---Begin to insert records." );

   mainCL.insert( { a: 0, b: 0 } );
   mainCL.insert( { a: 199, b: 199 } );
}

function createIndex ( csName, mainCL, subCL1, subCL2, idxName, subCLName1, subCLName2 )
{
   println( "\n---Begin to create index and check scanType." );

   //create an index on one of subCL
   subCL1.createIndex( idxName, { b: 1 } );
   subCL1.getIndex( idxName ).toObj()["IndexDef"]["name"];

   //check the index of subCL2
   try
   {
      subCL2.getIndex( idxName );
   }
   catch( e )
   {
      if( e !== -47 ) //-47:Index name does not exist
      {
         throw buildException( "createIndex", null, null,
            "[e:-47]", "[e:" + e + "]" );
      }
   }

   //create the same index in the mainCL
   mainCL.createIndex( idxName, { b: 1 } );
   var mainCLIdx = mainCL.getIndex( idxName ).toObj()["IndexDef"]["name"];
   var subCLIdx1 = subCL1.getIndex( idxName ).toObj()["IndexDef"]["name"];
   var subCLIdx2 = subCL2.getIndex( idxName ).toObj()["IndexDef"]["name"];

   //check the index in each cl
   if( subCLIdx1 !== idxName || mainCLIdx !== idxName
      || subCLIdx2 !== idxName )
   {
      throw buildException( "createIndex", null, "[create the same index in the mainCL and check it]",
         "[subCLIdx1:" + idxName + ",mainCLIdx:" + idxName
         + ",subCLIdx2:" + idxName + "]",
         "[subCLIdx1:" + subCLIdx1 + ",mainCLIdx" + mainCLIdx
         + ",subCLIdx2:" + subCLIdx2 + "]" );
   }

   //check the scanType in the subCL1
   var scanIdxInfo1 = mainCL.find( { b: 0 } ).explain().current().toObj()["SubCollections"][0];
   var scanType1 = scanIdxInfo1["ScanType"];
   var scanIdxName1 = scanIdxInfo1["IndexName"];
   if( scanType1 !== "ixscan" || scanIdxName1 !== idxName )
   {
      throw buildException( "createIndex", null, "[check scanType of subCL1]",
         "scanType1:ixscan,scanIdxName1:" + idxName + "]",
         "scanType1:" + scanType1 + "scanIdxName1" + scanIdxName1 + "]" );
   }

   //check the scanType in the subCL2
   var scanIdxInfo2 = mainCL.find( { b: 199 } ).explain().current().toObj()["SubCollections"][0];
   var scanType2 = scanIdxInfo2["ScanType"];
   var scanIdxName2 = scanIdxInfo2["IndexName"];
   if( scanType2 !== "ixscan" || scanIdxName2 !== idxName )
   {
      throw buildException( "createIndex", null, "[check scanType of subCL2]",
         "scanType2:ixscan,scanIdxName2:" + idxName + "]",
         "scanType2:" + scanType2 + "scanIdxName2" + scanIdxName2 + "]" );
   }
}

function dropIndex ( mainCL, subCL1, subCL2, idxName )
{
   println( "\n---Begin to drop index and check index." );

   //drop index in the mainCL
   mainCL.dropIndex( idxName );
   //check the index in the mainCL
   try
   {
      mainCL.getIndex( idxName );
   }
   catch( e )
   {
      if( e !== -47 )
      {
         throw buildException( "createIndex", null, "[drop the index,and check it in the subCL2]",
            "[e:-47]", "[e:" + e + "]" );
      }
   }
   //check the index in the subCL1
   try
   {
      subCL1.getIndex( idxName );
   }
   catch( e )
   {
      if( e !== -47 )
      {
         throw buildException( "createIndex", null, "[drop the index,and check it in the subCL1]",
            "[e:-47]", "[e:" + e + "]" );
      }
   }
   //check the index in the subCL2
   try
   {
      subCL2.getIndex( idxName );
   }
   catch( e )
   {
      if( e !== -47 )
      {
         throw buildException( "createIndex", null, "[drop the index,and check it in the subCL2]",
            "[e:-47]", "[e:" + e + "]" );
      }
   }
}

function checkResult ( mainCL )
{
   println( "\n---Begin to check records." );

   //check records
   var recsCount = mainCL.count();
   var insertRecsNum = 2;  //insert records
   if( parseInt( recsCount ) !== insertRecsNum )
   {
      throw buildException( "checkResult", null, "[check count of records]",
         "[recsCount:" + insertRecsNum + "]",
         "[recsCount:" + parseInt( recsCount ) + "]" );
   }
}