/******************************************************************************
 * @Description   : seqDB-26661:合并无交集的 MCV
 * @Author        : Lin Suqiang
 * @CreateTime    : 2022.07.20
 * @LastEditTime  : 2022.07.20
 * @LastEditors   : Lin Suqiang
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;

main( test );

function test ()
{
   var mainCLName = "mainCL_26661";
   var subCLName1 = "subCL_26661_1";
   var subCLName2 = "subCL_26661_2";
   var subCLName3 = "subCL_26661_3";
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

   commCreateIndex( mainCL, "index_26661", { "a": 1, "b": -1 } );

   var records = [];
   var totalRecords = 2000;
   for( var i = 0; i < totalRecords; i++ )
   {
      records.push( { "a": ( i % 600 ), "b": i } );
   }
   mainCL.insert( records );

   db.analyze( {
      "Collection": COMMCSNAME + "." + mainCLName,
      "Index": "index_26661",
      "SampleNum": totalRecords
   } );

   var actResult = mainCL.getIndexStat( "index_26661", true ).toObj();
   delete ( actResult.StatTimestamp );
   delete ( actResult.TotalIndexLevels );
   delete ( actResult.TotalIndexPages );
   var values = records;
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
   for( var i = 0; i < totalRecords; i++ )
   {
      fracs.push( 5 );
   }
   var expResult = {
      "Collection": COMMCSNAME + "." + mainCLName,
      "Index": "index_26661",
      "Unique": false,
      "KeyPattern": { "a": 1, "b": -1 },
      "DistinctValNum": [600, 2000],
      "MinValue": values[0],
      "MaxValue": values[values.length - 1],
      "MCV": {
         "Values": values,
         "Frac": fracs
      },
      "NullFrac": 0,
      "UndefFrac": 0,
      "SampleRecords": totalRecords,
      "TotalRecords": totalRecords
   };
   assert.equal( actResult, expResult );
   commDropCL( db, COMMCSNAME, mainCLName );
}
