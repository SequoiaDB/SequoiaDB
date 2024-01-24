/******************************************************************************
 * @Description   : seqDB-26665:普通表合并 MCV
                    seqDB-26666:哈希分区表合并 MCV
                    seqDB-26667:范围分区表合并 MCV
 * @Author        : Lin Suqiang
 * @CreateTime    : 2022.07.21
 * @LastEditTime  : 2022.07.21
 * @LastEditors   : Lin Suqiang
 ******************************************************************************/
main( test );

function testCLMCV ( clName, indexName )
{
   var cl = db.getCS( COMMCSNAME ).getCL( clName );
   var onePartRecords = [];
   var totalRecords = 1000;
   var duplicateNum = 10;
   for( var i = 0; i < ( totalRecords / 10 ); i++ )
   {
      onePartRecords.push( { "a": ( i % 10 ), "b": i } );
   }
   for( var j = 0; j < duplicateNum; j++ )
   {
      cl.insert( onePartRecords );
   }

   db.analyze( {
      "Collection": COMMCSNAME + "." + clName,
      "Index": indexName,
      "SampleNum": 1000
   } );

   var actResult = cl.getIndexStat( indexName, true ).toObj();
   delete ( actResult.StatTimestamp );
   delete ( actResult.TotalIndexLevels );
   delete ( actResult.TotalIndexPages );
   var values = onePartRecords;
   values.sort(
      function( l, r )
      {
         if( l.a != r.a )
         {
            return l.a - r.a;
         }
         else
         {
            return l.b - r.b;
         }
      } );
   var fracs = [];
   for( var i = 0; i < onePartRecords.length; i++ )
   {
      fracs.push( 100 );
   }
   var expResult = {
      "Collection": COMMCSNAME + "." + clName,
      "Index": indexName,
      "Unique": false,
      "KeyPattern": { "a": 1, "b": -1 },
      "DistinctValNum": [10, 100],
      "MinValue": values[0],
      "MaxValue": values[values.length - 1],
      "NullFrac": 0,
      "UndefFrac": 0,
      "MCV": {
         "Values": values,
         "Frac": fracs
      },
      "SampleRecords": totalRecords,
      "TotalRecords": totalRecords
   };
   assert.equal( actResult, expResult );
}

function test ()
{
   // normal collection
   var testcaseNo = "26665";
   var clName = "cl_" + testcaseNo;
   var indexName = "index_" + testcaseNo;
   var cl = commCreateCL( db, COMMCSNAME, clName, {} );
   commCreateIndex( cl, indexName, { "a": 1, "b": -1 } );
   testCLMCV( clName, indexName );
   commDropCL( db, COMMCSNAME, clName );

   if( true == commIsStandalone( db ) )
   {
      return;
   }

   // no sharding hash collection
   var testcaseNo = "26666_1";
   var clName = "cl_" + testcaseNo;
   var indexName = "index_" + testcaseNo;
   var cl = commCreateCL( db, COMMCSNAME, clName, {
      "ShardingKey": { "a": 1 },
      "ShardingType": "hash",
   } );
   commCreateIndex( cl, indexName, { "a": 1, "b": -1 } );
   testCLMCV( clName, indexName );
   commDropCL( db, COMMCSNAME, clName );

   // no sharding range collection
   var testcaseNo = "26667_1";
   var clName = "cl_" + testcaseNo;
   var indexName = "index_" + testcaseNo;
   var cl = commCreateCL( db, COMMCSNAME, clName, {
      "ShardingKey": { "a": 1 },
      "ShardingType": "range",
   } );
   commCreateIndex( cl, indexName, { "a": 1, "b": -1 } );
   testCLMCV( clName, indexName );
   commDropCL( db, COMMCSNAME, clName );

   var dataGroupNames = commGetDataGroupNames( db );
   if( 1 === dataGroupNames.length )
   {
      return;
   }

   // hash collection
   var testcaseNo = "26666_2";
   var clName = "cl_" + testcaseNo;
   var indexName = "index_" + testcaseNo;
   var cl = commCreateCL( db, COMMCSNAME, clName, {
      "ShardingKey": { "a": 1 },
      "ShardingType": "hash",
      "Group": dataGroupNames[0]
   } );
   cl.split( dataGroupNames[0], dataGroupNames[1], 50 );
   commCreateIndex( cl, indexName, { "a": 1, "b": -1 } );
   testCLMCV( clName, indexName );
   commDropCL( db, COMMCSNAME, clName );

   // range collection
   var testcaseNo = "26667_2";
   var clName = "cl_" + testcaseNo;
   var indexName = "index_" + testcaseNo;
   var cl = commCreateCL( db, COMMCSNAME, clName, {
      "ShardingKey": { "a": 1 },
      "ShardingType": "range",
      "Group": dataGroupNames[0]
   } );
   cl.split( dataGroupNames[0], dataGroupNames[1], { "a": 0 }, { "a": 5 } );
   commCreateIndex( cl, indexName, { "a": 1, "b": -1 } );
   testCLMCV( clName, indexName );
   commDropCL( db, COMMCSNAME, clName );
}
