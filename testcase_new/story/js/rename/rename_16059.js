/* *****************************************************************************
@discretion: rename maincl
@author��2018-10-12 wuyan  Init
***************************************************************************** */

main( test );
function test ()
{
   if( true == commIsStandalone( db ) )
   {
      return;
   }

   var mainCLName = CHANGEDPREFIX + "_renamecl16059";
   var newMainCLName1 = CHANGEDPREFIX + "_newcl16059a";
   var newMainCLName2 = CHANGEDPREFIX + "_newcl16059b";
   var newMainCLName3 = CHANGEDPREFIX + "_newcl16059c";
   var csName = CHANGEDPREFIX + "_cs16059";
   var subCLName1 = CHANGEDPREFIX + "_subcl16059a";
   var subCLName2 = CHANGEDPREFIX + "_subcl16059b";
   commDropCS( db, csName, true, "Failed to drop CS." );
   commDropCL( db, COMMCSNAME, subCLName2, true, true, "clear collection in the beginning" );

   commCreateCS( db, csName, false, "Failed to create CS." );
   var mainclShardingKey = { a: 1 };
   var dbcl = createMainCL( csName, mainCLName, mainclShardingKey );
   var shardingKey = { no: 1 };
   createCL( csName, subCLName1, shardingKey );
   createCL( COMMCSNAME, subCLName2, shardingKey );

   //test a: rename mainCL, no attach subcl
   db.getCS( csName ).renameCL( mainCLName, newMainCLName1 );

   //test b: rename mainCL with subcls, use the new mainclName test the result of scene a     
   testRenameMainCLBySameCS( csName, newMainCLName1, newMainCLName2, csName, subCLName1 );

   //test c: rename mainCL with subcls, maincl and subcl in different cs
   testRenameMainCLByDiffCS( csName, newMainCLName2, newMainCLName3, COMMCSNAME, subCLName1, subCLName2 )

   commDropCS( db, csName, true, "clear cs in the ending" );
   commDropCL( db, COMMCSNAME, subCLName2, true, true, "clear collection in the ending" );
}

function testRenameMainCLBySameCS ( maincsName, oldMainCLName, newMainCLName, subcsName, subCLName )
{
   var newMaincl1 = db.getCS( maincsName ).getCL( oldMainCLName );
   var options = { LowBound: { "a": 0 }, UpBound: { "a": 1000 } };
   newMaincl1.attachCL( maincsName + "." + subCLName, options );
   db.getCS( maincsName ).renameCL( oldMainCLName, newMainCLName );
   checkRenameMainCLResult( maincsName, subcsName, newMainCLName, oldMainCLName, subCLName );
}

function testRenameMainCLByDiffCS ( maincsName, oldMainCLName, newMainCLName, subcsName, subCLName1, subCLName2 )
{
   var newMaincl2 = db.getCS( maincsName ).getCL( oldMainCLName );
   var options = { LowBound: { "a": 1000 }, UpBound: { "a": 2000 } };
   newMaincl2.attachCL( COMMCSNAME + "." + subCLName2, options );
   db.getCS( maincsName ).renameCL( oldMainCLName, newMainCLName );
   checkRenameMainCLResult( maincsName, maincsName, newMainCLName, oldMainCLName, subCLName1 );
   checkRenameMainCLResult( maincsName, subcsName, newMainCLName, oldMainCLName, subCLName2 );
}
