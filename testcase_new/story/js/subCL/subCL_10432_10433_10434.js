/************************************************
 * @Description:
 * seqDB-10432:包含部分主表分区键，插入单条数据 
 * seqDB-10433:包含部分子表分区键，插入单条数据 
 * seqDB-10434:attach子表分区键字段使用不同的类型，插入单条数据 
 * @Author:linsuqiang
 * @Date:2016-11-24
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
   var lSubCLName = COMMCLNAME + "_scl_l";
   var lLowBounds = [-1000, 0, 250];
   var lUpBounds = [0, 100, 600];
   createSubCL( csName, lSubCLName );
   attachCL( csName, mainCL, lSubCLName, lLowBounds, lUpBounds );
   var startCondition = { Partition: 2048 };
   ClSplitOneTimes( csName, lSubCLName, startCondition, null );

   // right subCL
   var rSubCLName = COMMCLNAME + "_scl_r";
   var rLowBounds = [1000, 200, 800];
   var rUpBounds = [2000, 300, 1500];
   createSubCL( csName, rSubCLName );
   attachCL( csName, mainCL, rSubCLName, rLowBounds, rUpBounds );
   var startCondition = { Partition: 2048 };
   ClSplitOneTimes( csName, rSubCLName, startCondition, null );

   // cover seqDB-10429
   var inBoundRecs = [// just in low bound of left subCL
      {
         MCLKEY_0: lLowBounds[0],
         MCLKEY_1: lLowBounds[1],
         MCLKEY_2: lLowBounds[2]
      },
      // just in up bound of left subCL
      {
         MCLKEY_0: lUpBounds[0],
         MCLKEY_1: lUpBounds[1],
         MCLKEY_2: lUpBounds[2] - 1
      },
      // just in low bound of right subCL
      {
         MCLKEY_0: rLowBounds[0],
         MCLKEY_1: rLowBounds[1],
         MCLKEY_2: rLowBounds[2]
      },
      // just in up bound of right subCL
      {
         MCLKEY_0: rUpBounds[0],
         MCLKEY_1: rUpBounds[1],
         MCLKEY_2: rUpBounds[2] - 1
      },
      // a random and valid value
      {
         MCLKEY_0: 1500,
         MCLKEY_1: 200,
         MCLKEY_2: 260
      }];
   insertRecs( mainCL, inBoundRecs, true, "in bound" );

   var outBoundRecs = [// just out of low bound of left subCL
      {
         MCLKEY_0: lLowBounds[0],
         MCLKEY_1: lLowBounds[1],
         MCLKEY_2: lLowBounds[2] - 1
      },
      // just out of up bound of left subCL
      {
         MCLKEY_0: lUpBounds[0],
         MCLKEY_1: lUpBounds[1],
         MCLKEY_2: lUpBounds[2]
      },
      // just out of low bound of right subCL
      {
         MCLKEY_0: rLowBounds[0],
         MCLKEY_1: rLowBounds[1],
         MCLKEY_2: rLowBounds[2] - 1
      },
      // just out of up bound of right subCL
      {
         MCLKEY_0: rUpBounds[0],
         MCLKEY_1: rUpBounds[1],
         MCLKEY_2: rUpBounds[2]
      },
      // a random and invalid value
      {
         MCLKEY_0: -1500,
         MCLKEY_1: 500,
         MCLKEY_2: 260
      }];
   insertRecs( mainCL, outBoundRecs, false, "out of bound" );

   // cover seqDB-10430
   var subKeyRecs = [{ SCLKEY: "hijkaa" },  // value is designed casually
   { SCLKEY: "iowelqza" }]; // value is designed casually
   insertRecs( mainCL, subKeyRecs, false, "with only subCL keys" );

   // cover seqDB-10431
   var noShardKeyRecs = [{ "llbecsd": 100 }]; // the key and value is designed casually
   insertRecs( mainCL, noShardKeyRecs, false, "with no any sharding key" );

   // cover seqDB-10434
   // insert sharding keys of both mainCL and subCL
   var bothKeyRecs = [ // a random and valid value in lSubCL
      {
         MCLKEY_0: -500,
         MCLKEY_1: 50,
         MCLKEY_2: 300,
         SCLKEY: "ui792"
      },
      // a random and valid value in rSubCL
      {
         MCLKEY_0: 1500,
         MCLKEY_1: 250,
         MCLKEY_2: 1300,
         SCLKEY: "ui792"
      }];

   insertRecs( mainCL, bothKeyRecs, true, "with both mainCL and subCL keys" );

   var validRecs = [];
   validRecs = inBoundRecs.concat( bothKeyRecs );
   checkResult( mainCL, validRecs );

   // unset variable
   commDropCL( db, csName, mainCLName, true, true, "Fail to drop CL in the end" );
}

function createMainCL ( csName, mainCLName )
{
   var options = {
      ShardingKey: {
         MCLKEY_0: 1,
         MCLKEY_1: 1,
         MCLKEY_2: 1
      },
      ShardingType: "range",
      IsMainCL: true
   };
   var mainCL = commCreateCL( db, csName, mainCLName, options, false, true, "Failed to create mainCL." );
   return mainCL;
}

function createSubCL ( csName, subCLName )
{
   var options = { ShardingKey: { SCLKEY: 1 }, ShardingType: "hash" };
   subCL = commCreateCL( db, csName, subCLName, options, false, true, "Failed to create subCL" );
   return subCL;
}

function attachCL ( csName, mainCL, subCLName, lowBounds, upBounds )
{
   var options = {
      LowBound: {
         MCLKEY_0: lowBounds[0],
         MCLKEY_1: lowBounds[1],
         MCLKEY_2: lowBounds[2]
      },

      UpBound: {
         MCLKEY_0: upBounds[0],
         MCLKEY_1: upBounds[1],
         MCLKEY_2: upBounds[2]
      }
   };
   mainCL.attachCL( csName + "." + subCLName, options );
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
   var rc = mainCL.find().sort( { _id: 1 } );
   lsqCheckRec( rc, validRecs );
}
