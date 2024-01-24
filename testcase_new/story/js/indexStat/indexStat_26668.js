/******************************************************************************
 * @Description   : seqDB-26668:js 驱动功能测试
 * @Author        : Lin Suqiang
 * @CreateTime    : 2022.07.21
 * @LastEditTime  : 2022.07.21
 * @LastEditors   : Lin Suqiang
 ******************************************************************************/
testConf.clName = "cl_26668";

main( test );

function test ()
{
   var clName = testConf.clName;
   var cl = testPara.testCL;
   var indexName = "index_26668";

   commCreateIndex( cl, indexName, { "k": 1 } );
   cl.insert( { "k": 1 } );
   db.analyze( {
      "Collection": COMMCSNAME + "." + clName,
      "Index": indexName,
   } );

   var actResult = cl.getIndexStat( indexName, true ).toObj();
   delete ( actResult.StatTimestamp );
   delete ( actResult.TotalIndexLevels );
   delete ( actResult.TotalIndexPages );
   var expResult = {
      "Collection": COMMCSNAME + "." + clName,
      "Index": indexName,
      "Unique": false,
      "KeyPattern": { "k": 1 },
      "DistinctValNum": [1],
      "MinValue": { "k": 1 },
      "MaxValue": { "k": 1 },
      "NullFrac": 0,
      "UndefFrac": 0,
      "MCV": {
         "Values": [{ "k": 1 }],
         "Frac": [10000]
      },
      "SampleRecords": 1,
      "TotalRecords": 1
   };
   assert.equal( actResult, expResult );

   var actResult = cl.getIndexStat( indexName, false ).toObj();
   delete ( actResult.StatTimestamp );
   delete ( actResult.TotalIndexLevels );
   delete ( actResult.TotalIndexPages );
   var expResult = {
      "Collection": COMMCSNAME + "." + clName,
      "Index": indexName,
      "Unique": false,
      "KeyPattern": { "k": 1 },
      "DistinctValNum": [1],
      "MinValue": { "k": 1 },
      "MaxValue": { "k": 1 },
      "NullFrac": 0,
      "UndefFrac": 0,
      "SampleRecords": 1,
      "TotalRecords": 1
   };
   assert.equal( actResult, expResult );
}

