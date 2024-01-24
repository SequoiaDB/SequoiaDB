/***************************************************************************
@Description : seqDB-22652:主子表中获取索引统计信息 
@Modify list : Zhao Xiaoni 2020/8/20
****************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test()
{
   var mainCLName = "mainCL_22652";
   var subCLName1 = "subCL_22652_1"; 
   var subCLName2 = "subCL_22652_2";
   var mainCL = commCreateCL( db, COMMCSNAME, mainCLName, { "IsMainCL": true, "ShardingKey": { "a": 1 }, "ShardingType": "range" } );
   var subCL1 = commCreateCL( db, COMMCSNAME, subCLName1, { "ShardingKey": { "a": 1 }, "ShardingType": "range" } );
   var subCL2 = commCreateCL( db, COMMCSNAME, subCLName2, { "ShardingKey": { "a": 1 }, "ShardingType": "hash" } );
   mainCL.attachCL( COMMCSNAME + "." + subCLName1, { "LowBound": { "a": 0 }, "UpBound": { "a": 200 } } );
   mainCL.attachCL( COMMCSNAME + "." + subCLName2, { "LowBound": { "a": 200 }, "UpBound": { "a": 400 } } );
   commCreateIndex ( mainCL, "index_22652", { "b": 1 } );

   var array = new Array( 1024 );
   array = array.join( "a" );
   var records = [];
   for( var i = 0; i < 210; i++ )
   {
      records.push( { "a": i, "b": array + i } );
   }
   mainCL.insert( records );

   db.analyze( { "Collection": COMMCSNAME + "." + mainCLName, "Index": "index_22652" } );
   
   //连接子表1查询索引统计信息
   var actResult = subCL1.getIndexStat( "index_22652" ).toObj();
   //取样时从索引树上层往下取，只取满足sampleRecord的那层，如果没有，则取最后一层，所以以下参数不参与比较
   delete( actResult.StatTimestamp );
   delete( actResult.DistinctValNum );
   delete( actResult.SampleRecords );
   delete( actResult.MinValue );
   delete( actResult.MaxValue );
   delete( actResult.TotalIndexPages );
   var expResult = { "Collection": COMMCSNAME + "." + subCLName1, "Index": "index_22652", "Unique": false, "KeyPattern": { "b": 1 }, "TotalIndexLevels": 2, "NullFrac": 0, "UndefFrac": 0, "TotalRecords": 200 };
   if( !commCompareObject( expResult, actResult ) )
   {
      throw new Error( "\nExpected:\n" + JSON.stringify( expResult ) + "\nactual:\n" + JSON.stringify( actResult ) );
   }

   //连接子表2查询索引统计信息
   actResult = subCL2.getIndexStat( "index_22652" ).toObj();
   delete( actResult.StatTimestamp );
   expResult = { "Collection": COMMCSNAME + "." + subCLName2, "Index": "index_22652", "Unique": false, "KeyPattern": { "b": 1 }, "TotalIndexLevels": 1, "TotalIndexPages": 1, "DistinctValNum": [ 10 ], "MinValue": { "b": array + 200 }, "MaxValue": { "b": array + 209 }, "NullFrac": 0, "UndefFrac": 0, "SampleRecords": 10, "TotalRecords": 10 };
   if( !commCompareObject( expResult, actResult ) )
   {
      throw new Error( "\nExpected:\n" + JSON.stringify( expResult ) + "\nactual:\n" + JSON.stringify( actResult ) );
   }


   //连接主表查询索引统计信息  
   actResult = mainCL.getIndexStat( "index_22652" ).toObj();
   delete( actResult.StatTimestamp );
   delete( actResult.DistinctValNum );
   delete( actResult.SampleRecords );
   delete( actResult.MinValue );
   delete( actResult.MaxValue );
   expResult = { "Collection": COMMCSNAME + "." + mainCLName, "Index": "index_22652", "Unique": false, "KeyPattern": { "b": 1 }, "TotalIndexLevels": 2, "TotalIndexPages": 5, "NullFrac": 0, "UndefFrac": 0, "TotalRecords": 210 }; 
   if( !commCompareObject( expResult, actResult ) )
   {
      throw new Error( "\nExpected:\n" + JSON.stringify( expResult ) + "\nactual:\n" + JSON.stringify( actResult ) );
   }  

   commDropCL ( db, COMMCSNAME, mainCLName );
}
