/* *****************************************************************************
@discretion: rename miancl and subcl,test three cases:
             test a :subcls on different groups
             test b: subcls on same group
             test c: subcl has been split
@author��2018-10-15 wuyan  Init
***************************************************************************** */


main( test );
function test ()
{
   if( true == commIsStandalone( db ) )
   {
      return;
   }

   //split at least two groups
   var allGroupName = getGroupName( db, true );
   if( 1 === allGroupName.length )
   {
      return;
   }

   var mainCLName = CHANGEDPREFIX + "_rename_maincl16061";
   var newMainCLName = CHANGEDPREFIX + "_newrename_maincl16061";
   var csName = CHANGEDPREFIX + "_cs16061";
   var subCLName1 = CHANGEDPREFIX + "_subcl16061a";
   var subCLName2 = CHANGEDPREFIX + "_subcl16061b";
   var subCLName3 = CHANGEDPREFIX + "_subcl16061c";
   var newSubCLName1 = CHANGEDPREFIX + "_newsubcl16061a";
   var newSubCLName2 = CHANGEDPREFIX + "_newsubcl16061b";
   var newSubCLName3 = CHANGEDPREFIX + "_newsubcl16061c";
   commDropCS( db, csName, true, "Failed to drop CS." );
   commDropCL( db, COMMCSNAME, subCLName2, true, true, "clear collection in the beginning" );
   commDropCL( db, COMMCSNAME, newSubCLName2, true, true, "clear collection in the beginning" );

   commCreateCS( db, csName, false, "Failed to create CS." );
   var groupName1 = allGroupName[0][0];
   var groupName2 = allGroupName[1][0];
   createCLAndSplitCL( csName, mainCLName, COMMCSNAME, subCLName1, subCLName2, subCLName3, groupName1, groupName2 );

   db.getCS( csName ).renameCL( mainCLName, newMainCLName );

   //test a: rename mainCL with subcls, maincl and subcl in different cs
   testRenameSubCLInDiffGroups( csName, newMainCLName, COMMCSNAME, subCLName1, newSubCLName1, subCLName2, newSubCLName2 );

   //test b: rename mainCL with subcls, maincl and subcl in same cs   
   testRenameSubCLInSameGroups( csName, newMainCLName, csName, subCLName3, newSubCLName3 );

   commDropCS( db, csName, true, "clear cs in the ending" );
   commDropCL( db, COMMCSNAME, subCLName2, true, true, "clear collection in the ending" );
   commDropCL( db, COMMCSNAME, newSubCLName2, true, true, "clear collection in the ending" );
}

function createCLAndSplitCL ( maincsName, mainCLName, subcsName, subCLName1, subCLName2, subCLName3, groupName1, groupName2 )
{
   var mainclShardingKey = { a: 1 };
   var dbmaincl = createMainCL( maincsName, mainCLName, mainclShardingKey );

   var shardingKey = { no: 1 };
   var clOptions1 = { ShardingKey: { no: 1 }, ShardingType: "range", ReplSize: 0, Group: groupName1 };
   commCreateCL( db, maincsName, subCLName1, clOptions1, false, true, "Failed to create cl." );
   var clOptions2 = { ShardingKey: { no: 1 }, ShardingType: "hash", ReplSize: 0, Group: groupName2 };
   commCreateCL( db, subcsName, subCLName2, clOptions2, false, true, "Failed to create cl." );
   var clOptions3 = { ShardingKey: { no: 1 }, ShardingType: "hash", ReplSize: 0, Group: groupName1 };
   var dbcl = commCreateCL( db, maincsName, subCLName3, clOptions3, true, true, "Failed to create cl." );

   var options1 = { LowBound: { "a": 0 }, UpBound: { "a": 1000 } };
   dbmaincl.attachCL( maincsName + "." + subCLName1, options1 );

   var options2 = { LowBound: { "a": 1000 }, UpBound: { "a": 2000 } };
   dbmaincl.attachCL( subcsName + "." + subCLName2, options2 );

   var options3 = { LowBound: { "a": 2000 }, UpBound: { "a": 3000 } };
   dbmaincl.attachCL( maincsName + "." + subCLName3, options3 );

   dbcl.split( groupName1, groupName2, 50 );
}

function testRenameSubCLInDiffGroups ( maincsName, mainCLName, subcsName, oldSubCLName1, newSubCLName1, oldSubCLName2, newSubCLName2 )
{
   db.getCS( maincsName ).renameCL( oldSubCLName1, newSubCLName1 );
   db.getCS( subcsName ).renameCL( oldSubCLName2, newSubCLName2 );

   checkRenameSubCLResult( maincsName, mainCLName, maincsName, oldSubCLName1, newSubCLName1 );
   checkRenameSubCLResult( maincsName, mainCLName, subcsName, oldSubCLName2, newSubCLName2 );
}

function testRenameSubCLInSameGroups ( maincsName, mainCLName, subcsName, oldSubCLName, newSubCLName )
{
   db.getCS( maincsName ).renameCL( oldSubCLName, newSubCLName );
   checkRenameSubCLResult( maincsName, mainCLName, maincsName, oldSubCLName, newSubCLName );
}