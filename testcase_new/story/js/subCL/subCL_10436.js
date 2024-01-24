/************************************************
 * @Description:
 * seqDB-10436:子表为分区表且为逆序，插入单条数据 
 * @Author:linsuqiang
 * @Date:2016-11-25
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
   var mclKeyPM = [1, 1, 1];

   // unset variable
   commDropCL( db, csName, mainCLName, true, true, "Fail to drop CL in the beginning" );

   // create mainCL and subCL
   db.setSessionAttr( { PreferedInstance: "M" } );
   var mainCL = createMainCL( csName, mainCLName, mclKeyPM );
   var subCLName = COMMCLNAME + "_scl";
   var sclKeyPM = [-1, -1, -1];
   var lowBounds = [0, 100, 600];
   var upBounds = [100, 200, 800];
   createSubCL( csName, subCLName, sclKeyPM );
   attachCL( csName, mainCL, subCLName, lowBounds, upBounds );
   var startCondition = { Partition: 2048 };
   ClSplitOneTimes( csName, subCLName, startCondition, null );

   // all values below are in bound
   var allMclKeysRecs = [{
      MCLKEY_0: 50,
      MCLKEY_1: 150,
      MCLKEY_2: 700
   }];
   insertRecs( mainCL, allMclKeysRecs, true, "all mainCL keys" );

   var allSclKeysRecs = [{
      MCLKEY_0: 50,
      MCLKEY_1: 150,
      MCLKEY_2: 700,
      SCLKEY_0: 357,
      SCLKEY_1: 12,
      SCLKEY_2: 5637
   }];
   insertRecs( mainCL, allSclKeysRecs, true, "all mainCL and subCL keys" );

   var partMclKeysRecs = [// a valid record
      {
         MCLKEY_0: 50,
         MCLKEY_2: 700
      },
      // a invalid record
      {
         MCLKEY_1: 150,
         MCLKEY_2: 700,
         SCLKEY_0: 357,
         SCLKEY_1: 12,
      }];
   insertRecs( mainCL, partMclKeysRecs[0], true, "part of mainCL keys" );
   insertRecs( mainCL, partMclKeysRecs[1], true, "part of mainCL keys" );

   var partSclKeysRecs = [{
      MCLKEY_0: 50,
      MCLKEY_1: 150,
      MCLKEY_2: 700,
      SCLKEY_0: 357,
      SCLKEY_1: 12,
   }];
   insertRecs( mainCL, partSclKeysRecs, true, "all mainCL keys and part of subCL keys" );

   var onlySclKeysRecs = [{
      SCLKEY_0: 357,
      SCLKEY_1: 12,
      SCLKEY_2: 689
   }];
   insertRecs( mainCL, onlySclKeysRecs, false, "only subCL keys" );

   var noShardKeyRecs = [{
      "llbecsd": 100,
      "jasdf8ow": 559
   }]; // the key is designed casually
   insertRecs( mainCL, noShardKeyRecs, false, "no any sharding key" );

   var validRecs = [];
   validRecs = allMclKeysRecs.concat( allSclKeysRecs ).concat( partSclKeysRecs );
   checkResult( mainCL, validRecs );

   // unset variable
   commDropCL( db, csName, mainCLName, true, true, "Fail to drop CL in the end" );
}

function createMainCL ( csName, mainCLName, mclKeyPM )
{
   var options = {
      ShardingKey: {
         MCLKEY_0: mclKeyPM[0],
         MCLKEY_1: mclKeyPM[1],
         MCLKEY_2: mclKeyPM[2]
      },
      ShardingType: "range",
      IsMainCL: true
   };

   mainCL = commCreateCL( db, csName, mainCLName, options, false, true );
   return mainCL;
}

function createSubCL ( csName, subCLName, sclKeyPM )
{
   var options = {
      ShardingKey: {
         SCLKEY_0: sclKeyPM[0],
         SCLKEY_1: sclKeyPM[1],
         SCLKEY_2: sclKeyPM[2]
      },
      ShardingType: "hash"
   };

   subCL = commCreateCL( db, csName, subCLName, options, false, true );
   return subCL;
}

function attachCL ( csName, mainCL, subCLName, lowBounds, upBounds )
{
   var options = {
      LowBound: {
         MCLKEY_0: lowBounds[0],
         MCLKEY_1: lowBounds[1],
         MCLKEY_2: lowBounds[2],
      },
      UpBound: {
         MCLKEY_0: upBounds[0],
         MCLKEY_1: upBounds[1],
         MCLKEY_2: upBounds[2],
      }
   };

   mainCL.attachCL( csName + "." + subCLName, options );
}

function insertRecs ( mainCL, recs, isValid, keyMsg )
{
   if( isValid ) 
   {
      insertValidRecs( mainCL, recs );
   }
   else
   {
      insertInvalidRecs( mainCL, recs )
   }
}

function checkResult ( mainCL, validRecs )
{
   var rc = mainCL.find().sort( { _id: 1 } );
   lsqCheckRec( rc, validRecs );
}
