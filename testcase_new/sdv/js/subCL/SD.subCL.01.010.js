/************************************************************************
*@Description:  	seqDB-823:挂载子表,在主表做remove_SD.subCL.01.010
*@Author:   2015/10/10   huangxiaoni
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
      var subCLName1 = COMMCLNAME + "_scl01";
      var subCLName2 = COMMCLNAME + "_scl02";

      println( "\n---Begin to drop cl in the pre-condition." );
      commDropCL( db, csName, subCLName1, true, true, "Failed to drop CL[" + subCLName1 + "]" );
      commDropCL( db, csName, subCLName2, true, true, "Failed to drop CL[" + subCLName2 + "]" );
      commDropCL( db, csName, mainCLName, true, true, "Failed to drop CL[" + mainCLName + "]" );

      var mainCL = createMainCL( csName, mainCLName );

      var subCL1 = createSubCL( csName, subCLName1 );
      var options1 = { LowBound: { "a": 0 }, UpBound: { "a": 50 } };
      attachCL( csName, mainCL, subCLName1, options1 );

      var subCL2 = createSubCL( csName, subCLName2 );
      var options2 = { LowBound: { "a": 50 }, UpBound: { "a": 100 } };
      attachCL( csName, mainCL, subCLName2, options2 );

      var insertRecsNum = 100;
      insertRecs( mainCL, insertRecsNum );
      removeRecs( mainCL );
      checkResult( csName, mainCL, subCL1, subCL2, insertRecsNum );

      println( "\n---Begin to drop cl in the end-condition." );
      commDropCL( db, csName, subCLName1, true, false, "Failed to drop CL[" + subCLName1 + "]" );
      commDropCL( db, csName, subCLName2, true, false, "Failed to drop CL[" + subCLName2 + "]" );
      commDropCL( db, csName, mainCLName, true, false, "Failed to drop CL[" + mainCLName + "]" );
   }
   catch( e )
   {
      throw e;
   }
}

function createMainCL ( csName, mainCLName )
{
   println( "\n---Begin to createCL[" + mainCLName + "]." );

   var options = { ShardingKey: { a: 1 }, IsMainCL: true };
   var mainCL = commCreateCL( db, csName, mainCLName, options, false,
      true, "Failed to create mainCL." );
   return mainCL;
}

function createSubCL ( csName, subCLName )
{
   println( "\n---Begin to createCL[" + subCLName + "]." );

   var options = {
      ShardingKey: { a: 1 }, ShardingType: "range",
      ReplSize: 0, Compressed: true
   };
   var subCL = commCreateCL( db, csName, subCLName, options, false,
      true, "Failed to create subCL." );
   return subCL;
}

function attachCL ( csName, mainCL, subCLName, options )
{
   println( "\n---Begin to attachCL[" + subCLName + "]." );

   mainCL.attachCL( csName + "." + subCLName, options );
}

function insertRecs ( mainCL, insertRecsNum )
{
   println( "\n---Begin to insert records." );

   for( i = 0; i < insertRecsNum; i++ )
   {
      mainCL.insert( { a: i, b: i } );
   }
}

function removeRecs ( mainCL )
{
   println( "\n---Begin to remove records." );

   mainCL.remove( { $and: [{ a: { $gte: 25 } }, { a: { $lt: 75 } }] } );
}

function checkResult ( csName, mainCL, subCL1, subCL2, insertRecsNum )
{
   println( "\n---Begin to check result." );

   //compare count of the rest records
   var tolRecsCnt = mainCL.count();
   var restRecsCnt = mainCL.find( { $or: [{ a: { $lt: 25 } }, { a: { $gte: 75 } }] } ).count();
   var expRestRecsCnt = 50;
   if( parseInt( tolRecsCnt ) !== parseInt( restRecsCnt ) || parseInt( restRecsCnt ) !== expRestRecsCnt )
   {
      throw buildException( "checkResult", null, "[compare count of the rest records]",
         "[tolRecsCnt:" + expRestRecsCnt + ",restRecsCnt:" + expRestRecsCnt + "]",
         "[tolRecsCnt:" + parseInt( tolRecsCnt ) + ",restRecsCnt:" + parseInt( restRecsCnt ) + "]" );
   }

   //compare count of records
   for( i = 0; i < 25; i++ )
   {
      var recsCnt = subCL1.find( { a: i } ).count();
      var expectCnt = 1;
      if( parseInt( recsCnt ) !== expectCnt )
      {
         throw buildException( "checkResult", null, "[compare count of records in the " + subCLName1 + "]",
            "[recsCnt:" + expectCnt + "]",
            "[recsCnt:" + parseInt( recsCnt ) + "]" );
      }
   }
   for( i = 75; i < 100; i++ )
   {
      var recsCnt = subCL2.find( { a: i } ).count();
      var expectCnt = 1;
      if( parseInt( recsCnt ) !== expectCnt )
      {
         throw buildException( "checkResult", null, "[compare count of records in the " + subCLName1 + "]",
            "[recsCnt:" + expectCnt + "]",
            "[recsCnt:" + parseInt( recsCnt ) + "]" );
      }
   }
}