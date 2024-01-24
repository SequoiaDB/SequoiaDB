/* *****************************************************************************
@discretion: rename subcl
@author��2018-10-15 wuyan  Init
***************************************************************************** */

main( test );
function test ()
{
   if( true == commIsStandalone( db ) )
   {
      return;
   }

   var mainCLName = CHANGEDPREFIX + "_rename_maincl16060";
   var csName = CHANGEDPREFIX + "_cs16060";
   var subCLName1 = CHANGEDPREFIX + "_subcl16060a";
   var subCLName2 = CHANGEDPREFIX + "_subcl16060b";
   var newSubCLName1 = CHANGEDPREFIX + "_newsubcl16060a";
   var newSubCLName2 = CHANGEDPREFIX + "_newsubcl16060b";
   commDropCS( db, csName, true, "Failed to drop CS." );
   commDropCL( db, COMMCSNAME, subCLName2, true, true, "clear collection in the beginning" );
   commDropCL( db, COMMCSNAME, newSubCLName2, true, true, "clear collection in the beginning" );

   commCreateCS( db, csName, false, "Failed to create CS." );
   createCLAndAttachCL( csName, mainCLName, COMMCSNAME, subCLName1, subCLName2 )

   //test a: rename mainCL with subcls, maincl and subcl in different cs
   testRenameSubCLByDiffCS( csName, mainCLName, COMMCSNAME, subCLName2, newSubCLName2 )

   //test b: rename mainCL with subcls, maincl and subcl in same cs   
   testRenameSubCLBySameCS( csName, mainCLName, csName, subCLName1, newSubCLName1 );

   commDropCS( db, csName, true, "clear cs in the ending" );
   commDropCL( db, COMMCSNAME, subCLName2, true, true, "clear collection in the ending" );
   commDropCL( db, COMMCSNAME, newSubCLName2, true, true, "clear collection in the ending" );
}

function createCLAndAttachCL ( maincsName, mainCLName, subcsName, subCLName1, subCLName2 )
{
   var mainclShardingKey = { a: 1 };
   var dbcl = createMainCL( maincsName, mainCLName, mainclShardingKey );
   var shardingKey = { no: 1 };
   createCL( maincsName, subCLName1, shardingKey );
   createCL( subcsName, subCLName2, shardingKey );

   var options = { LowBound: { "a": 0 }, UpBound: { "a": 1000 } };
   dbcl.attachCL( maincsName + "." + subCLName1, options );

   var options = { LowBound: { "a": 1000 }, UpBound: { "a": 2000 } };
   dbcl.attachCL( subcsName + "." + subCLName2, options );
}

function testRenameSubCLBySameCS ( maincsName, mainCLName, subcsName, oldSubCLName, newSubCLName )
{
   db.getCS( maincsName ).renameCL( oldSubCLName, newSubCLName );
   checkRenameSubCLResult( maincsName, mainCLName, subcsName, oldSubCLName, newSubCLName );
}

function testRenameSubCLByDiffCS ( maincsName, mainCLName, subcsName, oldSubCLName, newSubCLName )
{
   db.getCS( subcsName ).renameCL( oldSubCLName, newSubCLName );
   checkRenameSubCLResult( maincsName, mainCLName, subcsName, oldSubCLName, newSubCLName );
}