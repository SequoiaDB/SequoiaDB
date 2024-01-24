/***************************************************************************
@Description :seqDB-12069 :集合空间中包含部分子表，且子表中创建了全文索引，删除集合空间 
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

   var clName = COMMCLNAME + "_ES_12069";
   dropCL( db, COMMCSNAME, clName, true, true );
   var mainCL = commCreateCL( db, COMMCSNAME, clName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   var csName1 = "sub1_cs_12069";
   dropCS( db, csName1 );
   var subCLName1 = "sub1_cl_12069";
   var subCL1 = commCreateCL( db, csName1, subCLName1 );
   var csName2 = "sub2_cs_12069";
   dropCS( db, csName2 );
   var subCLName2 = "sub2_cl_12069";
   var subCL2 = commCreateCL( db, csName2, subCLName2 );
   mainCL.attachCL( csName1 + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 4567 } } );
   mainCL.attachCL( csName2 + "." + subCLName2, { LowBound: { a: 4567 }, UpBound: { a: 10001 } } );

   //create index
   commCreateIndex( mainCL, "fullIndex_12069", { b: "text" } );

   //insert records
   var records = new Array();
   for( var i = 0; i < 10000; i++ )
   {
      var record = { a: i, b: "b" + i };
      records.push( record );
   }
   mainCL.insert( records );
   checkMainCLFullSyncToES( COMMCSNAME, clName, "fullIndex_12069", 10000 )

   //删除部分子表所在的集合空间
   var dbOperator = new DBOperator();
   var subESIndexNames1 = dbOperator.getESIndexNames( csName1, subCLName1, "fullIndex_12069" );
   db.dropCS( csName1 );
   checkMainCLFullSyncToES( COMMCSNAME, clName, "fullIndex_12069", 10000 - 4567 );

   //其余子表主备节点数据一致
   checkConsistency( csName2, subCLName2 );
   checkInspectResult( csName2, subCLName2, 5 );

   var expResult = dbOperator.findFromCL( subCL2, { "": { $Text: { "query": { "match_all": {} } } } }, { b: "" } );
   var subESIndexNames2 = dbOperator.getESIndexNames( csName2, subCLName2, "fullIndex_12069" );
   var actResult = new Array();
   var esOperator = new ESOperator();
   for( var i in subESIndexNames2 )
   {
      var esRecords = esOperator.findFromES( subESIndexNames2[i], '{"query":{"match_all":{}}, "size":10000}' );
      actResult = actResult.concat( esRecords );
   }

   expResult.sort( compare( "b" ) );
   actResult.sort( compare( "b" ) );
   checkResult( expResult, actResult );

   dropCL( db, COMMCSNAME, clName, true, true );
   dropCS( db, csName1 );
   dropCS( db, csName2 );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( subESIndexNames1 );
   checkIndexNotExistInES( subESIndexNames2 );
}
