/******************************************************************************
 * @Description   : seqDB-26663:合并有交集的 MCV
 * @Author        : Lin Suqiang
 * @CreateTime    : 2022.07.21
 * @LastEditTime  : 2022.07.21
 * @LastEditors   : Lin Suqiang
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;

main( test );

function test ()
{
   var mainCLName = "mainCL_26663";
   var subCLName1 = "subCL_26663_1";
   var subCLName2 = "subCL_26663_2";
   var subCLName3 = "subCL_26663_3";
   var dataGroupNames = commGetDataGroupNames( db );

   var mainCL = commCreateCL( db, COMMCSNAME, mainCLName, {
      "IsMainCL": true,
      "ShardingKey": { "a": 1 },
      "ShardingType": "range"
   } );
   var subCL1 = commCreateCL( db, COMMCSNAME, subCLName1, {
      "ShardingKey": { "a": 1 },
      "ShardingType": "range",
      "Group": dataGroupNames[0]
   } );
   subCL1.split( dataGroupNames[0], dataGroupNames[1], { "a": 0 }, { "a": 100 } );
   var subCL2 = commCreateCL( db, COMMCSNAME, subCLName2, {
      "ShardingKey": { "a": 1 },
      "ShardingType": "hash",
      "Group": dataGroupNames[0]
   } );
   subCL2.split( dataGroupNames[0], dataGroupNames[1], 50 );
   var subCL3 = commCreateCL( db, COMMCSNAME, subCLName3 );

   mainCL.attachCL( COMMCSNAME + "." + subCLName1,
      { "LowBound": { "a": 0 }, "UpBound": { "a": 200 } } );
   mainCL.attachCL( COMMCSNAME + "." + subCLName2,
      { "LowBound": { "a": 200 }, "UpBound": { "a": 400 } } );
   mainCL.attachCL( COMMCSNAME + "." + subCLName3,
      { "LowBound": { "a": 400 }, "UpBound": { "a": 600 } } );

   // Data distribution
   // Duplicate:  5000  500   50    5     1
   // Distinct:   10    100   1K    1W    1W

   var distinctNumArray = [10, 100, 1000, 10000, 10000];
   var duplicateNumArray = [5000, 500, 50, 5, 1];

   var rangeStart = 0;
   var rangeEnd = 0;
   for( var i = 0; i < distinctNumArray.length; i++ )
   {
      var bulk = [];
      var distinctNum = distinctNumArray[i];
      var duplicateNum = duplicateNumArray[i];
      rangeStart = rangeEnd;
      rangeEnd = rangeStart + distinctNum;
      for( var j = rangeStart; j < rangeEnd; j++ )
      {
         bulk.push( { "a": getRandomInt( 0, 600 ), "b": j } );
      }
      for( var j = 0; j < duplicateNum; j++ )
      {
         mainCL.insert( bulk );
      }
   }

   commCreateIndex( mainCL, "index_26663", { "b": -1 } );

   db.analyze( {
      "Collection": COMMCSNAME + "." + mainCLName,
      "Index": "index_26663",
      "SampleNum": 10000
   } );

   var actResult = mainCL.getIndexStat( "index_26663", true ).toObj();
   var valueArray = actResult.MCV.Values;

   println( "Values length: " + valueArray.length );
   if( valueArray.length > 10000 )
   {
      throw new Error( "\nMCV size is too big:\n" + valueArray.length );
   }

   for( var i = 1; i < valueArray.length; i++ )
   {
      var l = valueArray[i - 1];
      var r = valueArray[i];
      if( l.b >= r.b )
      {
         throw new Error( "\nMCV is not in order:\n" + JSON.stringify( valueArray ) );
      }
   }

   // 高频值要被保留，低频值应该被淘汰，所以统计低频值的 frac 应该比实际占比少
   var rareValueRangeStart = 0;
   var rareThreshold = 5;
   for( var i = 0; i < distinctNumArray.length; i++ )
   {
      if( duplicateNumArray[i] <= rareThreshold )
      {
         rareValueRangeStart += distinctNumArray[i];
      }
   }

   var fracArray = actResult.MCV.Frac;
   var statsRareFrac = 0;
   for( var i = 0; i < valueArray.length; i++ )
   {
      // These value has few duplication
      if( valueArray[i].b >= rareValueRangeStart )
      {
         statsRareFrac += fracArray[i];
      }
   }

   var totalRecords = 0;
   var rareValueRecords = 0;
   for( var i = 0; i < duplicateNumArray.length; i++ )
   {
      var records = duplicateNumArray[i] * distinctNumArray[i];
      totalRecords += records;
      if( duplicateNumArray[i] <= rareThreshold )
      {
         rareValueRecords += records;
      }
   }

   var realRareFrac = ( rareValueRecords / totalRecords ) * 10000;
   println( "statsRareFrac:" + statsRareFrac );
   println( "realRareFrac:" + realRareFrac );
   if( statsRareFrac >= realRareFrac )
   {
      throw new Error( "\nMCV doesn't keep the most frequent values:\n" +
         JSON.stringify( actResult.MCV ) );
   }

   // 简单覆盖其它字段内容正确性
   delete ( actResult.StatTimestamp );
   delete ( actResult.TotalIndexLevels );
   delete ( actResult.TotalIndexPages );
   delete ( actResult.DistinctValNum );
   delete ( actResult.MinValue );
   delete ( actResult.MaxValue );
   delete ( actResult.SampleRecords );
   delete ( actResult.MCV );
   var expResult = {
      "Collection": COMMCSNAME + "." + mainCLName,
      "Index": "index_26663",
      "Unique": false,
      "KeyPattern": { "b": -1 },
      "NullFrac": 0,
      "UndefFrac": 0,
      "TotalRecords": totalRecords
   };
   assert.equal( actResult, expResult );
   commDropCL( db, COMMCSNAME, mainCLName );
}

