/***************************************************************************
@Description :seqDB-12070 : 集合空间中包含主表，且通过主表创建了全文索引，删除集合空间 
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

   var clName = COMMCLNAME + "_ES_12070";
   var csName = "main_cs_12070";
   dropCS( db, csName );
   var mainCL = commCreateCL( db, csName, clName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   var subCLName1 = "sub1_cl_12070";
   var subCL1 = commCreateCL( db, csName, subCLName1 );
   var csName2 = "sub2_cs_12070";
   dropCS( db, csName2 );
   var subCLName2 = "sub2_cl_12070";
   var subCL2 = commCreateCL( db, csName2, subCLName2 );
   mainCL.attachCL( csName + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 4567 } } );
   mainCL.attachCL( csName2 + "." + subCLName2, { LowBound: { a: 4567 }, UpBound: { a: 10001 } } );

   //create idnex
   commCreateIndex( mainCL, "fullIndex_12070", { b: "text" } );

   //insert records
   var records = new Array();
   for( var i = 0; i < 10000; i++ )
   {
      var record = { a: i, b: "b" + i };
      records.push( record );
   }
   mainCL.insert( records );
   checkMainCLFullSyncToES( csName, clName, "fullIndex_12070", 10000 )

   //删除主表所在的集合空间
   var dbOperator = new DBOperator();
   var subESIndexNames1 = dbOperator.getESIndexNames( csName, subCLName1, "fullIndex_12070" );
   db.dropCS( csName );
   checkFullSyncToES( csName2, subCLName2, "fullIndex_12070", 10000 - 4567 );

   //其余子表主备节点数据一致
   checkConsistency( csName2, subCLName2 );
   checkInspectResult( csName2, subCLName2, 5 );

   //check records
   var expResult = dbOperator.findFromCL( subCL2, { "": { $Text: { "query": { "match_all": {} } } } }, { b: "" } );
   var subESIndexNames2 = dbOperator.getESIndexNames( csName2, subCLName2, "fullIndex_12070" );

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

   dropCS( db, csName );
   dropCS( db, csName2 );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( subESIndexNames1 );
   checkIndexNotExistInES( subESIndexNames2 );
}
