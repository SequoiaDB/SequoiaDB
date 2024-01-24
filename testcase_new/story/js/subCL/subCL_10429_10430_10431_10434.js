/************************************************
 * @Description: 
 * seqDB-10429: 数据包含主表分区键，插入单条数据 
 * seqDB-10430: 数据包含子表分区键，插入单条数据 
 * seqDB-10431: 数据不包含主、子表分区键，插入单条数据 
 * seqDB-10434: attach子表分区键字段使用不同的类型，插入单条数据 
 * @Author: linsuqiang
 * @Date: 2016-11-24
 * **********************************************/

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
   // left subCL
   var lSubCL = {
      name: COMMCLNAME + "_scl_l",
      lowBound: { $date: "2000-01-01" },
      upBound: { $date: "2001-01-01" }
   }

   // unset variable
   createSubCL( csName, lSubCL );
   attachCL( csName, mainCL, lSubCL );
   var startCondition = { Partition: 2048 };
   ClSplitOneTimes( csName, lSubCL.name, startCondition, null );

   // right subCL
   var rSubCL = {
      name: COMMCLNAME + "_scl_r",
      lowBound: { $date: "2002-01-01" },
      upBound: { $date: "2003-01-01" }
   }
   createSubCL( csName, rSubCL );
   attachCL( csName, mainCL, rSubCL );
   var startCondition = { Partition: 2048 };
   ClSplitOneTimes( csName, rSubCL.name, startCondition, null );

   // cover seqDB-10429
   var inBoundRecs = [{ MCLKEY: lSubCL.lowBound },
   { MCLKEY: { $date: "2000-12-31" } },   // lSubCL.upBound - 1
   { MCLKEY: rSubCL.lowBound },
   { MCLKEY: { $date: "2002-12-31" } },   // rSubCL.upBound -1
   { MCLKEY: { $date: "2000-03-09" } }];  // a random and valid value
   insertRecs( mainCL, inBoundRecs, true, "in bound" );

   var outBoundRecs = [{ MCLKEY: { $date: "1999-12-31" } }, // lSubCL.lowBound - 1
   { MCLKEY: lSubCL.upBound },
   { MCLKEY: { $date: "2001-06-06" } },   // between lSubCL.upBound ~ rSubCL.lowBound
   { MCLKEY: { $date: "2001-12-31" } },   // rSubCL.lowBound -1
   { MCLKEY: rSubCL.upBound },
   { MCLKEY: { $date: "2016-03-09" } }];  // a random but invalid value
   insertRecs( mainCL, outBoundRecs, false, "out of bound" );

   // cover seqDB-10430
   var subKeyRecs = [{ SCLKEY: 100 },  // value is designed casually
   { SCLKEY: 150 }]; // value is designed casually
   insertRecs( mainCL, subKeyRecs, false, "with only subCL keys" );

   // cover seqDB-10431
   var noShardKeyRecs = [{ "llbecsd": 100 }]; // the key and value is designed casually
   insertRecs( mainCL, noShardKeyRecs, false, "with no any sharding key" );

   // cover seqDB-10434
   var bothKeyRecs = [{ MCLKEY: { $date: "2000-06-06" }, SCLKEY: 100 }, // a random and valid value in lSubCL
   { MCLKEY: { $date: "2002-09-09" }, SCLKEY: 150 }]; // a random and valid value in rSubCL
   insertRecs( mainCL, bothKeyRecs, true, "with both mainCL keys and subCL keys" );

   var validRecs = [];
   validRecs = inBoundRecs.concat( bothKeyRecs );
   checkResult( mainCL, validRecs );

   // unset variable
   commDropCL( db, csName, mainCLName, true, true, "Fail to drop CL in the end" );
}

function createMainCL ( csName, mainCLName )
{
   var options = { ShardingKey: { MCLKEY: 1 }, ShardingType: "range", IsMainCL: true };
   var mainCL = commCreateCL( db, csName, mainCLName, options, false, true, "Failed to create mainCL." );
   return mainCL;
}

function createSubCL ( csName, subCL )
{
   var options = { ShardingKey: { SCLKEY: 1 }, ShardingType: "hash" };
   subCL.CL = commCreateCL( db, csName, subCL.name, options, false, true, "Failed to create subCL." );
   return;
}

function attachCL ( csName, mainCL, subCL )
{
   var options = {
      LowBound: { MCLKEY: subCL.lowBound },
      UpBound: { MCLKEY: subCL.upBound }
   };

   mainCL.attachCL( csName + "." + subCL.name, options );
}

function insertRecs ( mainCL, recs, isValid, msg )
{
   if( isValid )
   {
      insertValidRecs( mainCL, recs );
   }
   else
   {
      insertInvalidRecs( mainCL, recs );
   }
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
