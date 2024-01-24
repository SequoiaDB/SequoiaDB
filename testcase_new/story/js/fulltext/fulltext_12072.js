/***************************************************************************
@Description :seqDB-12072 :通过主表创建了全文索引，删除主表 
@Modify list :
              2018-11-27  YinZhen  Create
****************************************************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   //创建主子表，并在主表中创建全文索引 2.通过主表插入部分数据，数据分布在各子表中
   var mainclName = COMMCLNAME + "_ES_12072";
   dropCL( db, COMMCSNAME, mainclName, true, true );
   var mainCL = commCreateCL( db, COMMCSNAME, mainclName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   var subCLName1 = COMMCLNAME + "slave1_cl_12072";
   dropCL( db, COMMCSNAME, subCLName1, true, true );
   var subCL1 = commCreateCL( db, COMMCSNAME, subCLName1 );
   var subCLName2 = COMMCLNAME + "slave2_cl_12072";
   dropCL( db, COMMCSNAME, subCLName2, true, true );
   var subCL2 = commCreateCL( db, COMMCSNAME, subCLName2 );
   mainCL.attachCL( COMMCSNAME + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 4567 } } );
   mainCL.attachCL( COMMCSNAME + "." + subCLName2, { LowBound: { a: 4567 }, UpBound: { a: 10001 } } );

   //create index
   commCreateIndex( mainCL, "fullIndex_12072", { b: "text" } );

   //insert records
   var records = new Array();
   for( var i = 0; i < 10000; i++ )
   {
      var record = { a: i, b: "b" + i };
      records.push( record );
   }
   mainCL.insert( records );
   checkMainCLFullSyncToES( COMMCSNAME, mainclName, "fullIndex_12072", 10000 )

   //删除某个子表
   var dbOperator = new DBOperator();
   var esIndexNames1 = dbOperator.getESIndexNames( COMMCSNAME, subCLName1, "fullIndex_12072" );
   var esIndexNames2 = dbOperator.getESIndexNames( COMMCSNAME, subCLName2, "fullIndex_12072" );
   db.getCS( COMMCSNAME ).dropCL( subCLName1 );
   checkMainCLFullSyncToES( COMMCSNAME, mainclName, "fullIndex_12072", 10000 - 4567 );

   //get actResult and expResult
   var actResult = dbOperator.findFromCL( mainCL, { "": { $Text: { "query": { "match_all": {} } } } }, null, { "_id": 1 } );
   var expResult = dbOperator.findFromCL( subCL2, null, null, { "_id": 1 } );
   checkResult( expResult, actResult );

   checkConsistency( COMMCSNAME, subCLName2 );
   checkInspectResult( COMMCSNAME, subCLName2, 5 );

   dropCL( db, COMMCSNAME, subCLName1, true, true );
   dropCL( db, COMMCSNAME, subCLName2, true, true );
   dropCL( db, COMMCSNAME, mainclName, true, true );

   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames1 );
   checkIndexNotExistInES( esIndexNames2 );
}
