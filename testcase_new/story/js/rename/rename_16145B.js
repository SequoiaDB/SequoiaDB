/************************************
*@Description: 主子表修改cs名后执行数据操作 
*@author:      luweikang
*@createdate:  2018.10.12
*@testlinkCase:seqDB-16145
**************************************/

main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }
   if( commGetGroupsNum( db ) < 2 )
   {
      return;
   }
   var oldcsName = CHANGEDPREFIX + "_16145B_oldcs";
   var newcsName = CHANGEDPREFIX + "_16145B_newcs";
   var subCSName = CHANGEDPREFIX + "_16145B_subcs";
   var subCSNameNew = CHANGEDPREFIX + "_16145B_sub_newcs";
   var mainCLName = CHANGEDPREFIX + "_16145B_mainCL";
   var subCLName1 = CHANGEDPREFIX + "_16145B_subCL1";
   var subCLName2 = CHANGEDPREFIX + "_16145B_subCL2";

   var cs = commCreateCS( db, oldcsName, false, "create cs in begine" );
   var subCS = commCreateCS( db, subCSName, false, "create cs in begine" );
   var mainOptions = { ShardingType: 'range', ShardingKey: { a: 1 }, IsMainCL: true };
   var subOptions = { ShardingType: 'hash', ShardingKey: { a: 1 } };
   var mainCL = commCreateCL( db, oldcsName, mainCLName, mainOptions, false, false, "create MainCL in the begin" );
   var subCL = commCreateCL( db, subCSName, subCLName1, subOptions, false, false, "create SubCL in the begin" );
   var subCL = commCreateCL( db, subCSName, subCLName2, {}, false, false, "create SubCL in the begin" );
   //create mainCS defaultCL to check cs snapshot result
   var defaultCL = commCreateCL( db, oldcsName, mainCLName + "_defaultCL", {}, false, false, "create MainCL in the begin" );

   mainCL.attachCL( subCSName + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 500 } } );
   mainCL.attachCL( subCSName + "." + subCLName2, { LowBound: { a: 500 }, UpBound: { a: 1000 } } );

   //insert 1000 data
   insertData( mainCL, 1000 );

   db.renameCS( oldcsName, newcsName );

   db.renameCS( subCSName, subCSNameNew );

   checkRenameCSResult( oldcsName, newcsName, 1 );

   checkRenameCSResult( subCSName, subCSNameNew, 2 );

   mainCL = db.getCS( newcsName ).getCL( mainCLName );

   //insert 1000 data, and check data
   insertData( mainCL, 1000 );

   //update ($set: {no:10086}) 2000 data, and check data
   updateData( mainCL );

   //delete no < 500 data, and check data
   deleteData( mainCL );

   commDropCS( db, newcsName, true, "clean cs---" );
   commDropCS( db, subCSNameNew, true, "clean cs---" );
}

function updateData ( cl )
{
   cl.update( { $set: { no: 10086 } } );
   var recordNum = cl.count( { no: 10086 } );
   assert.equal( recordNum, 2000 );
}

function deleteData ( cl )
{
   cl.remove( { a: { $lt: 500 } } );
   var recordNum = cl.count();
   assert.equal( recordNum, 1000 );
}