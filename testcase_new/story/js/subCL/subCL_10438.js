/************************************************
 * @Description:
 * seqDB-10437:attach子表时LowBound、UpBound使用不同类型的值，插入单条数据 
 * @Author:linsuqiang
 * @Date:2016-11-25
 * **********************************************/

main( test );

function test ()
{
   //type order: Undefined < NumberDouble = NumberInt < String < Date

   if( commIsStandalone( db ) )
   {
      return;
   }

   //splitting need two groups at least
   allGroupName = getGroupName( db );
   if( 1 === allGroupName.length )
   {
      return;
   }

   var csName = COMMCSNAME;
   var mainCLName = COMMCLNAME + "_mcl";

   // unset variable
   commDropCL( db, csName, mainCLName, true, true, "Fail to drop CL in the beginning" );

   // create mainCL and subCL
   db.setSessionAttr( { PreferedInstance: "M" } );
   var mainCL = createMainCL( csName, mainCLName );
   var subCLName = COMMCLNAME + "_scl";
   var lowBound = 2000.11; // NumberDouble
   var upBound = "aaaaa"; // String
   createSubCL( csName, subCLName );
   attachCL( csName, mainCL, subCLName, lowBound, upBound );
   var startCondition = { Partition: 2048 };
   ClSplitOneTimes( csName, subCLName, startCondition, null );

   var inBoundRecs = [{ MCLKEY: 2022.63 },    // same type as lowBound
   { MCLKEY: 12345 },        // different type in bound
   { MCLKEY: "aaaa" }];   // same type as upBound
   insertValidRecs( mainCL, inBoundRecs );

   var outBoundRecs = [{ MCLKEY: undefined },     // different type out of lowBound
   { MCLKEY: 1998.69 },       // same type as lowBound
   { MCLKEY: "aaaaaaa" },     // same type as upBound
   { MCLKEY: { $date: "2008-01-01" } }]; // different type out of upBound
   insertInvalidRecs( mainCL, outBoundRecs );

   var validRecs = inBoundRecs;
   checkResult( mainCL, validRecs );

   // unset variable
   commDropCL( db, csName, mainCLName, true, true, "Fail to drop CL in the end" );
}

function createMainCL ( csName, mainCLName )
{

   var options = { ShardingKey: { MCLKEY: 1 }, ShardingType: "range", IsMainCL: true };

   mainCL = commCreateCL( db, csName, mainCLName, options, false, true );
   return mainCL;
}

function createSubCL ( csName, subCLName )
{

   var options = { ShardingKey: { SCLKEY: 1 }, ShardingType: "hash" };

   subCL = commCreateCL( db, csName, subCLName, options, false, true );
   return subCL;
}

function attachCL ( csName, mainCL, subCLName, lowBound, upBound )
{

   var options = { LowBound: { MCLKEY: lowBound }, UpBound: { MCLKEY: upBound } };

   mainCL.attachCL( csName + "." + subCLName, options );
}

function checkResult ( mainCL, validRecs )
{
   try 
   {
      var rc = mainCL.find().sort( { _id: 1 } );
   }
   catch( e ) 
   {
      if( e.message == SDB_DMS_CS_NOTEXIST || e.message == SDB_DMS_NOTEXIST )
      {
         // to prevent slave nodes are not ready!
         sleep( 5000 );
         var rc = mainCL.find().sort( { _id: 1 } );
      }
   }
   lsqCheckRec( rc, validRecs );
}
