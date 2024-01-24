/******************************************************************************
 * @Description   : seqDB-26664:合并空 MCV
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
   var mainCLName = "mainCL_26664";
   var subCLName1 = "subCL_26664_1";
   var subCLName2 = "subCL_26664_2";
   var subCLName3 = "subCL_26664_3";
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

   commCreateIndex( mainCL, "index_26664", { "b": 1 } );

   db.analyze( {
      "Collection": COMMCSNAME + "." + mainCLName,
      "Index": "index_26664",
      "SampleNum": 10000
   } );

   var actResult = mainCL.getIndexStat( "index_26664", true ).toObj();
   // 简单覆盖其它字段内容正确性
   delete ( actResult.StatTimestamp );
   delete ( actResult.TotalIndexLevels );
   delete ( actResult.TotalIndexPages );
   var expResult = {
      "Collection": COMMCSNAME + "." + mainCLName,
      "Index": "index_26664",
      "Unique": false,
      "KeyPattern": { "b": 1 },
      "DistinctValNum": [0],
      "MinValue": null,
      "MaxValue": null,
      "MCV": {
         "Values": [],
         "Frac": []
      },
      "NullFrac": 0,
      "UndefFrac": 0,
      "SampleRecords": 0,
      "TotalRecords": 0
   };
   assert.equal( actResult, expResult );
   commDropCL( db, COMMCSNAME, mainCLName );
}

