/******************************************************
 * @Description: seqDB-10443:部分数据分区范围内、部分数据分区范围外，批量插入数据 
 * @Author: linsuqiang 
 * @Date: 2016-11-29
 ******************************************************/

main( test );

function test ()
{
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
   var lowBound = -3000;
   var upBound = 5000;
   createSubCL( csName, subCLName );
   attachCL( csName, mainCL, subCLName, lowBound, upBound );
   var startCondition = { Partition: 2048 };
   ClSplitOneTimes( csName, subCLName, startCondition, null );

   // test bulk inserting random records
   // values of randomRecs are not all valid 
   var randomRecs = [{ MCLKEY: 100 },
   { MCLKEY: 0 },
   { MCLKEY: -998 },
   { MCLKEY: -1555 },
   { MCLKEY: -96325 },
   { MCLKEY: 6589 },
   { MCLKEY: 7000 },
   { MCLKEY: 20000 },
   { MCLKEY: 555 },
   { MCLKEY: 4599 }];
   bulkinsertRandomRecs( mainCL, randomRecs );

   // test bulk inserting valid records
   var validRecs = [{ MCLKEY: 100 },
   { MCLKEY: 0 },
   { MCLKEY: 859 },
   { MCLKEY: 758 },
   { MCLKEY: 550 },
   { MCLKEY: 336 },
   { MCLKEY: -998 },
   { MCLKEY: -1555 },
   { MCLKEY: 555 },
   { MCLKEY: 4599 }];
   bulkinsertValidRecs( mainCL, validRecs );

   checkResult( mainCL, validRecs );

   // unset variable
   commDropCL( db, csName, mainCLName, true, true, "Fail to drop CL in the end" );
}

function createMainCL ( csName, mainCLName )
{

   var options = { ShardingKey: { MCLKEY: 1 }, ShardingType: "range", IsMainCL: true };
   var mainCL = commCreateCL( db, csName, mainCLName, options, false, true );
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

function bulkinsertRandomRecs ( mainCL, recs )
{

   assert.tryThrow( SDB_CAT_NO_MATCH_CATALOG, function()
   {
      mainCL.insert( recs );
   } );
}

function bulkinsertValidRecs ( mainCL, recs )
{

   mainCL.insert( recs );
}

function checkResult ( mainCL, validRecs )
{
   var rc = mainCL.find().sort( { _id: 1 } );
   lsqCheckRec( rc, validRecs );
}