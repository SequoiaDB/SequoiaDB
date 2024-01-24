/******************************************************************************
 * @Description   : seqDB-26662:合并无交集的 MCV
 * @Author        : Lin Suqiang
 * @CreateTime    : 2022.07.20
 * @LastEditTime  : 2022.07.20
 * @LastEditors   : Lin Suqiang
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;

main( test );

function test()
{
   var mainCLName = "mainCL_26662";
   var subCLName1 = "subCL_26662_1";
   var subCLName2 = "subCL_26662_2";
   var subCLName3 = "subCL_26662_3";
   var dataGroupNames = commGetDataGroupNames( db);

   var mainCL = commCreateCL( db, COMMCSNAME, mainCLName, {
      "IsMainCL": true,
      "ShardingKey": { "a": 1 },
      "ShardingType": "range"
   } );
   var subCL1 = commCreateCL( db, COMMCSNAME, subCLName1, {
      "ShardingKey": { "a": 1 },
      "ShardingType": "range",
      "Group": dataGroupNames[ 0 ]
   } );
   subCL1.split( dataGroupNames[ 0 ], dataGroupNames[ 1 ], { "a": 0 }, { "a": 100 } );
   var subCL2 = commCreateCL( db, COMMCSNAME, subCLName2, {
      "ShardingKey": { "a": 1 },
      "ShardingType": "hash",
      "Group": dataGroupNames[ 0 ]
   } );
   subCL2.split( dataGroupNames[ 0 ], dataGroupNames[ 1 ], 50 );
   var subCL3 = commCreateCL( db, COMMCSNAME, subCLName3 );

   mainCL.attachCL( COMMCSNAME + "." + subCLName1,
      { "LowBound": { "a": 0 }, "UpBound": { "a": 200 } } );
   mainCL.attachCL( COMMCSNAME + "." + subCLName2,
      { "LowBound": { "a": 200 }, "UpBound": { "a": 400 } } );
   mainCL.attachCL( COMMCSNAME + "." + subCLName3,
      { "LowBound": { "a": 400 }, "UpBound": { "a": 600 } } );

   var bulk = [];
   var totalRecords = 500 * 1000;
   var bulkSize = 1000;
   for( var i = 0; i < totalRecords; i++ )
   {
      var value = getRandomInt( 0, 50 * 1000 );
      bulk.push( { "a": ( value % 600 ), "b": value } );
      if ( bulk.length >= bulkSize )
      {
         mainCL.insert( bulk );
         bulk = [];
      }
   }

   commCreateIndex ( mainCL, "index_26662", { "b": 1 } );

   db.analyze( {
      "Collection": COMMCSNAME + "." + mainCLName,
      "Index": "index_26662",
      "SampleNum": 10000
   } );

   // 手工检查耗时，不能太久
   var startTime = Date.now();
   var actResult = mainCL.getIndexStat( "index_26662", true ).toObj();
   var endTime = Date.now();
   println( "Cost:" + ( ( endTime - startTime ) / 1000 ) + " s" );

   // 检查 MCV 值有序性
   var valueArray = actResult.MCV.Values;
   for ( var i = 1; i < valueArray.length; i++ )
   {
      var l = valueArray[ i - 1 ];
      var r = valueArray[ i ];
      if ( l.b >= r.b )
      {
         throw new Error( "\nMCV is not in order:\n" + JSON.stringify( valueArray ) );
      }
   }
   // 手工检查 MCV 大小，应该接近于 1W 但是不保证一定是 1W
   println( "Values length: " + valueArray.length );
   if ( valueArray.length > 10000 )
   {
      throw new Error( "\nMCV size is too big:\n" + valueArray.length );
   }

   // 简单覆盖其它字段内容正确性
   delete( actResult.StatTimestamp );
   delete( actResult.TotalIndexLevels );
   delete( actResult.TotalIndexPages );
   delete( actResult.DistinctValNum );
   delete( actResult.MinValue );
   delete( actResult.MaxValue );
   delete( actResult.SampleRecords );
   delete( actResult.MCV );
   var expResult = {
      "Collection": COMMCSNAME + "." + mainCLName,
      "Index": "index_26662",
      "Unique": false,
      "KeyPattern": { "b": 1 },
      "NullFrac": 0,
      "UndefFrac": 0,
      "TotalRecords": totalRecords
   };
   assert.equal( actResult, expResult );
   commDropCL ( db, COMMCSNAME, mainCLName );
}
