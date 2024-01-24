/* *****************************************************************************
@discretion: rename miancl and subcl,than insert/update/find/remove datas
             test a :rename maincl ,than insert/update/find/remove datas
             test b :rename subcl ,than insert/update/find/remove datas            
@author��2018-10-16 wuyan  Init
***************************************************************************** */

main( test );
function test ()
{
   if( true == commIsStandalone( db ) )
   {
      return;
   }

   var mainCLName = CHANGEDPREFIX + "_rename_maincl16142";
   var newMainCLName = CHANGEDPREFIX + "_newrename_maincl16142";
   var csName = CHANGEDPREFIX + "_cs16142";
   var subCLName1 = CHANGEDPREFIX + "_subcl16142a";
   var subCLName2 = CHANGEDPREFIX + "_subcl16142b";
   var newSubCLName1 = CHANGEDPREFIX + "_newsubcl16142a";
   var newSubCLName2 = CHANGEDPREFIX + "_newsubcl16142b";
   commDropCS( db, csName, true, "Failed to drop CS." );
   commDropCL( db, COMMCSNAME, subCLName2, true, true, "clear collection in the beginning" );
   commDropCL( db, COMMCSNAME, newSubCLName2, true, true, "clear collection in the beginning" );
   commCreateCS( db, csName, false, "Failed to create CS." );
   createCLAndAttachCL( csName, mainCLName, COMMCSNAME, subCLName1, subCLName2 );

   db.getCS( csName ).renameCL( mainCLName, newMainCLName );
   dataOperation( csName, newMainCLName );

   db.getCS( csName ).renameCL( subCLName1, newSubCLName1 );
   dataOperation( csName, newMainCLName );

   db.getCS( COMMCSNAME ).renameCL( subCLName2, newSubCLName2 );
   dataOperation( csName, newMainCLName );

   commDropCS( db, csName, true, "clear cs in the ending" );
   commDropCL( db, COMMCSNAME, newSubCLName2, true, true, "clear collection in the ending" );
}

function createCLAndAttachCL ( maincsName, mainCLName, subcsName, subCLName1, subCLName2 )
{
   var mainclShardingKey = { a: 1 };
   var dbmaincl = createMainCL( maincsName, mainCLName, mainclShardingKey );

   var shardingKey = { no: 1 };
   var clOptions1 = { ShardingKey: { no: 1 }, ShardingType: "range", ReplSize: 0 };
   commCreateCL( db, maincsName, subCLName1, clOptions1, false, true, "Failed to create cl." );
   var clOptions2 = { ShardingKey: { no: 1 }, ShardingType: "hash", ReplSize: 0 };
   commCreateCL( db, subcsName, subCLName2, clOptions2, false, true, "Failed to create cl." );

   var options1 = { LowBound: { "a": 0 }, UpBound: { "a": 1000 } };
   dbmaincl.attachCL( maincsName + "." + subCLName1, options1 );

   var options2 = { LowBound: { "a": 1000 }, UpBound: { "a": 2000 } };
   dbmaincl.attachCL( subcsName + "." + subCLName2, options2 );
}

function dataOperation ( maincsName, mainCLName )
{
   var dbcl = db.getCS( maincsName ).getCL( mainCLName );
   var recordNums = 2000;
   insertData( dbcl, recordNums );
   updateDataAndCheckResult( dbcl, recordNums );
   removeDataAndCheckResult( dbcl, recordNums );
   truncateCLAndCheckResult( dbcl );
}


function updateDataAndCheckResult ( dbcl, expRecordNums )
{
   dbcl.update( { $set: { "user": "testupdate16142" } } );
   var count = dbcl.find( { "user": "testupdate16142" } ).count();
   assert.equal( count, expRecordNums );
}

function removeDataAndCheckResult ( dbcl, recordNums )
{
   var removeNums = 1100;
   dbcl.remove( { a: { $gte: ( recordNums - removeNums ) } } );
   var removeDataNumsInCL = dbcl.count( { a: { $gte: ( recordNums - removeNums ) } } );
   var expRemoveNumsInCL = 0;
   assert.equal( removeDataNumsInCL, expRemoveNumsInCL );

   var count = dbcl.count( { a: { $lt: removeNums } } );
   var expRecordNums = recordNums - removeNums;
   assert.equal( count, expRecordNums );
}

function truncateCLAndCheckResult ( dbcl )
{
   dbcl.truncate();

   //check the records nums is 0
   var count = dbcl.count();
   assert.equal( count, 0 );
}