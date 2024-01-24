/***************************************************************************
@Description : seqDB-22653:混合分区表中获取索引统计信息 
@Modify list : Zhao Xiaoni 2020/8/20
****************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test()
{
   var mainCLName = "mainCL_22653";
   var subCLName1 = "subCL_22653_1"; 
   var subCLName2 = "subCL_22653_2";
   var mainCL = commCreateCL( db, COMMCSNAME, mainCLName, { "IsMainCL": true, "ShardingKey": { "a": 1 }, "ShardingType": "range" });
   var subCL1 = commCreateCL( db, COMMCSNAME, subCLName1, { "ShardingKey": { "a": 1 }, "ShardingType": "range" });
   var subCL2 = commCreateCL( db, COMMCSNAME, subCLName2, { "ShardingKey": { "a": 1 }, "ShardingType": "hash", "AutoSplit": true });
   mainCL.attachCL( COMMCSNAME + "." + subCLName1, { "LowBound": { "a": 0 }, "UpBound": { "a": 50 } } );
   mainCL.attachCL( COMMCSNAME + "." + subCLName2, { "LowBound": { "a": 50 }, "UpBound": { "a": 100 } } );

   var groups = commGetGroups ( db );
   commCreateIndex ( mainCL, "index_22653", { "b": 1 } );

   var records = [];
   for( var i = 0; i < 90; i++ )
   {
      records.push( { "a": i, "b": null } );
      records.push( { "a": i } );
   }
   mainCL.insert( records );
   db.analyze( { "Collection": COMMCSNAME + "." + mainCLName, "Index": "index_22653" } );
   
   //连接子表1查询索引统计信息
   var actResult = subCL1.getIndexStat( "index_22653" ).toObj();
   delete( actResult.StatTimestamp );
   var expResult = { "Collection": COMMCSNAME + "." + subCLName1, "Index": "index_22653", "Unique": false, "KeyPattern": { "b": 1 }, "TotalIndexLevels": 1, "TotalIndexPages": 1, "DistinctValNum": [1], "MinValue": null, "MaxValue": null, "NullFrac": 5000, "UndefFrac": 5000, "SampleRecords":100, "TotalRecords": 100 };
   if( !commCompareObject( expResult, actResult ) )
   {
      throw new Error( "\nExpected:\n" + JSON.stringify( expResult ) + "\nactual:\n" + JSON.stringify( actResult ) );
   }

   //连接子表2查询索引统计信息
   actResult = subCL2.getIndexStat( "index_22653" ).toObj();
   delete( actResult.StatTimestamp );
   expResult = { "Collection": COMMCSNAME + "." + subCLName2, "Index": "index_22653", "Unique": false, "KeyPattern": { "b": 1 }, "TotalIndexLevels": 1, "TotalIndexPages": groups.length, "DistinctValNum": [ groups.length ], "MinValue": null, "MaxValue": null, "NullFrac": 5000, "UndefFrac": 5000, "SampleRecords": 80, "TotalRecords": 80 };
   if( !commCompareObject( expResult, actResult ) )
   {
      throw new Error( "\nExpected:\n" + JSON.stringify( expResult ) + "\nactual:\n" + JSON.stringify( actResult ) );
   }


   //连接主表查询索引统计信息  
   actResult = mainCL.getIndexStat( "index_22653" ).toObj();
   delete( actResult.StatTimestamp );
   expResult = { "Collection": COMMCSNAME + "." + mainCLName, "Index": "index_22653", "Unique": false, "KeyPattern": { "b": 1 }, "TotalIndexLevels": 1, "TotalIndexPages": groups.length + 1, "DistinctValNum": [ groups.length + 1], "MinValue": null, "MaxValue": null, "NullFrac": 5000, "UndefFrac": 5000, "SampleRecords": 180, "TotalRecords": 180 }; 
   if( !commCompareObject( expResult, actResult ) )
   {
      throw new Error( "\nExpected:\n" + JSON.stringify( expResult ) + "\nactual:\n" + JSON.stringify( actResult ) );
   }  

   commDropCL ( db, COMMCSNAME, mainCLName );
}


