/************************************************************************
*@Description:  		seqDB-826:主表跟子表不在同一个CS,删除主表所在CS,分离子表_SD.subCL.01.013
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

      var mainCSName = COMMCLNAME + "_mcs";
      var subCSName = COMMCLNAME + "_scs";
      var mainCLName = COMMCLNAME + "_mcl";
      var subCLName = COMMCLNAME + "_scl";

      println( "\n---Begin to create cs in the pre-condition." );
      commDropCS( db, subCSName, true, "Failed to drop subCS." );
      commDropCS( db, mainCSName, true, "Failed to drop mainCS." );
      commCreateCS( db, mainCSName, false, "Failed to create cs." );
      commCreateCS( db, subCSName, false, "Failed to create cs." );

      var mainCL = createMainCL( mainCSName, mainCLName );
      var subCL = createSubCL( subCSName, subCLName );
      attachCL( mainCL, subCSName, subCLName );

      var insertRecsNum = 10;
      insertRecs( mainCL, insertRecsNum );

      commDropCS( db, mainCSName, false, "Failed to drop cs." );
      checkResult( mainCSName, mainCLName, subCSName, subCLName, insertRecsNum );

      println( "\n---Begin to drop subCS in the end-condition." );
      commDropCS( db, subCSName, false, "Failed to drop subCS." );
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

function attachCL ( mainCL, csName, subCLName )
{
   println( "\n---Begin to attach CL." );

   var options = { LowBound: { "a": 0 }, UpBound: { "a": 100 } };
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

function checkResult ( mainCSName, mainCLName, subCSName, subCLName, insertRecsNum )
{
   println( "\n---Begin to check result." );

   //check mainCS does not exist
   try
   {
      eval( 'db.getCS("' + mainCSName + '")' )
   }
   catch( e )
   {
      if( e !== -34 )
      {
         throw buildException( "checkResult", null, "[compare mainCL whether exist]",
            "[e:-34]", "[e:" + e + "]" );
      }
   }

   //check subCL exist
   try
   {
      eval( 'db.' + subCSName + '.getCL("' + subCLName + '")' )
   }
   catch( e )
   {
      if( e === -23 )
      {
         throw buildException( "checkResult", null, "[compare subCL whether exist]",
            "[e:-23]", "[e:" + e + "]" );
      }
   }

   //compare records of subCL
   sleep( 5 );
   var subCLRecsCnt = db.getCS( subCSName ).getCL( subCLName ).count();
   if( parseInt( subCLRecsCnt ) !== insertRecsNum )
   {
      throw buildException( "checkResult", null, "[compare records of subCL]",
         "[subCLRecsCnt:" + insertRecsNum + "]",
         "[subCLRecsCnt:" + parseInt( subCLRecsCnt ) + "]" );
   }
}