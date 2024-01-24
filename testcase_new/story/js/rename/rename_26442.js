/******************************************************************************
 * @Description   : seqDB-26442:子表做findAndModify操作后另外一个子表cs.cl做rename操作
 * @Author        : Zhangyanan
 * @CreateTime    : 2022.05.05
 * @LastEditTime  : 2022.06.20
 * @LastEditors   : Zhang Yanan
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var csName1 = "CS_26442_1";
   var csName2 = "CS_26442_2";
   var newCsName2 = "CS_26442_2_new";
   var subCLName2 = "subCL_26442_2";
   var mainCLName = "mainCL_26442";
   var subCLName1 = "subCL_26442_1";
   var findAndModifyNum = 1022;
   var groups = commGetDataGroupNames( db );


   commDropCS( db, csName1, true );
   commDropCS( db, csName2, true );

   var cs1 = commCreateCS( db, csName1, false, "Failed to create CS." );
   var cs2 = commCreateCS( db, csName2, false, "Failed to create CS." );

   var mainCL = cs1.createCL( mainCLName, { ShardingKey: { a: 1 }, IsMainCL: true } );
   var subCL1 = cs1.createCL( subCLName1, { "Group": groups[0] } );
   cs2.createCL( subCLName2, { "Group": groups[0] } );
   mainCL.attachCL( csName1 + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 10 } } );
   mainCL.attachCL( csName2 + "." + subCLName2, { LowBound: { a: 10 }, UpBound: { a: 100 } } );

   var docs = [];
   for( var i = 0; i < 2000; i++ )
   {
      docs.push( { a: 1, b: 1, c: 1 } );
   }
   subCL1.insert( docs );

   executeFindAndModify( subCL1, findAndModifyNum );

   var rc = subCL1.find( { a: 1 } ).update( { $set: { c: 46 } } );
   var j = 0;
   while( rc.next() && j < findAndModifyNum )
   {
      j++;
   }

   var newDB = new Sdb( COORDHOSTNAME, COORDSVCNAME );

   newDB.renameCS( csName2, newCsName2 );
   checkRenameCSResult( csName2, newCsName2 );

   rc.close();
   newDB.close();
   commDropCS( db, csName1, false );
   commDropCS( db, newCsName2, false );
}

function executeFindAndModify ( dbcl, findAndModifyNum )
{
   // 执行5个查询后，使主表的访问计划生效 
   for( var i = 0; i < 5; i++ )
   {
      var rc = dbcl.find( { a: i } ).update( { $set: { c: 46 } } );
      var j = 0;
      while( rc.next() && j < findAndModifyNum )
      {
         j++;
      };
      rc.close();
   }
}