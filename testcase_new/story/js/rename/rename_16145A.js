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
   var oldcsName = CHANGEDPREFIX + "_16145A_oldcs";
   var newcsName = CHANGEDPREFIX + "_16145A_newcs";
   var mainCLName = CHANGEDPREFIX + "_16145A_mainCL";
   var subCLName1 = CHANGEDPREFIX + "_16145A_subCL1";
   var subCLName2 = CHANGEDPREFIX + "_16145A_subCL2";

   var cs = commCreateCS( db, oldcsName, false, "create cs in begine" );
   var mainOptions = { ShardingType: 'range', ShardingKey: { a: 1 }, IsMainCL: true };
   var subOptions = { ShardingType: 'hash', ShardingKey: { a: 1 } };
   var mainCL = commCreateCL( db, oldcsName, mainCLName, mainOptions, false, false, "create MainCL in the begin" );
   var subCL = commCreateCL( db, oldcsName, subCLName1, subOptions, false, false, "create SubCL in the begin" );
   var subCL = commCreateCL( db, oldcsName, subCLName2, {}, false, false, "create SubCL in the begin" );

   mainCL.attachCL( oldcsName + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 500 } } );
   mainCL.attachCL( oldcsName + "." + subCLName2, { LowBound: { a: 500 }, UpBound: { a: 1000 } } );

   //insert 1000 data
   insertData( mainCL, 1000 );

   db.renameCS( oldcsName, newcsName );

   checkRenameCSResult( oldcsName, newcsName, 2 );

   mainCL = db.getCS( newcsName ).getCL( mainCLName );

   //insert 1000 data, and check data
   insertData( mainCL, 1000 );

   //update ($set: {no:10086}) 2000 data, and check data
   updateData( mainCL );

   //delete no < 500 data, and check data
   deleteData( mainCL );

   commDropCS( db, newcsName, true, "clean cs---" );
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