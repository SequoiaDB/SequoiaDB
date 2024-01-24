/************************************************************************
*@Description:   seqDB-825:主表跟子表属于同一个CS,删除主表_SD.subCL.01.012
*           主表跟子表在同一CS，删除主表时会同时将挂载在主表的子表也一并删除。
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

      var domainName = COMMCLNAME + "_domain";
      var csName = COMMCSNAME;
      var mainCLName = COMMCLNAME + "_mcl";
      var subCLName = COMMCLNAME + "_scl";

      println( "\n---Begin to drop cl in the pre-condition." );
      commDropCL( db, csName, subCLName, true, true, "Failed to drop mainCL." );
      commDropCL( db, csName, mainCLName, true, true, "Failed to drop subCL." );

      var mainCL = createMainCL( csName, mainCLName );
      createSubCL( csName, subCLName );
      attachCL( csName, mainCL, subCLName );

      var insertRecsNum = 10;
      insertRecs( mainCL, insertRecsNum );

      println( "\n---Begin to drop mainCL." );
      commDropCL( db, csName, mainCLName, true, false, "Failed to drop mainCL." );
      checkResult( csName, mainCLName, subCLName );
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

function insertRecs ( mainCL, insertRecsNum )
{
   println( "\n---Begin to insert records." );

   for( i = 0; i < insertRecsNum; i++ )
   {
      mainCL.insert( { a: i, b: i } );
   }
}

function checkResult ( csName, mainCLName, subCLName )
{
   println( "\n---Begin to check result." );

   //check mainCL does not exist
   try
   {
      eval( 'db.' + csName + '.getCL("' + mainCLName + '")' )
   }
   catch( e )
   {
      if( e !== -23 )
      {
         throw buildException( "checkResult", null, "[compare mainCL whether exist]",
            "[e:-23]", "[e:" + e + "]" );
      }
   }

   //check subCL exist
   try
   {
      eval( 'db.' + csName + '.getCL("' + subCLName + '")' )
   }
   catch( e )
   {
      if( e !== -23 )
      {
         throw buildException( "checkResult", null, "[compare subCL whether exist]",
            "[e:-23]", "[e:" + e + "]" );
      }
   }
}