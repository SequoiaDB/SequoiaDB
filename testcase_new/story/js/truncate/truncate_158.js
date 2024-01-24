/******************************************************************************
*@Description: 测试垂直分区表写入大于多个数据页的普通记录，然后再truncate*                        
*@Modify list:
*              2015-5-13  xiaojun Hu   Init
*              2019-05-27 wuyan        modify
******************************************************************************/

main( test );


function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var mainCSName = CHANGEDPREFIX + "_largeThanPage_mainCS158";
   var mainCLName = CHANGEDPREFIX + "_maincl158";
   var subCLName1 = CHANGEDPREFIX + "_subcl158a";
   var subCLName2 = CHANGEDPREFIX + "_subcl158b";
   var subCLFullName1 = mainCSName + "." + subCLName1;
   var subCLFullName2 = mainCSName + "." + subCLName2;
   commDropCS( db, mainCSName, true, "drop main cs begin" );

   var pageSize = 4096;
   var mainCL = createCLAndAttachCL( mainCSName, mainCLName, subCLName1, subCLName2, pageSize );

   var recordSize = pageSize * 2;
   var recordNum = 5;
   insertDataAndCheckDataNum( mainCL, mainCSName, subCLName1, subCLName2, recordNum, recordSize );

   truncateAndCheckResult( mainCL, subCLFullName1, subCLFullName2 );
   commDropCS( db, mainCSName, false, "drop main cs end" );
}

function createCLAndAttachCL ( mainCSName, mainCLName, subCLName1, subCLName2, pageSize )
{
   commCreateCS( db, mainCSName, true, "", { PageSize: pageSize } );
   var clOption = {
      "ShardingKey": { "ID_Default": 1 }, "ShardingType": "range", "IsMainCL": true
   };
   var mainCL = commCreateCL( db, mainCSName, mainCLName, clOption, true, false, "create collection begin" );
   commCreateCL( db, mainCSName, subCLName1, {}, true, false, "create sub CL1 begin" );
   commCreateCL( db, mainCSName, subCLName2, {}, true, false, "create sub CL2 begin" );
   mainCL.attachCL( mainCSName + "." + subCLName1, { "LowBound": { "ID_Default": 0 }, "UpBound": { "ID_Default": 3 } } );
   mainCL.attachCL( mainCSName + "." + subCLName2, { "LowBound": { "ID_Default": 3 }, "UpBound": { "ID_Default": 5 } } );
   return mainCL;
}

function insertDataAndCheckDataNum ( mainCL, mainCSName, subCLName1, subCLName2, recordNum, recordSize )
{
   truncateInsertRecord( mainCL, recordNum, recordSize );
   var subCL1 = db.getCS( mainCSName ).getCL( subCLName1 );
   var subCL2 = db.getCS( mainCSName ).getCL( subCLName2 );
   var count1 = subCL1.count();
   var count2 = subCL2.count();
   var AllCount = mainCL.count();
   var expCount1 = 3;
   var expCount2 = 2;
   if( Number( recordNum ) !== Number( AllCount ) || Number( expCount1 ) !== Number( count1 )
      || Number( expCount2 ) !== Number( count2 ) )
   {
      throw new Error( "recordNum: " + recordNum + "\nAllCount: " + AllCount + "\nexpCount1: " + expCount1 + "\ncount1: " + count1 + "\nexpCount2: " + expCount2 + "\ncount2: " + count2 );
   }

}

function truncateAndCheckResult ( mainCL, subCLFullName1, subCLFullName2 )
{
   mainCL.truncate();
   truncateVerify( db, subCLFullName1 );
   truncateVerify( db, subCLFullName2 );

   var expCount = 0;
   var count = mainCL.count();
   assert.equal( expCount, count );
}
